#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class StompForgeAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                               public juce::DragAndDropContainer,
                                               private juce::Timer
{
public:
    explicit StompForgeAudioProcessorEditor(StompForgeAudioProcessor&);
    ~StompForgeAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    class PedalCard;
    class GridCell;
    StompForgeAudioProcessor& processor;
    std::array<std::unique_ptr<PedalCard>, StompForgeAudioProcessor::numEffects> pedals;
    std::array<std::unique_ptr<GridCell>, StompForgeAudioProcessor::numSlots> gridCells;
    juce::Slider inputKnob, outputKnob;
    juce::Label inputLabel, outputLabel;
    juce::TextButton gridButton;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> inputAttachment, outputAttachment;

    void layoutPedals();
    juce::Rectangle<int> getGridCellBounds(int slot) const;
    void showEffectMenu(int slot);
    void showGridSelector();
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StompForgeAudioProcessorEditor)
};
