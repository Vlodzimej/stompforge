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
    class LevelFader;
    class TouchMenuLookAndFeel;
    StompForgeAudioProcessor& processor;
    std::array<std::unique_ptr<PedalCard>, StompForgeAudioProcessor::numEffects> pedals;
    std::array<std::unique_ptr<GridCell>, StompForgeAudioProcessor::numSlots> gridCells;
    std::unique_ptr<LevelFader> inputFader, outputFader;
    juce::Label inputLabel, outputLabel;
    juce::TextButton gridButton, bufferButton;
    std::unique_ptr<TouchMenuLookAndFeel> touchMenuLookAndFeel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> inputAttachment, outputAttachment;
    juce::int64 lastPersistedStateHash = 0;
    int persistenceTimerTicks = 0;

    void layoutPedals();
    void updateResizeLimitsForGrid(bool growIfNeeded);
    juce::Rectangle<int> getGridCellBounds(int slot) const;
    void showEffectMenu(int slot);
    void showGridSelector();
    void persistStandaloneStateIfChanged(bool force);
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StompForgeAudioProcessorEditor)
};
