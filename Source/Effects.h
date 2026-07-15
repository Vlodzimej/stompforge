#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

class EffectModule
{
public:
    virtual ~EffectModule() = default;
    virtual void prepare(const juce::dsp::ProcessSpec&) = 0;
    virtual void reset() = 0;
    virtual void process(juce::AudioBuffer<float>&) = 0;
};

class GateEffect final : public EffectModule
{
public:
    GateEffect(std::atomic<float>& threshold, std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override;
    void reset() override;
    void process(juce::AudioBuffer<float>&) override;

private:
    std::atomic<float>& threshold;
    std::atomic<float>& bypass;
    juce::LinearSmoothedValue<float> gain;
};

class DriveEffect final : public EffectModule
{
public:
    DriveEffect(std::atomic<float>& drive, std::atomic<float>& mix, std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override {}
    void reset() override {}
    void process(juce::AudioBuffer<float>&) override;

private:
    std::atomic<float>& drive;
    std::atomic<float>& mix;
    std::atomic<float>& bypass;
};

class ToneEffect final : public EffectModule
{
public:
    ToneEffect(std::atomic<float>& bass, std::atomic<float>& mid, std::atomic<float>& treble,
               std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override;
    void reset() override;
    void process(juce::AudioBuffer<float>&) override;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    std::atomic<float>& bass;
    std::atomic<float>& mid;
    std::atomic<float>& treble;
    std::atomic<float>& bypass;
    std::array<Filter, 2> lowShelf, midPeak, highShelf;
    double sampleRate = 44100.0;
    void updateFilters();
};
