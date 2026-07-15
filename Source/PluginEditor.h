#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class StompForgeAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               public juce::DragAndDropContainer
{
public:
    explicit StompForgeAudioProcessorEditor(StompForgeAudioProcessor&);
    ~StompForgeAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class PedalCard;
    StompForgeAudioProcessor& processor;
    std::array<std::unique_ptr<PedalCard>, 3> pedals;
    juce::Slider outputKnob;
    juce::Label outputLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> outputAttachment;

    void layoutPedals();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StompForgeAudioProcessorEditor)
};
