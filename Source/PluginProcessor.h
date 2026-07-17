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

    enum class PedalId : int { gate = 0, ds1, tone, jcm800, chorus, reverb, delay, tuner, amp5150, impulseCab, modeler, empty = 15 };
    static constexpr size_t numSlots = 12;
    static constexpr size_t numEffects = 11;
    struct TunerState { int midiNote = -1; float cents = 0.0f; float frequency = 0.0f; float confidence = 0.0f; };
    TunerState getTunerState() const noexcept;
    std::array<PedalId, numSlots> getPedalOrder() const noexcept;
    void movePedal(int sourceSlot, int targetSlot);
    void replacePedal(int slot, PedalId replacement);
    void clearPedal(int slot);
    bool loadCabImpulse(const juce::File& source);
    bool loadCabImpulse(const juce::URL& source);
    juce::String getCabImpulseName() const;
    bool loadModelerModel(const juce::File& source, juce::String& error);
    bool loadModelerModel(const juce::URL& source, juce::String& error);
    juce::String getModelerName() const;
    int getGridRows() const noexcept;
    int getGridColumns() const noexcept;
    bool setGridSize(int rows, int columns);
    bool isImpulseActive() const noexcept { return impulseActive.load(std::memory_order_acquire); }
    float consumeInputLevel() noexcept { return inputLevel.exchange(0.0f, std::memory_order_relaxed); }
    float consumeOutputLevel() noexcept { return outputLevel.exchange(0.0f, std::memory_order_relaxed); }

private:
    std::array<std::unique_ptr<EffectModule>, numEffects> effects;
    std::atomic<bool> impulseActive { false };
    LunerEffect* luner = nullptr;
    ImpulseCabEffect* impulseCab = nullptr;
    ModelerEffect* modeler = nullptr;
    std::atomic<juce::uint64> packedOrder { 0u };
    juce::LinearSmoothedValue<float> inputGain, outputGain;
    std::atomic<float> inputLevel { 0.0f }, outputLevel { 0.0f };
    static void publishLevel(std::atomic<float>& destination, float level) noexcept;
    static juce::uint64 packOrder(const std::array<PedalId, numSlots>&) noexcept;
    void setPedalOrder(const std::array<PedalId, numSlots>&, bool saveToState);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StompForgeAudioProcessor)
};
