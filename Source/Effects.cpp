#include "Effects.h"

GateEffect::GateEffect(std::atomic<float>& t, std::atomic<float>& b) : threshold(t), bypass(b) {}

void GateEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    gain.reset(spec.sampleRate, 0.01);
    gain.setCurrentAndTargetValue(1.0f);
}

void GateEffect::reset() { gain.setCurrentAndTargetValue(1.0f); }

void GateEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypass.load() >= 0.5f) return;
    const auto thresholdGain = juce::Decibels::decibelsToGain(threshold.load());
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float detector = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            detector = juce::jmax(detector, std::abs(buffer.getSample(ch, sample)));
        gain.setTargetValue(detector >= thresholdGain ? 1.0f : 0.0f);
        const auto currentGain = gain.getNextValue();
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, sample, buffer.getSample(ch, sample) * currentGain);
    }
}

DriveEffect::DriveEffect(std::atomic<float>& d, std::atomic<float>& m, std::atomic<float>& b)
    : drive(d), mix(m), bypass(b) {}

void DriveEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypass.load() >= 0.5f) return;
    const auto amount = juce::Decibels::decibelsToGain(drive.load());
    const auto wetMix = mix.load() * 0.01f;
    const auto normaliser = std::tanh(amount);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            const auto dry = buffer.getSample(ch, sample);
            const auto wet = std::tanh(dry * amount) / normaliser;
            buffer.setSample(ch, sample, dry + (wet - dry) * wetMix);
        }
}

Dist1Effect::Dist1Effect(std::atomic<float>& d, std::atomic<float>& t,
                                         std::atomic<float>& l, std::atomic<float>& b)
    : distortion(d), tone(t), level(l), bypass(b) {}

void Dist1Effect::prepare(const juce::dsp::ProcessSpec& spec)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing(spec.maximumBlockSize);
    oversampledRate = spec.sampleRate * oversampling->getOversamplingFactor();
    juce::dsp::ProcessSpec highRateSpec{oversampledRate,
        spec.maximumBlockSize * oversampling->getOversamplingFactor(), 1};
    for (auto* bank : {&inputHighPass, &transistorLowPass, &opAmpHighPass, &clippingLowPass,
                       &toneLowPass, &toneHighPass, &dcBlock})
        for (auto& filter : *bank) filter.prepare(highRateSpec);
    updateFilters();
    reset();
}

void Dist1Effect::reset()
{
    if (oversampling != nullptr) oversampling->reset();
    for (auto* bank : {&inputHighPass, &transistorLowPass, &opAmpHighPass, &clippingLowPass,
                       &toneLowPass, &toneHighPass, &dcBlock})
        for (auto& filter : *bank) filter.reset();
}

void Dist1Effect::updateFilters()
{
    // First-edition DIST-1 values: C1/R2 input coupling, Q2's 470k/250p
    // collector roll-off, 4.7k/0.1u Dist feedback, 2.2k/10n clipping
    // node, and the two passive Tone branches (6.8k/0.1u and 6.8k/22n).
    // Each of these networks is a single R/C pole in the analogue circuit.
    // Using a resonant biquad here adds a second pole and makes the two Tone
    // branches almost cancel once their startup transient has decayed.
    const auto input = Coefficients::makeFirstOrderHighPass(oversampledRate, 7.2);
    const auto q2RollOff = Coefficients::makeFirstOrderLowPass(oversampledRate, 1355.0);
    const auto feedbackHighPass = Coefficients::makeFirstOrderHighPass(oversampledRate, 339.0);
    const auto clippingNode = Coefficients::makeFirstOrderLowPass(oversampledRate, 7234.0);
    const auto darkBranch = Coefficients::makeFirstOrderLowPass(oversampledRate, 234.0);
    const auto brightBranch = Coefficients::makeFirstOrderHighPass(oversampledRate, 1060.0);
    const auto dc = Coefficients::makeFirstOrderHighPass(oversampledRate, 12.0);
    for (size_t ch = 0; ch < inputHighPass.size(); ++ch) {
        *inputHighPass[ch].coefficients = *input;
        *transistorLowPass[ch].coefficients = *q2RollOff;
        *opAmpHighPass[ch].coefficients = *feedbackHighPass;
        *clippingLowPass[ch].coefficients = *clippingNode;
        *toneLowPass[ch].coefficients = *darkBranch;
        *toneHighPass[ch].coefficients = *brightBranch;
        *dcBlock[ch].coefficients = *dc;
    }
}

float Dist1Effect::siliconDiodePair(float sample) noexcept
{
    // Antiparallel silicon diodes: firm knee around 0.62 V with a narrow
    // exponential-like transition rather than an ideal digital clipper.
    constexpr float forwardVoltage = 0.62f;
    constexpr float knee = 0.055f;
    const auto magnitude = std::abs(sample);
    const auto limited = magnitude <= forwardVoltage
        ? magnitude
        : forwardVoltage + knee * std::tanh((magnitude - forwardVoltage) / knee);
    return std::copysign(limited, sample);
}

void Dist1Effect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypass.load() >= 0.5f || oversampling == nullptr) return;

    juce::dsp::AudioBlock<float> baseBlock(buffer);
    auto block = oversampling->processSamplesUp(baseBlock);
    const auto dist = juce::jlimit(0.0f, 1.0f, distortion.load() * 0.01f);
    const auto tonePosition = juce::jlimit(0.0f, 1.0f, tone.load() * 0.01f);
    const auto levelPosition = juce::jlimit(0.0f, 1.0f, level.load() * 0.01f);
    const auto output = std::pow(levelPosition, 1.35f) * 1.15f;
    // At audio frequencies the 100k Dist pot over 4.7k gives a theoretical
    // non-inverting gain range of approximately 1..22.3.
    const auto feedbackGain = 1.0f + dist * (100000.0f / 4700.0f);

    for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
        auto* samples = block.getChannelPointer(ch);
        const auto filterChannel = juce::jmin(ch, inputHighPass.size() - 1);
        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto value = inputHighPass[filterChannel].processSample(samples[i]);
            // Q2 is an inverting common-emitter stage. Its 250 pF collector
            // capacitor both limits fizz and makes this stage compress before
            // the op-amp/diode hard clipper.
            value = transistorLowPass[filterChannel].processSample(value);
            value = -std::tanh(value * 14.0f) * 0.42f;
            const auto frequencyDependent = opAmpHighPass[filterChannel].processSample(value);
            value += frequencyDependent * (feedbackGain - 1.0f);
            value = clippingLowPass[filterChannel].processSample(value);
            value = -siliconDiodePair(value); // inverting TA7136 stage
            const auto dark = toneLowPass[filterChannel].processSample(value);
            const auto bright = toneHighPass[filterChannel].processSample(value);
            value = dark * std::sqrt(1.0f - tonePosition) + bright * std::sqrt(tonePosition);
            samples[i] = dcBlock[filterChannel].processSample(value) * output;
        }
    }
    oversampling->processSamplesDown(baseBlock);
}

ToneEffect::ToneEffect(std::atomic<float>& lo, std::atomic<float>& mi, std::atomic<float>& hi,
                       std::atomic<float>& b)
    : bass(lo), mid(mi), treble(hi), bypass(b) {}

void ToneEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    auto monoSpec = spec;
    monoSpec.numChannels = 1;
    for (auto* bank : {&lowShelf, &midPeak, &highShelf})
        for (auto& filter : *bank) filter.prepare(monoSpec);
    reset();
}

void ToneEffect::reset()
{
    for (auto* bank : {&lowShelf, &midPeak, &highShelf})
        for (auto& filter : *bank) filter.reset();
}

void ToneEffect::updateFilters()
{
    const auto low = Coefficients::makeLowShelf(sampleRate, 180.0, 0.707,
        juce::Decibels::decibelsToGain(bass.load()));
    const auto peak = Coefficients::makePeakFilter(sampleRate, 850.0, 0.8,
        juce::Decibels::decibelsToGain(mid.load()));
    const auto high = Coefficients::makeHighShelf(sampleRate, 3500.0, 0.707,
        juce::Decibels::decibelsToGain(treble.load()));
    for (size_t ch = 0; ch < lowShelf.size(); ++ch) {
        *lowShelf[ch].coefficients = *low;
        *midPeak[ch].coefficients = *peak;
        *highShelf[ch].coefficients = *high;
    }
}

void ToneEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypass.load() >= 0.5f) return;
    updateFilters();
    for (int ch = 0; ch < juce::jmin(buffer.getNumChannels(), 2); ++ch)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            auto value = buffer.getSample(ch, sample);
            value = lowShelf[static_cast<size_t>(ch)].processSample(value);
            value = midPeak[static_cast<size_t>(ch)].processSample(value);
            value = highShelf[static_cast<size_t>(ch)].processSample(value);
            buffer.setSample(ch, sample, value);
        }
}

Mars8Effect::Mars8Effect(std::atomic<float>& pre, std::atomic<float>& bass,
    std::atomic<float>& mid, std::atomic<float>& treble, std::atomic<float>& master,
    std::atomic<float>& presence, std::atomic<float>& sag, std::atomic<float>& cab,
    std::atomic<float>& bypass)
    : preampParam(pre), bassParam(bass), middleParam(mid), trebleParam(treble),
      masterParam(master), presenceParam(presence), sagParam(sag),
      cabEnabledParam(cab), bypassParam(bypass) {}

void Mars8Effect::prepare(const juce::dsp::ProcessSpec& spec)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing(spec.maximumBlockSize);
    highRate = spec.sampleRate * oversampling->getOversamplingFactor();
    juce::dsp::ProcessSpec osSpec { highRate,
        spec.maximumBlockSize * oversampling->getOversamplingFactor(), 1 };
    for (auto* bank : {&inputHighPass, &brightHighPass, &toneLow, &toneMid, &toneHigh,
                       &transformerHighPass, &transformerLowPass, &cabHighPass, &cabLowPass,
                       &cabLowBody, &cabMidScoop, &cabPresence, &cabTopNotch})
        for (auto& filter : *bank) filter.prepare(osSpec);
    constexpr std::array<float, 3> couplingCutoffs { 15.0f, 8.0f, 5.0f };
    for (size_t i = 0; i < couplingPole.size(); ++i)
        couplingPole[i] = std::exp(-juce::MathConstants<float>::twoPi * couplingCutoffs[i]
                                   / static_cast<float>(highRate));
    updateFilters(); reset();
}

void Mars8Effect::reset()
{
    if (oversampling) oversampling->reset();
    supplyEnvelope.fill(0.0f);
    couplingInput = {}; couplingOutput = {};
    for (auto* bank : {&inputHighPass, &brightHighPass, &toneLow, &toneMid, &toneHigh,
                       &transformerHighPass, &transformerLowPass, &cabHighPass, &cabLowPass,
                       &cabLowBody, &cabMidScoop, &cabPresence, &cabTopNotch})
        for (auto& filter : *bank) filter.reset();
}

void Mars8Effect::updateFilters()
{
    const auto bass = bassParam.load() * 0.01f, mid = middleParam.load() * 0.01f;
    const auto treble = trebleParam.load() * 0.01f, presence = presenceParam.load() * 0.01f;
    cachedBass = bass; cachedMiddle = mid; cachedTreble = treble; cachedPresence = presence;
    auto set = [] (auto& bank, const auto& c) { for (auto& f : bank) *f.coefficients = *c; };
    set(inputHighPass, Coefficients::makeFirstOrderHighPass(highRate, 7.2));
    set(brightHighPass, Coefficients::makeHighPass(highRate, 720.0, 0.707));
    set(toneLow, Coefficients::makeLowShelf(highRate, 110.0, 0.7, juce::Decibels::decibelsToGain(juce::jmap(bass, -11.0f, 7.0f))));
    set(toneMid, Coefficients::makePeakFilter(highRate, 520.0, 0.65, juce::Decibels::decibelsToGain(juce::jmap(mid, -13.0f, 5.0f))));
    set(toneHigh, Coefficients::makeHighShelf(highRate, 1800.0, 0.7, juce::Decibels::decibelsToGain(juce::jmap(treble, -10.0f, 9.0f))));
    set(transformerHighPass, Coefficients::makeFirstOrderHighPass(highRate, 32.0));
    set(transformerLowPass, Coefficients::makeLowPass(highRate, juce::jmap(presence, 5200.0f, 8200.0f), 0.72));
    set(cabHighPass, Coefficients::makeFirstOrderHighPass(highRate, 68.0));
    set(cabLowPass, Coefficients::makeLowPass(highRate, 5600.0, 0.72));
    set(cabLowBody, Coefficients::makePeakFilter(highRate, 115.0, 1.0, juce::Decibels::decibelsToGain(3.0f)));
    set(cabMidScoop, Coefficients::makePeakFilter(highRate, 520.0, 0.75, juce::Decibels::decibelsToGain(-4.0f)));
    set(cabPresence, Coefficients::makePeakFilter(highRate, 2700.0, 1.15, juce::Decibels::decibelsToGain(4.5f)));
    set(cabTopNotch, Coefficients::makePeakFilter(highRate, 4100.0, 2.2, juce::Decibels::decibelsToGain(-5.0f)));
}

float Mars8Effect::triode(float x, float bias, float hardness) noexcept
{
    const auto rest = std::tanh(bias * hardness);
    return (std::tanh((x + bias) * hardness) - rest) / juce::jmax(0.15f, 1.0f - std::abs(rest));
}

void Mars8Effect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassParam.load() >= 0.5f || !oversampling) return;
    const auto bassNow = bassParam.load() * 0.01f, middleNow = middleParam.load() * 0.01f;
    const auto trebleNow = trebleParam.load() * 0.01f, presenceNow = presenceParam.load() * 0.01f;
    if (bassNow != cachedBass || middleNow != cachedMiddle || trebleNow != cachedTreble || presenceNow != cachedPresence)
        updateFilters();
    juce::dsp::AudioBlock<float> base(buffer); auto block = oversampling->processSamplesUp(base);
    const auto pre = preampParam.load() * 0.01f, master = masterParam.load() * 0.01f;
    const auto sagDepth = sagParam.load() * 0.01f;
    const auto cab = cabEnabledParam.load() >= 0.5f;
    const auto attack = std::exp(-1.0f / static_cast<float>(0.012 * highRate));
    const auto release = std::exp(-1.0f / static_cast<float>(0.18 * highRate));
    auto couple = [this] (float value, size_t stage, size_t channel) noexcept {
        const auto result = value - couplingInput[stage][channel]
                          + couplingPole[stage] * couplingOutput[stage][channel];
        couplingInput[stage][channel] = value;
        couplingOutput[stage][channel] = result;
        return result;
    };
    for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
        auto* data = block.getChannelPointer(ch); const auto c = juce::jmin(ch, size_t{1});
        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = inputHighPass[c].processSample(data[i]);
            const auto bright = brightHighPass[c].processSample(x);
            // Gains are normalised to the waveshaper's unit operating range.
            // Driving std::tanh with the literal voltage gain of every valve
            // stage makes it quantise to a constant +/-1 and lose all AC.
            x = triode((x + bright * (1.0f - pre) * 0.5f) * juce::jmap(pre, 2.0f, 9.0f), 0.10f, 1.05f);
            x = couple(x, 0, c);
            x = triode(x * juce::jmap(pre, 1.2f, 3.4f), -0.34f, 1.35f); // 10k cold clipper
            x = couple(x, 1, c);
            x = triode(x * 1.55f, 0.08f, 1.05f);                        // V2 gain/cathode follower
            x = couple(x, 2, c);
            x = toneLow[c].processSample(x); x = toneMid[c].processSample(x); x = toneHigh[c].processSample(x);
            x *= std::pow(master, 1.4f) * 1.8f;
            const auto piA = triode(x * 1.7f, 0.03f, 1.1f);
            const auto piB = triode(-x * 1.7f, -0.03f, 1.1f);
            auto pushPull = 0.5f * (triode(piA * 2.2f, -0.02f, 1.3f) - triode(piB * 2.2f, 0.02f, 1.3f));
            const auto draw = std::abs(pushPull);
            supplyEnvelope[c] = draw > supplyEnvelope[c] ? attack * supplyEnvelope[c] + (1.0f - attack) * draw
                                                          : release * supplyEnvelope[c] + (1.0f - release) * draw;
            pushPull *= juce::jlimit(0.58f, 1.0f, 1.0f - sagDepth * supplyEnvelope[c] * 0.32f);
            x = transformerHighPass[c].processSample(pushPull);
            x = transformerLowPass[c].processSample(x);
            if (cab) { x = cabHighPass[c].processSample(x); x = cabLowBody[c].processSample(x);
                x = cabMidScoop[c].processSample(x); x = cabPresence[c].processSample(x);
                x = cabTopNotch[c].processSample(x); x = cabLowPass[c].processSample(x); }
            data[i] = x * 0.72f;
        }
    }
    oversampling->processSamplesDown(base);
}
