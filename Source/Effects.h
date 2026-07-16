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

// Real-time, circuit-inspired model of the DEIMOS-1 signal path. The
// analogue stages are represented separately so they can be refined or
// replaced by measured/WDF models without changing the pedalboard API.
class Deimos1Effect final : public EffectModule
{
public:
    Deimos1Effect(std::atomic<float>& distortion, std::atomic<float>& tone,
                        std::atomic<float>& level, std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override;
    void reset() override;
    void process(juce::AudioBuffer<float>&) override;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    std::atomic<float>& distortion;
    std::atomic<float>& tone;
    std::atomic<float>& level;
    std::atomic<float>& bypass;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::array<Filter, 2> inputHighPass, transistorLowPass, opAmpHighPass,
                          clippingLowPass, toneLowPass, toneHighPass, dcBlock;
    double oversampledRate = 176400.0;
    void updateFilters();
    static float siliconDiodePair(float sample) noexcept;
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

class Mars8Effect final : public EffectModule
{
public:
    Mars8Effect(std::atomic<float>& preamp, std::atomic<float>& bass,
                 std::atomic<float>& middle, std::atomic<float>& treble,
                 std::atomic<float>& master, std::atomic<float>& presence,
                 std::atomic<float>& sag, std::atomic<float>& cabEnabled,
                 std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override;
    void reset() override;
    void process(juce::AudioBuffer<float>&) override;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    std::atomic<float>& preampParam; std::atomic<float>& bassParam;
    std::atomic<float>& middleParam; std::atomic<float>& trebleParam;
    std::atomic<float>& masterParam; std::atomic<float>& presenceParam;
    std::atomic<float>& sagParam; std::atomic<float>& cabEnabledParam;
    std::atomic<float>& bypassParam;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::array<Filter, 2> inputHighPass, brightHighPass, toneLow, toneMid,
                          toneHigh, transformerHighPass, transformerLowPass,
                          cabHighPass, cabLowPass, cabLowBody, cabMidScoop,
                          cabPresence, cabTopNotch;
    std::array<float, 2> supplyEnvelope { 0.0f, 0.0f };
    std::array<std::array<float, 2>, 3> couplingInput {}, couplingOutput {};
    std::array<float, 3> couplingPole {};
    double highRate = 176400.0;
    float cachedBass = -1.0f, cachedMiddle = -1.0f, cachedTreble = -1.0f, cachedPresence = -1.0f;
    void updateFilters();
    static float triode(float x, float bias, float hardness) noexcept;
};

class Ceres2Effect final : public EffectModule
{
public:
    Ceres2Effect(std::atomic<float>& rate, std::atomic<float>& depth,
                 std::atomic<float>& mix, std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override;
    void reset() override;
    void process(juce::AudioBuffer<float>&) override;
private:
    std::atomic<float>& rateParam; std::atomic<float>& depthParam;
    std::atomic<float>& mixParam; std::atomic<float>& bypassParam;
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;
    std::array<Filter, 2> inputHighPass, antiAlias1, antiAlias2, antiAlias3,
                          reconstruction1, reconstruction2, reconstruction3;
    std::array<std::vector<float>, 2> bbdBuffer;
    std::array<size_t, 2> writePosition {};
    double sampleRate = 44100.0;
    float lfoPhase = 0.0f;
};

class ReverbEffect final : public EffectModule
{
public:
    ReverbEffect(std::atomic<float>& size, std::atomic<float>& damping,
                 std::atomic<float>& mix, std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override;
    void reset() override;
    void process(juce::AudioBuffer<float>&) override;
private:
    std::atomic<float>& sizeParam; std::atomic<float>& dampingParam;
    std::atomic<float>& mixParam; std::atomic<float>& bypassParam;
    juce::dsp::Reverb reverb;
};

class DelayEffect final : public EffectModule
{
public:
    DelayEffect(std::atomic<float>& time, std::atomic<float>& feedback,
                std::atomic<float>& mix, std::atomic<float>& bypass);
    void prepare(const juce::dsp::ProcessSpec&) override;
    void reset() override;
    void process(juce::AudioBuffer<float>&) override;
private:
    std::atomic<float>& timeParam; std::atomic<float>& feedbackParam;
    std::atomic<float>& mixParam; std::atomic<float>& bypassParam;
    std::array<std::vector<float>, 2> delayBuffer;
    std::array<size_t, 2> writePosition {};
    double sampleRate = 44100.0;
};
