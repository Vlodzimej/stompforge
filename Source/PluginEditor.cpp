#include "PluginEditor.h"

namespace
{
void configureKnob(juce::Slider& knob, juce::Colour colour)
{
    knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 18);
    knob.setColour(juce::Slider::rotarySliderFillColourId, colour.brighter(0.35f));
    knob.setColour(juce::Slider::thumbColourId, juce::Colour(0xffffd88a));
    knob.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    knob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0x45000000));
}
}

class GuitarForgeAudioProcessorEditor::PedalCard final : public juce::Component,
                                                          public juce::DragAndDropTarget
{
public:
    PedalCard(GuitarForgeAudioProcessorEditor& owner,
              GuitarForgeAudioProcessor::PedalId pedalId,
              juce::String pedalName, juce::Colour pedalColour,
              std::initializer_list<const char*> parameterIds,
              std::initializer_list<const char*> parameterNames,
              const char* bypassId)
        : editor(owner), id(pedalId), name(std::move(pedalName)), colour(pedalColour)
    {
        const std::vector<const char*> ids(parameterIds);
        const std::vector<const char*> names(parameterNames);
        for (size_t i = 0; i < ids.size(); ++i) {
            auto knob = std::make_unique<juce::Slider>();
            configureKnob(*knob, colour);
            if (juce::String(ids[i]) == "mix") knob->setTextValueSuffix(" %");
            else knob->setTextValueSuffix(" dB");
            addAndMakeVisible(*knob);
            attachments.push_back(std::make_unique<SliderAttachment>(editor.processor.parameters, ids[i], *knob));
            knobs.push_back(std::move(knob));

            auto label = std::make_unique<juce::Label>();
            label->setText(names[i], juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setColour(juce::Label::textColourId, juce::Colour(0xfff8efd8));
            addAndMakeVisible(*label);
            labels.push_back(std::move(label));
        }

        bypass.setButtonText("BYPASS");
        bypass.setClickingTogglesState(true);
        bypass.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff202329));
        bypass.setColour(juce::TextButton::buttonColourId, colour.darker(0.25f));
        addAndMakeVisible(bypass);
        bypassAttachment = std::make_unique<ButtonAttachment>(editor.processor.parameters, bypassId, bypass);
    }

    bool isInterestedInDragSource(const SourceDetails& details) override
    {
        return details.description.toString().startsWith("pedal:");
    }

    void itemDragEnter(const SourceDetails&) override { dragOver = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dragOver = false; repaint(); }
    void itemDropped(const SourceDetails& details) override
    {
        dragOver = false;
        const auto dragged = static_cast<GuitarForgeAudioProcessor::PedalId>(
            details.description.toString().fromFirstOccurrenceOf(":", false, false).getIntValue());
        editor.processor.movePedal(dragged, slot);
        editor.layoutPedals();
        repaint();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        canDrag = event.y < 48;
    }

    void mouseDrag(const juce::MouseEvent&) override
    {
        if (canDrag && !editor.isDragAndDropActive())
            editor.startDragging("pedal:" + juce::String(static_cast<int>(id)), this);
    }

    void setSlot(int newSlot) { slot = newSlot; }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(juce::Colour(0x66000000));
        g.fillRoundedRectangle(bounds.translated(0.0f, 5.0f), 13.0f);
        g.setColour(dragOver ? colour.brighter(0.35f) : colour);
        g.fillRoundedRectangle(bounds, 13.0f);
        g.setColour(colour.brighter(0.4f));
        g.drawRoundedRectangle(bounds, 13.0f, dragOver ? 4.0f : 1.5f);
        g.setColour(juce::Colour(0xfff8efd8));
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
        g.drawText(name, 12, 8, getWidth() - 24, 28, juce::Justification::centred);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("≡  DRAG TO REORDER", 12, 34, getWidth() - 24, 16, juce::Justification::centred);
        g.setColour(juce::Colour(0xaa17191e));
        g.fillRoundedRectangle(14.0f, static_cast<float>(getHeight() - 72),
                               static_cast<float>(getWidth() - 28), 56.0f, 7.0f);
    }

    void resized() override
    {
        auto controls = getLocalBounds().reduced(10).withTrimmedTop(48).withTrimmedBottom(76);
        const auto cellWidth = controls.getWidth() / static_cast<int>(knobs.size());
        for (size_t i = 0; i < knobs.size(); ++i) {
            auto cell = controls.removeFromLeft(cellWidth);
            labels[i]->setBounds(cell.removeFromTop(20));
            knobs[i]->setBounds(cell.reduced(1));
        }
        bypass.setBounds(getWidth() / 2 - 37, getHeight() - 62, 74, 38);
    }

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    GuitarForgeAudioProcessorEditor& editor;
    GuitarForgeAudioProcessor::PedalId id;
    juce::String name;
    juce::Colour colour;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    juce::TextButton bypass;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    int slot = 0;
    bool canDrag = false;
    bool dragOver = false;
};

GuitarForgeAudioProcessorEditor::GuitarForgeAudioProcessorEditor(GuitarForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    pedals[0] = std::make_unique<PedalCard>(*this, GuitarForgeAudioProcessor::PedalId::gate,
        "NOISE GATE", juce::Colour(0xff287f78), std::initializer_list<const char*>{"gate"},
        std::initializer_list<const char*>{"THRESHOLD"}, "gateBypass");
    pedals[1] = std::make_unique<PedalCard>(*this, GuitarForgeAudioProcessor::PedalId::drive,
        "DRIVE", juce::Colour(0xffd76525), std::initializer_list<const char*>{"drive", "mix"},
        std::initializer_list<const char*>{"GAIN", "MIX"}, "driveBypass");
    pedals[2] = std::make_unique<PedalCard>(*this, GuitarForgeAudioProcessor::PedalId::tone,
        "TONE SHAPER", juce::Colour(0xff2478a8), std::initializer_list<const char*>{"bass", "mid", "treble"},
        std::initializer_list<const char*>{"BASS", "MID", "TREBLE"}, "toneBypass");
    for (auto& pedal : pedals) addAndMakeVisible(*pedal);

    configureKnob(outputKnob, juce::Colour(0xffffa62b));
    outputKnob.setTextValueSuffix(" dB");
    addAndMakeVisible(outputKnob);
    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    outputLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(outputLabel);
    outputAttachment = std::make_unique<SliderAttachment>(processor.parameters, "output", outputKnob);
    setResizable(true, true);
    setResizeLimits(760, 430, 1280, 720);
    setSize(980, 560);
}

GuitarForgeAudioProcessorEditor::~GuitarForgeAudioProcessorEditor() = default;

void GuitarForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111318));
    juce::ColourGradient glow(juce::Colour(0xff3b2513), getWidth() * 0.5f, 0.0f,
                              juce::Colour(0xff111318), getWidth() * 0.5f, static_cast<float>(getHeight()), false);
    g.setGradientFill(glow); g.fillAll();
    g.setColour(juce::Colour(0xffffa62b));
    g.setFont(juce::FontOptions(29.0f, juce::Font::bold));
    g.drawText("GUITAR FORGE", 24, 12, getWidth() - 160, 38, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffaeb2bb));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("PEDALBOARD  •  SIGNAL FLOWS LEFT TO RIGHT", 27, 47, getWidth() - 180, 18,
               juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff2b2e36));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(10.0f), 10.0f, 1.5f);
}

void GuitarForgeAudioProcessorEditor::layoutPedals()
{
    auto area = getLocalBounds().reduced(18).withTrimmedTop(70).withTrimmedBottom(12);
    area.removeFromRight(100);
    const auto order = processor.getPedalOrder();
    const int gap = 10;
    const int width = (area.getWidth() - gap * 2) / 3;
    for (int slot = 0; slot < 3; ++slot) {
        auto& pedal = pedals[static_cast<size_t>(order[static_cast<size_t>(slot)])];
        pedal->setSlot(slot);
        pedal->setBounds(area.removeFromLeft(width));
        pedal->toFront(false);
        if (slot < 2) area.removeFromLeft(gap);
    }
}

void GuitarForgeAudioProcessorEditor::resized()
{
    layoutPedals();
    auto outputArea = getLocalBounds().removeFromRight(108).withTrimmedTop(75).withTrimmedBottom(20);
    outputLabel.setBounds(outputArea.removeFromTop(25));
    outputKnob.setBounds(outputArea.reduced(4));
}
