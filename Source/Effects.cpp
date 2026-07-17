#include "Effects.h"

#include <NAM/get_dsp.h>
namespace iplug { inline constexpr double PI = 3.14159265358979323846; }
#define DEFAULT_BLOCK_SIZE 512
#include <dsp/ResamplingContainer/ResamplingContainer.h>
#undef DEFAULT_BLOCK_SIZE

StargateEffect::StargateEffect(std::atomic<float>& t, std::atomic<float>& b) : threshold(t), bypass(b) {}

void StargateEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    gain.reset(spec.sampleRate, 0.01);
    gain.setCurrentAndTargetValue(1.0f);
}

void StargateEffect::reset() { gain.setCurrentAndTargetValue(1.0f); }

void StargateEffect::process(juce::AudioBuffer<float>& buffer)
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

Deimos1Effect::Deimos1Effect(std::atomic<float>& d, std::atomic<float>& t,
                                         std::atomic<float>& l, std::atomic<float>& b)
    : distortion(d), tone(t), level(l), bypass(b) {}

void Deimos1Effect::prepare(const juce::dsp::ProcessSpec& spec)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing(spec.maximumBlockSize);
    oversampledRate = spec.sampleRate * oversampling->getOversamplingFactor();
    juce::dsp::ProcessSpec highRateSpec{oversampledRate,
        static_cast<juce::uint32>(spec.maximumBlockSize * oversampling->getOversamplingFactor()), 1};
    for (auto* bank : {&inputHighPass, &transistorLowPass, &opAmpHighPass, &clippingLowPass,
                       &toneLowPass, &toneHighPass, &dcBlock})
        for (auto& filter : *bank) filter.prepare(highRateSpec);
    updateFilters();
    reset();
}

void Deimos1Effect::reset()
{
    if (oversampling != nullptr) oversampling->reset();
    for (auto* bank : {&inputHighPass, &transistorLowPass, &opAmpHighPass, &clippingLowPass,
                       &toneLowPass, &toneHighPass, &dcBlock})
        for (auto& filter : *bank) filter.reset();
}

void Deimos1Effect::updateFilters()
{
    // First-edition DEIMOS-1 values: C1/R2 input coupling, Q2's 470k/250p
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

float Deimos1Effect::siliconDiodePair(float sample) noexcept
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

void Deimos1Effect::process(juce::AudioBuffer<float>& buffer)
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

FrequencyEffect::FrequencyEffect(std::atomic<float>& lo, std::atomic<float>& mi, std::atomic<float>& hi,
                       std::atomic<float>& b)
    : bass(lo), mid(mi), treble(hi), bypass(b) {}

void FrequencyEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    auto monoSpec = spec;
    monoSpec.numChannels = 1;
    for (auto* bank : {&lowShelf, &midPeak, &highShelf})
        for (auto& filter : *bank) filter.prepare(monoSpec);
    reset();
}

void FrequencyEffect::reset()
{
    for (auto* bank : {&lowShelf, &midPeak, &highShelf})
        for (auto& filter : *bank) filter.reset();
}

void FrequencyEffect::updateFilters()
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

void FrequencyEffect::process(juce::AudioBuffer<float>& buffer)
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
    std::atomic<float>& bypass, std::atomic<bool>* cabSuppressed)
    : preampParam(pre), bassParam(bass), middleParam(mid), trebleParam(treble),
      masterParam(master), presenceParam(presence), sagParam(sag),
      cabEnabledParam(cab), bypassParam(bypass), cabSuppressedParam(cabSuppressed) {}

void Mars8Effect::prepare(const juce::dsp::ProcessSpec& spec)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing(spec.maximumBlockSize);
    highRate = spec.sampleRate * oversampling->getOversamplingFactor();
    juce::dsp::ProcessSpec osSpec { highRate,
        static_cast<juce::uint32>(spec.maximumBlockSize * oversampling->getOversamplingFactor()), 1 };
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
    const auto cab = cabEnabledParam.load() >= 0.5f
        && (cabSuppressedParam == nullptr || !cabSuppressedParam->load(std::memory_order_relaxed));
    const auto attack = std::exp(-1.0f / static_cast<float>(0.012 * highRate));
    const auto release = std::exp(-1.0f / static_cast<float>(0.18 * highRate));
    auto couple = [this] (float value, size_t stage, size_t channel) noexcept {
        const auto result = value - couplingInput[stage][channel]
                          + couplingPole[stage] * couplingOutput[stage][channel];
        couplingInput[stage][channel] = value;
        couplingOutput[stage][channel] = result;
        return result;
    };
    auto valveStage = [] (float input, float drive, float bias, float hardness,
                          float nonLinearAmount) noexcept {
        const auto linear = input * drive;
        const auto shaped = Mars8Effect::triode(linear, bias, hardness);
        return linear + (shaped - linear) * nonLinearAmount;
    };
    for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
        auto* data = block.getChannelPointer(ch); const auto c = juce::jmin(ch, size_t{1});
        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = inputHighPass[c].processSample(data[i]);
            const auto bright = brightHighPass[c].processSample(x);
            // Gains are normalised to the waveshaper's unit operating range.
            // Driving std::tanh with the literal voltage gain of every valve
            // stage makes it quantise to a constant +/-1 and lose all AC.
            x = valveStage(x + bright * (1.0f - pre) * 0.5f, juce::jmap(pre, 0.85f, 9.0f),
                           0.10f, 1.05f, juce::jmap(pre, 0.04f, 1.0f));
            x = couple(x, 0, c);
            x = valveStage(x, juce::jmap(pre, 0.92f, 3.4f), -0.34f, 1.35f,
                           juce::jmap(pre, 0.025f, 1.0f)); // 10k cold clipper
            x = couple(x, 1, c);
            x = valveStage(x, juce::jmap(pre, 1.0f, 1.55f), 0.08f, 1.05f,
                           juce::jmap(pre, 0.03f, 1.0f)); // V2 gain/cathode follower
            x = couple(x, 2, c);
            x = toneLow[c].processSample(x); x = toneMid[c].processSample(x); x = toneHigh[c].processSample(x);
            x *= std::pow(master, 1.4f) * 1.8f;
            const auto powerColour = juce::jmap(pre, 0.08f, 1.0f);
            const auto piA = valveStage(x, 1.7f, 0.03f, 1.1f, powerColour);
            const auto piB = valveStage(-x, 1.7f, -0.03f, 1.1f, powerColour);
            auto pushPull = 0.5f * (valveStage(piA, 2.2f, -0.02f, 1.3f, powerColour)
                                  - valveStage(piB, 2.2f, 0.02f, 1.3f, powerColour));
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

Vulcan5Effect::Vulcan5Effect(std::atomic<float>& channel, std::atomic<float>& gain,
    std::atomic<float>& bass, std::atomic<float>& middle, std::atomic<float>& treble,
    std::atomic<float>& master, std::atomic<float>& presence, std::atomic<float>& resonance,
    std::atomic<float>& bias, std::atomic<float>& sag, std::atomic<float>& cab,
    std::atomic<float>& bypass, std::atomic<bool>* cabSuppressed)
    : channelParam(channel), gainParam(gain), bassParam(bass), middleParam(middle),
      trebleParam(treble), masterParam(master), presenceParam(presence),
      resonanceParam(resonance), biasParam(bias), sagParam(sag),
      cabEnabledParam(cab), bypassParam(bypass), cabSuppressedParam(cabSuppressed) {}

void Vulcan5Effect::prepare(const juce::dsp::ProcessSpec& spec)
{
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        spec.numChannels, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing(spec.maximumBlockSize);
    highRate = spec.sampleRate * oversampling->getOversamplingFactor();
    juce::dsp::ProcessSpec osSpec { highRate,
        static_cast<juce::uint32>(spec.maximumBlockSize * oversampling->getOversamplingFactor()), 1 };
    for (auto* bank : {&inputHighPass, &interstageLowPass1, &interstageLowPass2,
                       &toneLow, &toneMid, &toneHigh, &transformerHighPass,
                       &transformerLowPass, &resonanceShelf, &cabHighPass,
                       &cabLowPass, &cabBody, &cabScoop, &cabPresence})
        for (auto& filter : *bank) filter.prepare(osSpec);
    // Coupling networks around the five 12AX7 gain stages. Single-pole DC
    // blockers remain numerically stable at the oversampled rate.
    constexpr std::array<float, 5> cutoffs { 7.2f, 15.4f, 10.5f, 24.0f, 7.2f };
    for (size_t i = 0; i < couplingPole.size(); ++i)
        couplingPole[i] = std::exp(-juce::MathConstants<float>::twoPi * cutoffs[i]
                                   / static_cast<float>(highRate));
    updateFilters(); reset();
}

void Vulcan5Effect::reset()
{
    if (oversampling) oversampling->reset();
    couplingInput = {}; couplingOutput = {}; supplyEnvelope.fill(0.0f);
    for (auto* bank : {&inputHighPass, &interstageLowPass1, &interstageLowPass2,
                       &toneLow, &toneMid, &toneHigh, &transformerHighPass,
                       &transformerLowPass, &resonanceShelf, &cabHighPass,
                       &cabLowPass, &cabBody, &cabScoop, &cabPresence})
        for (auto& filter : *bank) filter.reset();
}

void Vulcan5Effect::updateFilters()
{
    const auto bass = bassParam.load() * 0.01f, middle = middleParam.load() * 0.01f;
    const auto treble = trebleParam.load() * 0.01f, presence = presenceParam.load() * 0.01f;
    const auto resonance = resonanceParam.load() * 0.01f;
    cachedBass = bass; cachedMiddle = middle; cachedTreble = treble;
    cachedPresence = presence; cachedResonance = resonance;
    auto set = [] (auto& bank, const auto& coefficients) {
        for (auto& filter : bank) *filter.coefficients = *coefficients;
    };
    set(inputHighPass, Coefficients::makeFirstOrderHighPass(highRate, 7.2));
    // Plate bypass/compensation networks visible between V1B, V2A and V2B.
    set(interstageLowPass1, Coefficients::makeFirstOrderLowPass(highRate, 7200.0));
    set(interstageLowPass2, Coefficients::makeFirstOrderLowPass(highRate, 10800.0));
    set(toneLow, Coefficients::makeLowShelf(highRate, 105.0, 0.7,
        juce::Decibels::decibelsToGain(juce::jmap(bass, -12.0f, 8.0f))));
    set(toneMid, Coefficients::makePeakFilter(highRate, 620.0, 0.62,
        juce::Decibels::decibelsToGain(juce::jmap(middle, -14.0f, 6.0f))));
    set(toneHigh, Coefficients::makeHighShelf(highRate, 1900.0, 0.7,
        juce::Decibels::decibelsToGain(juce::jmap(treble, -11.0f, 10.0f))));
    set(transformerHighPass, Coefficients::makeFirstOrderHighPass(highRate, 30.0));
    set(transformerLowPass, Coefficients::makeLowPass(highRate,
        juce::jmap(presence, 4800.0f, 9800.0f), 0.72));
    set(resonanceShelf, Coefficients::makeLowShelf(highRate, 115.0, 0.72,
        juce::Decibels::decibelsToGain(juce::jmap(resonance, 0.0f, 8.5f))));
    set(cabHighPass, Coefficients::makeFirstOrderHighPass(highRate, 72.0));
    set(cabLowPass, Coefficients::makeLowPass(highRate, 5900.0, 0.72));
    set(cabBody, Coefficients::makePeakFilter(highRate, 125.0, 0.9, juce::Decibels::decibelsToGain(3.5f)));
    set(cabScoop, Coefficients::makePeakFilter(highRate, 680.0, 0.8, juce::Decibels::decibelsToGain(-4.5f)));
    set(cabPresence, Coefficients::makePeakFilter(highRate, 3600.0, 1.25, juce::Decibels::decibelsToGain(3.0f)));
}

float Vulcan5Effect::triode(float x, float bias, float hardness) noexcept
{
    const auto rest = std::tanh(bias * hardness);
    return (std::tanh((x + bias) * hardness) - rest)
         / juce::jmax(0.16f, 1.0f - std::abs(rest));
}

void Vulcan5Effect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassParam.load() >= 0.5f || !oversampling) return;
    const auto bass = bassParam.load() * 0.01f, middle = middleParam.load() * 0.01f;
    const auto treble = trebleParam.load() * 0.01f, presence = presenceParam.load() * 0.01f;
    const auto resonance = resonanceParam.load() * 0.01f;
    if (bass != cachedBass || middle != cachedMiddle || treble != cachedTreble
        || presence != cachedPresence || resonance != cachedResonance) updateFilters();

    juce::dsp::AudioBlock<float> base(buffer); auto block = oversampling->processSamplesUp(base);
    const auto channel = juce::jlimit(0, 2, juce::roundToInt(channelParam.load()));
    const auto gain = juce::jlimit(0.0f, 1.0f, gainParam.load() * 0.01f);
    const auto master = juce::jlimit(0.0f, 1.0f, masterParam.load() * 0.01f);
    const auto bias = juce::jlimit(0.0f, 1.0f, biasParam.load() * 0.01f);
    const auto sag = juce::jlimit(0.0f, 1.0f, sagParam.load() * 0.01f);
    const auto cab = cabEnabledParam.load() >= 0.5f
        && (cabSuppressedParam == nullptr || !cabSuppressedParam->load(std::memory_order_relaxed));
    const auto attack = std::exp(-1.0f / static_cast<float>(0.006 * highRate));
    const auto release = std::exp(-1.0f / static_cast<float>(0.24 * highRate));
    auto couple = [this] (float value, size_t stage, size_t channelIndex) noexcept {
        const auto result = value - couplingInput[stage][channelIndex]
                          + couplingPole[stage] * couplingOutput[stage][channelIndex];
        couplingInput[stage][channelIndex] = value;
        couplingOutput[stage][channelIndex] = result;
        return result;
    };
    for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
        auto* data = block.getChannelPointer(ch); const auto c = juce::jmin(ch, size_t{1});
        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            auto x = inputHighPass[c].processSample(data[i]);
            // Relays K2/K3 alter attenuation and bypass stages: Clean uses four
            // low-drive stages, Crunch raises their gain, Lead inserts V5B.
            const auto channelDrive = channel == 0 ? 0.48f : (channel == 1 ? 0.78f : 1.0f);
            x = triode(x * juce::jmap(gain, 1.5f, 8.5f) * channelDrive, 0.11f, 1.05f);
            x = couple(x, 0, c);
            x = interstageLowPass1[c].processSample(x);
            x = triode(x * juce::jmap(gain, 1.15f, 3.1f) * channelDrive, -0.18f, 1.18f);
            x = couple(x, 1, c);
            x = triode(x * (channel == 0 ? 0.78f : 1.85f), 0.07f, 1.12f);
            x = couple(x, 2, c);
            x = interstageLowPass2[c].processSample(x);
            x = triode(x * (channel == 0 ? 0.72f : 1.62f), -0.24f, 1.28f);
            x = couple(x, 3, c);
            if (channel == 2) x = triode(x * 1.72f, 0.05f, 1.2f);
            x = couple(x, 4, c);
            x = toneLow[c].processSample(x); x = toneMid[c].processSample(x); x = toneHigh[c].processSample(x);
            x *= std::pow(master, 1.35f) * 1.65f;
            const auto piA = triode(x * 1.65f, 0.025f, 1.08f);
            const auto piB = triode(-x * 1.65f, -0.025f, 1.08f);
            // Low factory idle current is represented by stronger crossover at
            // low Bias settings; raising Bias progressively softens the notch.
            // A literal dead zone behaves like a noise gate as a note decays.
            // Real push-pull valves retain finite small-signal transconductance;
            // model cold bias as a shallow, continuously differentiable gain
            // depression around zero instead of deleting quiet samples.
            const auto crossoverDepth = juce::jmap(bias, 0.075f, 0.008f);
            auto applyCrossover = [crossoverDepth] (float sample) noexcept {
                constexpr float crossoverWidth = 0.18f;
                const auto depression = crossoverDepth
                    * std::exp(-std::abs(sample) / crossoverWidth);
                return sample * (1.0f - depression);
            };
            auto powerA = applyCrossover(piA);
            auto powerB = applyCrossover(piB);
            auto pushPull = 0.5f * (triode(powerA * 2.25f, -0.03f, 1.28f)
                                  - triode(powerB * 2.25f, 0.03f, 1.28f));
            const auto draw = std::abs(pushPull);
            supplyEnvelope[c] = draw > supplyEnvelope[c]
                ? attack * supplyEnvelope[c] + (1.0f - attack) * draw
                : release * supplyEnvelope[c] + (1.0f - release) * draw;
            pushPull *= juce::jlimit(0.56f, 1.0f, 1.0f - sag * supplyEnvelope[c] * 0.36f);
            x = transformerHighPass[c].processSample(pushPull);
            x = resonanceShelf[c].processSample(x);
            x = transformerLowPass[c].processSample(x);
            if (cab) { x = cabHighPass[c].processSample(x); x = cabBody[c].processSample(x);
                x = cabScoop[c].processSample(x); x = cabPresence[c].processSample(x);
                x = cabLowPass[c].processSample(x); }
            data[i] = x * 0.68f;
        }
    }
    oversampling->processSamplesDown(base);
}

ImpulseCabEffect::ImpulseCabEffect(std::atomic<float>& level, std::atomic<float>& lowCut,
    std::atomic<float>& highCut, std::atomic<float>& mix, std::atomic<float>& bypass)
    : levelParam(level), lowCutParam(lowCut), highCutParam(highCut),
      mixParam(mix), bypassParam(bypass) {}

void ImpulseCabEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    convolution.prepare(spec);
    lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highCutFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lowCutFilter.prepare(spec); highCutFilter.prepare(spec);
    dryBuffer.setSize(static_cast<int>(spec.numChannels), static_cast<int>(spec.maximumBlockSize), false, false, true);
    reset();
}

void ImpulseCabEffect::reset()
{
    convolution.reset(); lowCutFilter.reset(); highCutFilter.reset(); dryBuffer.clear();
}

void ImpulseCabEffect::loadImpulse(const juce::File& file)
{
    if (!file.existsAsFile()) return;
    impulseLoaded.store(false, std::memory_order_release);
    convolution.loadImpulseResponse(file, juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::yes, 0, juce::dsp::Convolution::Normalise::yes);
    impulseLoaded.store(true, std::memory_order_release);
}

void ImpulseCabEffect::loadImpulse(juce::AudioBuffer<float>&& buffer, double sampleRate)
{
    impulseLoaded.store(false, std::memory_order_release);
    convolution.loadImpulseResponse(std::move(buffer), sampleRate,
        juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::yes,
        juce::dsp::Convolution::Normalise::yes);
    impulseLoaded.store(true, std::memory_order_release);
}

void ImpulseCabEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassParam.load() >= 0.5f || !hasImpulse()) return;
    const auto channels = juce::jmin(buffer.getNumChannels(), dryBuffer.getNumChannels());
    const auto samples = juce::jmin(buffer.getNumSamples(), dryBuffer.getNumSamples());
    for (int ch = 0; ch < channels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, samples);
    lowCutFilter.setCutoffFrequency(juce::jlimit(20.0f, 500.0f, lowCutParam.load()));
    highCutFilter.setCutoffFrequency(juce::jlimit(2000.0f, 22000.0f, highCutParam.load()));
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowCutFilter.process(context); highCutFilter.process(context); convolution.process(context);
    const auto wet = juce::jlimit(0.0f, 1.0f, mixParam.load() * 0.01f);
    const auto level = juce::Decibels::decibelsToGain(levelParam.load());
    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < samples; ++i) {
            const auto dry = dryBuffer.getSample(ch, i);
            buffer.setSample(ch, i, dry + (buffer.getSample(ch, i) * level - dry) * wet);
        }
}

namespace
{
double modelSampleRate(const nam::DSP& model)
{
    const auto reported = model.GetExpectedSampleRate();
    return reported > 0.0 ? reported : 48000.0;
}

class ResampledNamModel
{
public:
    ResampledNamModel(std::unique_ptr<nam::DSP> source, double hostRate, int maximumBlockSize)
        : model(std::move(source)), resampler(modelSampleRate(*model))
    {
        processFunction = [this] (float** input, float** output, int frames) {
            model->process(input, output, frames);
        };
        reset(hostRate, maximumBlockSize);
    }

    void reset(double hostRate, int maximumBlockSize)
    {
        hostSampleRate = hostRate;
        const auto ratio = hostRate / modelSampleRate(*model);
        const auto modelBlockSize = juce::jmax(1, static_cast<int>(std::ceil(maximumBlockSize / ratio)));
        model->Reset(modelSampleRate(*model), modelBlockSize);
        resampler.Reset(hostRate, maximumBlockSize);
    }

    void process(float* input, float* output, int frames)
    {
        float* inputs[] { input };
        float* outputs[] { output };
        if (modelSampleRate(*model) == hostSampleRate)
            model->process(inputs, outputs, frames);
        else
            resampler.ProcessBlock(inputs, outputs, frames, processFunction);
    }

private:
    std::unique_ptr<nam::DSP> model;
    dsp::ResamplingContainer<float, 1, 12> resampler;
    std::function<void(float**, float**, int)> processFunction;
    double hostSampleRate = 0.0;
};
}

struct ModelerEffect::Impl
{
    struct StereoModel
    {
        std::array<std::unique_ptr<ResampledNamModel>, 2> channels;
        juce::String name;
    };

    std::vector<std::unique_ptr<StereoModel>> models;
    std::atomic<StereoModel*> active { nullptr };
    std::atomic<StereoModel*> pending { nullptr };
    std::array<std::vector<float>, 2> inputScratch, outputScratch;
    double sampleRate = 48000.0;
    int maximumBlockSize = 512;
};

ModelerEffect::ModelerEffect(std::atomic<float>& input, std::atomic<float>& output,
    std::atomic<float>& mix, std::atomic<float>& bypass)
    : impl(std::make_unique<Impl>()), inputParam(input), outputParam(output),
      mixParam(mix), bypassParam(bypass) {}

ModelerEffect::~ModelerEffect() = default;

void ModelerEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    impl->sampleRate = spec.sampleRate;
    impl->maximumBlockSize = static_cast<int>(spec.maximumBlockSize);
    for (auto& buffer : impl->inputScratch) buffer.resize(spec.maximumBlockSize);
    for (auto& buffer : impl->outputScratch) buffer.resize(spec.maximumBlockSize);
    for (auto& stereo : impl->models)
        for (auto& channel : stereo->channels) {
            channel->reset(spec.sampleRate, impl->maximumBlockSize);
        }
}

void ModelerEffect::reset()
{
    if (auto* model = impl->active.load(std::memory_order_acquire))
        for (auto& channel : model->channels)
            channel->reset(impl->sampleRate, impl->maximumBlockSize);
}

bool ModelerEffect::loadModel(const juce::File& file, juce::String& error)
{
    if (!file.existsAsFile()) { error = "The selected NAM model does not exist."; return false; }
    try {
        const auto path = std::filesystem::u8path(file.getFullPathName().toStdString());
        auto stereo = std::make_unique<Impl::StereoModel>();
        for (auto& channel : stereo->channels) {
            auto model = nam::get_dsp(path, nam::DspLoadOptions { false });
            if (model->NumInputChannels() != 1 || model->NumOutputChannels() != 1)
                throw std::runtime_error("MODELER supports mono-input, mono-output NAM files.");
            channel = std::make_unique<ResampledNamModel>(std::move(model), impl->sampleRate,
                                                          impl->maximumBlockSize);
        }
        stereo->name = file.getFileNameWithoutExtension();
        auto* ready = stereo.get();
        impl->models.push_back(std::move(stereo));
        impl->pending.store(ready, std::memory_order_release);
        return true;
    } catch (const std::exception& exception) {
        error = exception.what();
        return false;
    }
}

juce::String ModelerEffect::getModelName() const
{
    if (auto* pending = impl->pending.load(std::memory_order_acquire)) return pending->name;
    if (auto* active = impl->active.load(std::memory_order_acquire)) return active->name;
    return "NO NAM LOADED";
}

void ModelerEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (auto* ready = impl->pending.exchange(nullptr, std::memory_order_acq_rel))
        impl->active.store(ready, std::memory_order_release);
    auto* model = impl->active.load(std::memory_order_acquire);
    if (model == nullptr || bypassParam.load() >= 0.5f) return;

    const auto samples = juce::jmin(buffer.getNumSamples(), impl->maximumBlockSize);
    const auto channels = juce::jmin(buffer.getNumChannels(), 2);
    const auto inputGain = juce::Decibels::decibelsToGain(inputParam.load());
    const auto outputGain = juce::Decibels::decibelsToGain(outputParam.load());
    const auto wet = juce::jlimit(0.0f, 1.0f, mixParam.load() * 0.01f);

    if (linkedChannels && channels > 0) {
        auto* input = impl->inputScratch[0].data();
        auto* output = impl->outputScratch[0].data();
        juce::FloatVectorOperations::copyWithMultiply(
            input, buffer.getReadPointer(0), inputGain, samples);
        model->channels[0]->process(input, output, samples);
        for (int channel = 0; channel < channels; ++channel) {
            auto* audio = buffer.getWritePointer(channel);
            for (int sample = 0; sample < samples; ++sample)
                audio[sample] += (output[sample] * outputGain - audio[sample]) * wet;
        }
        return;
    }

    for (int channel = 0; channel < channels; ++channel) {
        auto* audio = buffer.getWritePointer(channel);
        auto* input = impl->inputScratch[static_cast<size_t>(channel)].data();
        auto* output = impl->outputScratch[static_cast<size_t>(channel)].data();
        juce::FloatVectorOperations::copyWithMultiply(input, audio, inputGain, samples);
        model->channels[static_cast<size_t>(channel)]->process(input, output, samples);
        for (int sample = 0; sample < samples; ++sample)
            audio[sample] += (output[sample] * outputGain - audio[sample]) * wet;
    }
}

Ceres2Effect::Ceres2Effect(std::atomic<float>& rate, std::atomic<float>& depth,
                            std::atomic<float>& mix, std::atomic<float>& bypass)
    : rateParam(rate), depthParam(depth), mixParam(mix), bypassParam(bypass) {}

void Ceres2Effect::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    auto monoSpec = spec; monoSpec.numChannels = 1;
    for (auto* bank : {&inputHighPass, &antiAlias1, &antiAlias2, &antiAlias3,
                       &reconstruction1, &reconstruction2, &reconstruction3})
        for (auto& filter : *bank) filter.prepare(monoSpec);
    auto set = [] (auto& bank, const auto& coefficients) {
        for (auto& filter : bank) *filter.coefficients = *coefficients;
    };
    set(inputHighPass, Coefficients::makeFirstOrderHighPass(sampleRate, 22.0));
    // The three 10k/capacitor sections around the 1024-stage BBD form the
    // anti-alias and reconstruction response. Their schematic corner values
    // are approximately 4.8 kHz, 1.94 kHz and 4.8 kHz.
    set(antiAlias1, Coefficients::makeFirstOrderLowPass(sampleRate, 4820.0));
    set(antiAlias2, Coefficients::makeFirstOrderLowPass(sampleRate, 1940.0));
    set(antiAlias3, Coefficients::makeFirstOrderLowPass(sampleRate, 4820.0));
    set(reconstruction1, Coefficients::makeFirstOrderLowPass(sampleRate, 4820.0));
    set(reconstruction2, Coefficients::makeFirstOrderLowPass(sampleRate, 1940.0));
    set(reconstruction3, Coefficients::makeFirstOrderLowPass(sampleRate, 4820.0));
    const auto capacity = static_cast<size_t>(std::ceil(sampleRate * 0.04)) + 4;
    for (auto& channel : bbdBuffer) channel.assign(capacity, 0.0f);
    reset();
}

void Ceres2Effect::reset()
{
    for (auto* bank : {&inputHighPass, &antiAlias1, &antiAlias2, &antiAlias3,
                       &reconstruction1, &reconstruction2, &reconstruction3})
        for (auto& filter : *bank) filter.reset();
    for (auto& channel : bbdBuffer) std::fill(channel.begin(), channel.end(), 0.0f);
    writePosition.fill(0); lfoPhase = 0.0f;
}

void Ceres2Effect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassParam.load() >= 0.5f || bbdBuffer[0].empty()) return;
    const auto rate = juce::jmap(rateParam.load() * 0.01f, 0.25f, 3.2f);
    const auto depth = juce::jlimit(0.0f, 1.0f, depthParam.load() * 0.01f);
    const auto mix = juce::jlimit(0.0f, 1.0f, mixParam.load() * 0.01f);
    const auto phaseStep = rate / static_cast<float>(sampleRate);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        // Triangle motion reproduces the integrator/comparator LFO topology.
        const auto triangle = 1.0f - 4.0f * std::abs(lfoPhase - 0.5f);
        const auto delaySeconds = 0.0046f + triangle * depth * 0.0018f;
        const auto delaySamples = juce::jlimit(1.0f, static_cast<float>(bbdBuffer[0].size() - 3),
                                               delaySeconds * static_cast<float>(sampleRate));
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const auto c = static_cast<size_t>(juce::jmin(ch, 1));
            auto& ring = bbdBuffer[c]; const auto write = writePosition[c];
            const auto dry = buffer.getSample(ch, i);
            auto input = inputHighPass[c].processSample(dry);
            input = antiAlias1[c].processSample(input);
            input = antiAlias2[c].processSample(input);
            input = antiAlias3[c].processSample(input);
            // Limited charge transfer and finite BBD signal range.
            ring[write] = std::round(std::tanh(input * 1.35f) * 2048.0f) / 2048.0f;
            auto readPosition = static_cast<float>(write) - delaySamples;
            while (readPosition < 0.0f) readPosition += static_cast<float>(ring.size());
            const auto index0 = static_cast<size_t>(readPosition) % ring.size();
            const auto index1 = (index0 + 1) % ring.size();
            const auto fraction = readPosition - std::floor(readPosition);
            auto wet = ring[index0] + (ring[index1] - ring[index0]) * fraction;
            wet = reconstruction1[c].processSample(wet);
            wet = reconstruction2[c].processSample(wet);
            wet = reconstruction3[c].processSample(wet);
            buffer.setSample(ch, i, dry + (wet - dry) * mix);
            writePosition[c] = (write + 1) % ring.size();
        }
        lfoPhase += phaseStep;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
    }
}

VoidChamberEffect::VoidChamberEffect(std::atomic<float>& size, std::atomic<float>& damping,
                           std::atomic<float>& mix, std::atomic<float>& bypass)
    : sizeParam(size), dampingParam(damping), mixParam(mix), bypassParam(bypass) {}

void VoidChamberEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    reverb.prepare(spec); reset();
}

void VoidChamberEffect::reset() { reverb.reset(); }

void VoidChamberEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassParam.load() >= 0.5f) return;
    juce::dsp::Reverb::Parameters parameters;
    parameters.roomSize = juce::jlimit(0.0f, 1.0f, sizeParam.load() * 0.01f);
    parameters.damping = juce::jlimit(0.0f, 1.0f, dampingParam.load() * 0.01f);
    parameters.wetLevel = juce::jlimit(0.0f, 1.0f, mixParam.load() * 0.01f);
    parameters.dryLevel = 1.0f - parameters.wetLevel;
    parameters.width = 1.0f;
    reverb.setParameters(parameters);
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
}

PulsarEffect::PulsarEffect(std::atomic<float>& time, std::atomic<float>& feedback,
                         std::atomic<float>& mix, std::atomic<float>& bypass)
    : timeParam(time), feedbackParam(feedback), mixParam(mix), bypassParam(bypass) {}

void PulsarEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;
    const auto capacity = static_cast<size_t>(std::ceil(sampleRate * 2.0)) + 1;
    for (auto& channel : delayBuffer) channel.assign(capacity, 0.0f);
    reset();
}

void PulsarEffect::reset()
{
    for (auto& channel : delayBuffer) std::fill(channel.begin(), channel.end(), 0.0f);
    writePosition.fill(0);
}

void PulsarEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassParam.load() >= 0.5f || delayBuffer[0].empty()) return;
    const auto delaySamples = static_cast<size_t>(juce::jlimit(1.0, sampleRate * 1.95,
        sampleRate * juce::jmap(timeParam.load() * 0.01, 0.04, 1.2)));
    const auto feedback = juce::jlimit(0.0f, 0.92f, feedbackParam.load() * 0.0092f);
    const auto mix = juce::jlimit(0.0f, 1.0f, mixParam.load() * 0.01f);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const auto c = static_cast<size_t>(juce::jmin(ch, 1));
        auto& ring = delayBuffer[c]; auto write = writePosition[c];
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const auto read = (write + ring.size() - delaySamples) % ring.size();
            const auto dry = buffer.getSample(ch, i); const auto wet = ring[read];
            ring[write] = dry + wet * feedback;
            buffer.setSample(ch, i, dry + (wet - dry) * mix);
            write = (write + 1) % ring.size();
        }
        writePosition[c] = write;
    }
}

LunerEffect::LunerEffect(std::atomic<float>& bypass) : bypassParam(bypass) {}

void LunerEffect::prepare(const juce::dsp::ProcessSpec& spec)
{
    analysisSampleRate = spec.sampleRate * 0.5;
    analysisBuffer.assign(2048, 0.0f);
    const auto maximumLag = static_cast<size_t>(std::ceil(analysisSampleRate / 65.0));
    correlationScores.assign(maximumLag + 1, 0.0f);
    reset();
}

void LunerEffect::reset()
{
    std::fill(analysisBuffer.begin(), analysisBuffer.end(), 0.0f);
    std::fill(correlationScores.begin(), correlationScores.end(), 0.0f);
    writePosition = 0; decimationAccumulator = 0.0f; decimationPhase = 0;
    midiNote.store(-1); cents.store(0.0f); frequency.store(0.0f); confidence.store(0.0f);
}

void LunerEffect::process(juce::AudioBuffer<float>& buffer)
{
    if (bypassParam.load() >= 0.5f || analysisBuffer.empty()) return;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        auto mono = 0.0f;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) mono += buffer.getSample(ch, i);
        mono /= static_cast<float>(juce::jmax(1, buffer.getNumChannels()));
        decimationAccumulator += mono;
        if (++decimationPhase == 2) {
            analysisBuffer[writePosition++] = decimationAccumulator * 0.5f;
            decimationAccumulator = 0.0f; decimationPhase = 0;
            if (writePosition == analysisBuffer.size()) {
                analyse();
                writePosition = 0;
            }
        }
    }
}

void LunerEffect::analyse() noexcept
{
    double energy = 0.0, mean = 0.0;
    for (const auto sample : analysisBuffer) mean += sample;
    mean /= static_cast<double>(analysisBuffer.size());
    for (const auto sample : analysisBuffer) {
        const auto centred = static_cast<double>(sample) - mean;
        energy += centred * centred;
    }
    const auto rms = std::sqrt(energy / static_cast<double>(analysisBuffer.size()));
    if (rms < 0.0015) {
        midiNote.store(-1); frequency.store(0.0f); confidence.store(0.0f); return;
    }

    const auto minimumLag = static_cast<size_t>(analysisSampleRate / 1200.0);
    const auto maximumLag = juce::jmin(correlationScores.size() - 1,
        static_cast<size_t>(analysisSampleRate / 65.0));
    float bestScore = -1.0f; size_t bestLag = minimumLag;
    for (size_t lag = minimumLag; lag <= maximumLag; ++lag) {
        double correlation = 0.0, energyA = 0.0, energyB = 0.0;
        for (size_t i = 0; i + lag < analysisBuffer.size(); ++i) {
            const auto a = static_cast<double>(analysisBuffer[i]) - mean;
            const auto b = static_cast<double>(analysisBuffer[i + lag]) - mean;
            correlation += a * b; energyA += a * a; energyB += b * b;
        }
        const auto score = static_cast<float>(correlation / std::sqrt(energyA * energyB + 1.0e-18));
        correlationScores[lag] = score;
        if (score > bestScore) { bestScore = score; bestLag = lag; }
    }
    if (bestScore < 0.55f) {
        midiNote.store(-1); frequency.store(0.0f); confidence.store(bestScore); return;
    }

    // Prefer a plausible lower fundamental when its correlation is almost as
    // strong as an overtone peak.
    for (size_t multiple = 2; multiple <= 4; ++multiple) {
        const auto candidate = bestLag * multiple;
        if (candidate <= maximumLag && correlationScores[candidate] > bestScore * 0.9f)
            bestLag = candidate;
    }
    auto refinedLag = static_cast<float>(bestLag);
    if (bestLag > minimumLag && bestLag < maximumLag) {
        const auto left = correlationScores[bestLag - 1], centre = correlationScores[bestLag];
        const auto right = correlationScores[bestLag + 1];
        const auto denominator = left - 2.0f * centre + right;
        if (std::abs(denominator) > 1.0e-6f)
            refinedLag += 0.5f * (left - right) / denominator;
    }
    const auto detectedFrequency = static_cast<float>(analysisSampleRate) / refinedLag;
    const auto noteValue = 69.0f + 12.0f * std::log2(detectedFrequency / 440.0f);
    const auto nearestNote = static_cast<int>(std::round(noteValue));
    frequency.store(detectedFrequency); midiNote.store(nearestNote);
    cents.store(juce::jlimit(-50.0f, 50.0f, (noteValue - static_cast<float>(nearestNote)) * 100.0f));
    confidence.store(juce::jlimit(0.0f, 1.0f, bestScore));
}
