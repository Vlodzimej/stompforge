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
