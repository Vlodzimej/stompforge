#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Effects.h"

class StompForgeAudioProcessor final : public juce::AudioProcessor
{
public:
    StompForgeAudioProcessor();
    ~StompForgeAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

    enum class PedalId : int { gate = 0, ds1, tone, jcm800 };
    std::array<PedalId, 3> getPedalOrder() const noexcept;
    void movePedal(PedalId dragged, int targetSlot);
    void replacePedal(int slot, PedalId replacement);

private:
    std::array<std::unique_ptr<EffectModule>, 4> effects;
    std::atomic<juce::uint32> packedOrder { 0x24u };
    juce::LinearSmoothedValue<float> outputGain;
    static juce::uint32 packOrder(const std::array<PedalId, 3>&) noexcept;
    void setPedalOrder(const std::array<PedalId, 3>&, bool saveToState);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StompForgeAudioProcessor)
};
