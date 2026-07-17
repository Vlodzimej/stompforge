#include "PluginEditor.h"

namespace
{
constexpr int pedalCellWidth = 290;
constexpr int pedalCellHeight = 270;
constexpr int pedalCellGap = 10;

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

class GridSelector final : public juce::Component
{
public:
    GridSelector(int currentRows, int currentColumns, std::function<void(int, int)> selected)
        : rows(currentRows), columns(currentColumns), onSelected(std::move(selected))
    {
        setSize(440, 310);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff15181e));
        g.setColour(juce::Colours::white); g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("PEDALBOARD MATRIX", 18, 12, getWidth() - 36, 26, juce::Justification::centred);
        g.setColour(juce::Colour(0xffaeb2bb)); g.setFont(juce::FontOptions(12.0f));
        g.drawText("Click the bottom-right cell of the desired matrix",
                   18, 39, getWidth() - 36, 22, juce::Justification::centred);
        const auto area = gridArea();
        const auto cellWidth = area.getWidth() / 4.0f, cellHeight = area.getHeight() / 3.0f;
        for (int row = 0; row < 3; ++row)
            for (int column = 0; column < 4; ++column) {
                auto cell = juce::Rectangle<float>(area.getX() + column * cellWidth,
                    area.getY() + row * cellHeight, cellWidth, cellHeight).reduced(5.0f);
                const auto previewRows = hoveredRows > 0 ? hoveredRows : rows;
                const auto previewColumns = hoveredColumns > 0 ? hoveredColumns : columns;
                const auto inSelection = row < previewRows && column < previewColumns;
                g.setColour(inSelection ? juce::Colour(0xffa34f24) : juce::Colour(0xff272c35));
                g.fillRoundedRectangle(cell, 7.0f);
                g.setColour(inSelection ? juce::Colour(0xffffc77d) : juce::Colour(0xff59616e));
                g.drawRoundedRectangle(cell, 7.0f, 1.5f);
                if (!inSelection) {
                    g.setColour(juce::Colour(0x553f4651));
                    g.drawLine(cell.getX() + 8.0f, cell.getBottom() - 8.0f,
                               cell.getRight() - 8.0f, cell.getY() + 8.0f, 1.0f);
                }
            }
        const auto previewRows = hoveredRows > 0 ? hoveredRows : rows;
        const auto previewColumns = hoveredColumns > 0 ? hoveredColumns : columns;
        const auto selectedBounds = juce::Rectangle<float>(area.getX(), area.getY(),
            cellWidth * previewColumns, cellHeight * previewRows).reduced(2.0f);
        g.setColour(juce::Colour(0xffffd28c)); g.drawRoundedRectangle(selectedBounds, 9.0f, 3.0f);
        const auto first = juce::Rectangle<float>(area.getX() + (previewColumns - 1) * cellWidth,
            area.getY() + (previewRows - 1) * cellHeight, cellWidth, cellHeight).reduced(7.0f);
        const auto last = juce::Rectangle<float>(area.getX(), area.getY(), cellWidth, cellHeight).reduced(7.0f);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xffbdf0ff));
        g.drawText("INPUT", first.toNearestInt().removeFromBottom(18), juce::Justification::centred);
        g.drawText("OUTPUT", last.toNearestInt().removeFromTop(18), juce::Justification::centred);
        g.setColour(juce::Colour(0xffffc77d)); g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText((hoveredRows > 0 ? "PREVIEW  " : "CURRENT  ")
                       + juce::String(previewColumns) + " x " + juce::String(previewRows), 18, 280,
                   getWidth() - 36, 20, juce::Justification::centred);
    }

    void mouseMove(const juce::MouseEvent& event) override
    {
        const auto area = gridArea();
        if (!area.contains(event.position)) {
            clearHover();
            return;
        }
        const auto newColumns = juce::jlimit(1, 4, 1 + static_cast<int>((event.position.x - area.getX()) / (area.getWidth() / 4.0f)));
        const auto newRows = juce::jlimit(1, 3, 1 + static_cast<int>((event.position.y - area.getY()) / (area.getHeight() / 3.0f)));
        if (newRows != hoveredRows || newColumns != hoveredColumns) {
            hoveredRows = newRows;
            hoveredColumns = newColumns;
            repaint();
        }
    }

    void mouseExit(const juce::MouseEvent&) override { clearHover(); }

    void mouseDown(const juce::MouseEvent& event) override
    {
        const auto area = gridArea();
        if (!area.contains(event.position)) return;
        const auto newColumns = juce::jlimit(1, 4, 1 + static_cast<int>((event.position.x - area.getX()) / (area.getWidth() / 4.0f)));
        const auto newRows = juce::jlimit(1, 3, 1 + static_cast<int>((event.position.y - area.getY()) / (area.getHeight() / 3.0f)));
        rows = newRows; columns = newColumns; repaint();
        if (onSelected) onSelected(rows, columns);
    }

private:
    void clearHover()
    {
        if (hoveredRows == 0 && hoveredColumns == 0) return;
        hoveredRows = hoveredColumns = 0;
        repaint();
    }
    juce::Rectangle<float> gridArea() const { return { 42.0f, 70.0f, 356.0f, 200.0f }; }
    int rows, columns;
    int hoveredRows = 0, hoveredColumns = 0;
    std::function<void(int, int)> onSelected;
};

void configureParameterSlider(juce::Slider& slider, const juce::String& parameterId, juce::Colour colour)
{
    configureKnob(slider, colour);
    if (parameterId == "5150Channel") slider.setTextValueSuffix({});
    else if (parameterId == "irLowCut" || parameterId == "irHighCut") slider.setTextValueSuffix(" Hz");
    else if (parameterId == "mix" || parameterId == "irMix" || parameterId.startsWith("ds1")
        || parameterId.startsWith("chorus") || parameterId.startsWith("reverb")
        || parameterId.startsWith("delay") || parameterId.startsWith("jcm")
        || (parameterId.startsWith("5150") && parameterId != "5150Channel")) slider.setTextValueSuffix(" %");
    else slider.setTextValueSuffix(" dB");
}

class AdvancedControls final : public juce::Component
{
public:
    AdvancedControls(StompForgeAudioProcessor::APVTS& state, juce::Colour colour,
                     const std::vector<juce::String>& ids, const std::vector<juce::String>& names)
    {
        for (size_t i = 3; i < ids.size(); ++i) {
            auto slider = std::make_unique<juce::Slider>();
            configureParameterSlider(*slider, ids[i], colour); addAndMakeVisible(*slider);
            attachments.push_back(std::make_unique<SliderAttachment>(state, ids[i], *slider));
            sliders.push_back(std::move(slider));
            auto label = std::make_unique<juce::Label>();
            label->setText(names[i], juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setColour(juce::Label::textColourId, juce::Colour(0xfff8efd8));
            addAndMakeVisible(*label); labels.push_back(std::move(label));
        }
        const auto columns = juce::jmin(3, static_cast<int>(sliders.size()));
        const auto rows = (static_cast<int>(sliders.size()) + columns - 1) / columns;
        setSize(columns * 150 + 28, rows * 150 + 28);
    }
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff15181e)); }
    void resized() override
    {
        auto area = getLocalBounds().reduced(14);
        const auto columns = juce::jmin(3, static_cast<int>(sliders.size()));
        const auto rows = (static_cast<int>(sliders.size()) + columns - 1) / columns;
        const auto width = area.getWidth() / columns, height = area.getHeight() / rows;
        for (size_t i = 0; i < sliders.size(); ++i) {
            auto cell = juce::Rectangle<int>(area.getX() + static_cast<int>(i) % columns * width,
                area.getY() + static_cast<int>(i) / columns * height, width, height);
            labels[i]->setBounds(cell.removeFromTop(22)); sliders[i]->setBounds(cell.reduced(3));
        }
    }
private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
};
}

class StompForgeAudioProcessorEditor::LevelFader final : public juce::Slider
{
public:
    explicit LevelFader(juce::Colour accent) : colour(accent)
    {
        setSliderStyle(juce::Slider::LinearVertical);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setTextValueSuffix(" dB");
    }
    void setSignalLevel(float linear)
    {
        const auto target = juce::jlimit(0.0f, 1.0f,
            juce::jmap(juce::Decibels::gainToDecibels(linear, -60.0f), -60.0f, 0.0f, 0.0f, 1.0f));
        level = target >= level ? target : level * 0.82f; repaint();
    }
    void paint(juce::Graphics& g) override
    {
        auto track = getLocalBounds().toFloat().reduced(25.0f, 5.0f); track.removeFromBottom(25.0f);
        g.setColour(juce::Colour(0xff20242b)); g.fillRoundedRectangle(track, 8.0f);
        auto fill = track.withTop(track.getBottom() - track.getHeight() * level);
        juce::ColourGradient gradient(juce::Colour(0xffff5048), fill.getX(), track.getY(),
            colour.brighter(0.35f), fill.getX(), track.getBottom(), false);
        g.setGradientFill(gradient); g.fillRoundedRectangle(fill, 7.0f);
        const auto y = track.getBottom() - static_cast<float>(valueToProportionOfLength(getValue())) * track.getHeight();
        g.setColour(juce::Colour(0xffffe0a3));
        g.fillRoundedRectangle(track.getX() - 6.0f, y - 3.0f, track.getWidth() + 12.0f, 6.0f, 3.0f);
        g.setColour(juce::Colour(0x45000000));
        g.fillRoundedRectangle(4.0f, static_cast<float>(getHeight() - 21), static_cast<float>(getWidth() - 8), 19.0f, 4.0f);
        g.setColour(juce::Colours::white); g.setFont(juce::FontOptions(12.0f));
        g.drawText(getTextFromValue(getValue()), 4, getHeight() - 22, getWidth() - 8, 20, juce::Justification::centred);
    }
private:
    juce::Colour colour;
    float level = 0.0f;
};

class StompForgeAudioProcessorEditor::PedalCard final : public juce::Component,
                                                          public juce::DragAndDropTarget,
                                                          private juce::Timer
{
public:
    PedalCard(StompForgeAudioProcessorEditor& owner,
              StompForgeAudioProcessor::PedalId pedalId,
              juce::String pedalName, juce::Colour pedalColour,
              std::initializer_list<const char*> parameterIds,
              std::initializer_list<const char*> parameterNames,
              const char* bypassId, const char* optionId = nullptr, bool showTuner = false,
              bool showImpulseLoader = false)
        : editor(owner), id(pedalId), name(std::move(pedalName)), colour(pedalColour),
          tunerDisplay(showTuner), impulseLoader(showImpulseLoader)
    {
        for (auto* value : parameterIds) allIds.emplace_back(value);
        for (auto* value : parameterNames) allNames.emplace_back(value);
        const auto visibleCount = juce::jmin<size_t>(3, allIds.size());
        for (size_t i = 0; i < visibleCount; ++i) {
            auto knob = std::make_unique<juce::Slider>();
            configureParameterSlider(*knob, allIds[i], colour);
            addAndMakeVisible(*knob);
            knob->addMouseListener(this, false);
            attachments.push_back(std::make_unique<SliderAttachment>(editor.processor.parameters, allIds[i], *knob));
            knobs.push_back(std::move(knob));

            auto label = std::make_unique<juce::Label>();
            label->setText(allNames[i], juce::dontSendNotification);
            label->setJustificationType(juce::Justification::centred);
            label->setColour(juce::Label::textColourId, juce::Colour(0xfff8efd8));
            addAndMakeVisible(*label);
            label->addMouseListener(this, false);
            labels.push_back(std::move(label));
        }
        if (allIds.size() > 3) {
            more.setButtonText("MORE");
            more.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff292d35));
            more.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffd28c));
            more.onClick = [this] { showAdvancedControls(); };
            addAndMakeVisible(more); more.addMouseListener(this, false);
        }

        bypass.setButtonText("ON");
        bypass.setClickingTogglesState(true);
        bypass.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff4a2024));
        bypass.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff15171b));
        bypass.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffff8c91));
        bypass.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff777b82));
        addAndMakeVisible(bypass);
        bypass.addMouseListener(this, false);
        if (auto* parameter = editor.processor.parameters.getParameter(bypassId)) {
            bypassAttachment = std::make_unique<juce::ParameterAttachment>(*parameter,
                [this] (float value) {
                    bypass.setToggleState(value < 0.5f, juce::dontSendNotification);
                }, nullptr);
            bypass.onClick = [this] {
                bypassAttachment->setValueAsCompleteGesture(bypass.getToggleState() ? 0.0f : 1.0f);
            };
            bypassAttachment->sendInitialUpdate();
        }
        if (optionId != nullptr) {
            option.setButtonText("CAB SIM"); option.setClickingTogglesState(true);
            option.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffc08b32));
            addAndMakeVisible(option);
            option.addMouseListener(this, false);
            if (auto* parameter = editor.processor.parameters.getParameter(optionId)) {
                optionAttachment = std::make_unique<juce::ParameterAttachment>(*parameter,
                    [this] (float value) {
                        option.setToggleState(value >= 0.5f && !editor.processor.isImpulseActive(),
                                              juce::dontSendNotification);
                    }, nullptr);
                option.onClick = [this] {
                    if (option.getToggleState() && editor.processor.isImpulseActive()) {
                        option.setToggleState(false, juce::dontSendNotification);
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            "CABSIM is unavailable",
                            "Remove IMPULSE from the pedalboard before enabling an amp CABSIM.");
                        return;
                    }
                    optionAttachment->setValueAsCompleteGesture(option.getToggleState() ? 1.0f : 0.0f);
                };
                optionAttachment->sendInitialUpdate();
            }
        }
        if (impulseLoader) {
            loadImpulse.setButtonText("LOAD IR");
            loadImpulse.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff243c48));
            loadImpulse.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffbde9ff));
            addAndMakeVisible(loadImpulse); loadImpulse.addMouseListener(this, false);
            impulseName.setJustificationType(juce::Justification::centred);
            impulseName.setColour(juce::Label::textColourId, juce::Colour(0xffbde9ff));
            impulseName.setFont(juce::FontOptions(11.0f));
            impulseName.setText(editor.processor.getCabImpulseName(), juce::dontSendNotification);
            addAndMakeVisible(impulseName);
            loadImpulse.onClick = [this] {
                chooser = std::make_unique<juce::FileChooser>("Load cabinet impulse response", juce::File{}, "*.wav;*.aif;*.aiff");
                auto safeThis = juce::Component::SafePointer<PedalCard>(this);
                chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                    [safeThis] (const juce::FileChooser& selected) {
                        if (safeThis == nullptr) return;
                        const auto file = selected.getResult();
                        if (file.existsAsFile() && safeThis->editor.processor.loadCabImpulse(file))
                            safeThis->impulseName.setText(safeThis->editor.processor.getCabImpulseName(), juce::dontSendNotification);
                    });
            };
        }
    }

    bool isInterestedInDragSource(const SourceDetails& details) override
    {
        return details.description.toString().startsWith("slot:");
    }

    void itemDragEnter(const SourceDetails&) override { dragOver = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dragOver = false; repaint(); }
    void itemDropped(const SourceDetails& details) override
    {
        dragOver = false;
        const auto sourceSlot = details.description.toString()
            .fromFirstOccurrenceOf(":", false, false).getIntValue();
        editor.processor.movePedal(sourceSlot, slot);
        editor.layoutPedals();
        repaint();
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        canDrag = event.getEventRelativeTo(this).y < 48;
        dragStarted = false;
        startTimer(650);
    }

    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (event.getDistanceFromDragStart() > 7) {
            stopTimer();
            if (canDrag && !editor.isDragAndDropActive()) {
                dragStarted = true;
                editor.startDragging("slot:" + juce::String(slot), this);
            }
        }
    }

    void mouseUp(const juce::MouseEvent&) override { stopTimer(); }

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
        g.drawText("DRAG TO REORDER  |  HOLD TO REPLACE", 12, 34, getWidth() - 24, 16,
                   juce::Justification::centred);
        if (tunerDisplay) {
            const auto state = editor.processor.getTunerState();
            static constexpr std::array<const char*, 12> noteNames {
                "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            const auto hasNote = state.midiNote >= 0 && state.confidence >= 0.55f;
            const auto noteName = hasNote
                ? juce::String(noteNames[static_cast<size_t>((state.midiNote % 12 + 12) % 12)])
                    + juce::String(state.midiNote / 12 - 1)
                : juce::String("--");
            g.setColour(juce::Colour(0xffeef5f1));
            g.setFont(juce::FontOptions(48.0f, juce::Font::bold));
            g.drawText(noteName, 18, 62, getWidth() - 36, 58, juce::Justification::centred);
            g.setFont(juce::FontOptions(13.0f));
            g.drawText(hasNote ? juce::String(state.frequency, 1) + " Hz" : "PLAY A STRING",
                       18, 118, getWidth() - 36, 22, juce::Justification::centred);

            const auto rail = juce::Rectangle<float>(24.0f, 154.0f,
                static_cast<float>(getWidth() - 48), 18.0f);
            const auto centre = rail.getCentreX();
            g.setColour(juce::Colour(0xff182127)); g.fillRoundedRectangle(rail, 7.0f);
            g.setColour(juce::Colour(0xff607078)); g.drawRoundedRectangle(rail, 7.0f, 1.0f);
            if (hasNote) {
                const auto amount = juce::jlimit(-1.0f, 1.0f, state.cents / 50.0f);
                const auto length = std::abs(amount) * rail.getWidth() * 0.5f;
                const auto closeness = 1.0f - std::abs(amount);
                const auto meterColour = juce::Colour(0xffe13d35)
                    .interpolatedWith(juce::Colour(0xff35ff72), closeness);
                g.setColour(meterColour);
                const auto meter = amount < 0.0f
                    ? juce::Rectangle<float>(centre - length, rail.getY(), length, rail.getHeight())
                    : juce::Rectangle<float>(centre, rail.getY(), length, rail.getHeight());
                g.fillRoundedRectangle(meter, 6.0f);
                g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
                g.drawText(juce::String(state.cents, 1) + " cents", 20, 178,
                           getWidth() - 40, 20, juce::Justification::centred);
            }
            g.setColour(juce::Colour(0xff7dff9d));
            g.fillRect(centre - 1.5f, rail.getY() - 3.0f, 3.0f, rail.getHeight() + 6.0f);
        }
        g.setColour(juce::Colour(0xaa17191e));
        g.fillRoundedRectangle(14.0f, static_cast<float>(getHeight() - 72),
                               static_cast<float>(getWidth() - 28), 56.0f, 7.0f);
    }

    void resized() override
    {
        auto controls = getLocalBounds().reduced(10).withTrimmedTop(impulseLoader ? 68 : 48).withTrimmedBottom(76);
        const auto rows = knobs.size() > 3 ? 2 : 1;
        const auto perRow = static_cast<int>((knobs.size() + rows - 1) / rows);
        const auto cellWidth = perRow > 0 ? controls.getWidth() / perRow : 0;
        for (size_t i = 0; i < knobs.size(); ++i) {
            auto row = controls.withHeight(controls.getHeight() / rows).translated(0, static_cast<int>(i / perRow) * controls.getHeight() / rows);
            auto cell = row.withWidth(cellWidth).translated(static_cast<int>(i % perRow) * cellWidth, 0);
            labels[i]->setBounds(cell.removeFromTop(20));
            knobs[i]->setBounds(cell.reduced(1));
        }
        const auto hasOption = optionAttachment != nullptr || impulseLoader;
        const auto hasMore = allIds.size() > 3;
        const auto buttonCount = 1 + static_cast<int>(hasOption) + static_cast<int>(hasMore);
        const auto buttonWidth = 74, gap = 8;
        auto x = (getWidth() - (buttonCount * buttonWidth + (buttonCount - 1) * gap)) / 2;
        bypass.setBounds(x, getHeight() - 62, buttonWidth, 38); x += buttonWidth + gap;
        if (hasOption) option.setBounds(x, getHeight() - 62, buttonWidth, 38), x += buttonWidth + gap;
        if (hasMore) more.setBounds(x, getHeight() - 62, buttonWidth, 38);
        if (impulseLoader) {
            option.setVisible(false);
            loadImpulse.setBounds(option.getBounds());
            impulseName.setBounds(16, 48, getWidth() - 32, 18);
        }
    }

private:
    void showAdvancedControls()
    {
        juce::DialogWindow::LaunchOptions options;
        options.dialogTitle = name + " — Advanced controls";
        options.dialogBackgroundColour = juce::Colour(0xff15181e);
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.content.setOwned(new AdvancedControls(editor.processor.parameters, colour, allIds, allNames));
        options.launchAsync();
    }
    void timerCallback() override
    {
        stopTimer();
        if (!dragStarted) editor.showEffectMenu(slot);
    }

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    StompForgeAudioProcessorEditor& editor;
    StompForgeAudioProcessor::PedalId id;
    juce::String name;
    juce::Colour colour;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<SliderAttachment>> attachments;
    std::vector<juce::String> allIds, allNames;
    juce::TextButton bypass;
    std::unique_ptr<juce::ParameterAttachment> bypassAttachment;
    juce::TextButton option;
    juce::TextButton more;
    std::unique_ptr<juce::ParameterAttachment> optionAttachment;
    juce::TextButton loadImpulse;
    juce::Label impulseName;
    std::unique_ptr<juce::FileChooser> chooser;
    int slot = 0;
    bool canDrag = false;
    bool dragStarted = false;
    bool dragOver = false;
    bool tunerDisplay = false;
    bool impulseLoader = false;
};

class StompForgeAudioProcessorEditor::GridCell final : public juce::Component,
                                                        public juce::DragAndDropTarget,
                                                        private juce::Timer
{
public:
    explicit GridCell(StompForgeAudioProcessorEditor& owner) : editor(owner) {}
    void setSlot(int newSlot) { slot = newSlot; }
    bool isInterestedInDragSource(const SourceDetails& details) override
    {
        return details.description.toString().startsWith("slot:");
    }
    void itemDragEnter(const SourceDetails&) override { dragOver = true; repaint(); }
    void itemDragExit(const SourceDetails&) override { dragOver = false; repaint(); }
    void itemDropped(const SourceDetails& details) override
    {
        dragOver = false;
        const auto source = details.description.toString().fromFirstOccurrenceOf(":", false, false).getIntValue();
        editor.processor.movePedal(source, slot); editor.layoutPedals(); repaint();
    }
    void mouseDown(const juce::MouseEvent&) override { longPressOpened = false; dragged = false; startTimer(650); }
    void mouseDrag(const juce::MouseEvent& event) override
    {
        if (event.getDistanceFromDragStart() > 7) { dragged = true; stopTimer(); }
    }
    void mouseUp(const juce::MouseEvent&) override
    {
        stopTimer();
        if (!dragged && !longPressOpened) editor.showEffectMenu(slot);
    }
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        g.setColour(dragOver ? juce::Colour(0x553fa7d6) : juce::Colour(0x241a2028));
        g.fillRoundedRectangle(bounds, 13.0f);
        g.setColour(dragOver ? juce::Colour(0xff66c9ef) : juce::Colour(0x88616b78));
        g.drawRoundedRectangle(bounds, 13.0f, dragOver ? 2.5f : 1.2f);
        const auto centre = bounds.getCentre();
        g.setColour(dragOver ? juce::Colour(0xffbdefff) : juce::Colour(0x9979828d));
        g.drawLine(centre.x - 12.0f, centre.y, centre.x + 12.0f, centre.y, 1.5f);
        g.drawLine(centre.x, centre.y - 12.0f, centre.x, centre.y + 12.0f, 1.5f);
        g.setFont(juce::FontOptions(11.0f));
        g.drawText("DROP OR HOLD TO ADD", bounds.toNearestInt().withTrimmedTop(juce::roundToInt(bounds.getHeight() * 0.57f)),
                   juce::Justification::centred);
    }
private:
    void timerCallback() override { stopTimer(); longPressOpened = true; editor.showEffectMenu(slot); }
    StompForgeAudioProcessorEditor& editor;
    int slot = 0;
    bool dragOver = false;
    bool longPressOpened = false;
    bool dragged = false;
};

StompForgeAudioProcessorEditor::StompForgeAudioProcessorEditor(StompForgeAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    pedals[0] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::gate,
        "STARGATE", juce::Colour(0xff287f78), std::initializer_list<const char*>{"gate"},
        std::initializer_list<const char*>{"THRESHOLD"}, "gateBypass");
    pedals[1] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::ds1,
        "DEIMOS-1", juce::Colour(0xffef681f),
        std::initializer_list<const char*>{"ds1Tone", "ds1Level", "ds1Dist"},
        std::initializer_list<const char*>{"TONE", "LEVEL", "DIST"}, "ds1Bypass");
    pedals[2] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::tone,
        "FREQUENCY", juce::Colour(0xff2478a8), std::initializer_list<const char*>{"bass", "mid", "treble"},
        std::initializer_list<const char*>{"BASS", "MID", "TREBLE"}, "toneBypass");
    pedals[3] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::jcm800,
        "MARS-8", juce::Colour(0xff5b431d),
        std::initializer_list<const char*>{"jcmPreamp", "jcmMaster", "jcmPresence", "jcmBass", "jcmMiddle", "jcmTreble", "jcmSag"},
        std::initializer_list<const char*>{"PREAMP", "MASTER", "PRESENCE", "BASS", "MIDDLE", "TREBLE", "SAG"},
        "jcmBypass", "jcmCab");
    pedals[4] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::chorus,
        "CERES-2", juce::Colour(0xff3f83a6),
        std::initializer_list<const char*>{"chorusRate", "chorusDepth", "chorusMix"},
        std::initializer_list<const char*>{"RATE", "DEPTH", "MIX"}, "chorusBypass");
    pedals[5] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::reverb,
        "VOID CHAMBER", juce::Colour(0xff327a91),
        std::initializer_list<const char*>{"reverbSize", "reverbDamping", "reverbMix"},
        std::initializer_list<const char*>{"SIZE", "DAMP", "MIX"}, "reverbBypass");
    pedals[6] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::delay,
        "PULSAR", juce::Colour(0xff3f659b),
        std::initializer_list<const char*>{"delayTime", "delayFeedback", "delayMix"},
        std::initializer_list<const char*>{"TIME", "FEEDBACK", "MIX"}, "delayBypass");
    pedals[7] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::tuner,
        "LUNER", juce::Colour(0xff26343d), std::initializer_list<const char*>{},
        std::initializer_list<const char*>{}, "tunerBypass", nullptr, true);
    pedals[8] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::amp5150,
        "VULCAN-5", juce::Colour(0xff6f2020),
        std::initializer_list<const char*>{"5150Channel", "5150Gain", "5150Master", "5150Bass", "5150Middle", "5150Treble", "5150Presence", "5150Resonance", "5150Bias", "5150Sag"},
        std::initializer_list<const char*>{"CHANNEL", "GAIN", "MASTER", "BASS", "MIDDLE", "TREBLE", "PRESENCE", "RESONANCE", "BIAS", "SUPPLY"},
        "5150Bypass", "5150Cab");
    pedals[9] = std::make_unique<PedalCard>(*this, StompForgeAudioProcessor::PedalId::impulseCab,
        "IMPULSE", juce::Colour(0xff28576b),
        std::initializer_list<const char*>{"irLevel", "irMix", "irLowCut", "irHighCut"},
        std::initializer_list<const char*>{"LEVEL", "MIX", "LOW CUT", "HIGH CUT"},
        "irBypass", nullptr, false, true);
    for (auto& cell : gridCells) { cell = std::make_unique<GridCell>(*this); addAndMakeVisible(*cell); }
    for (auto& pedal : pedals) addAndMakeVisible(*pedal);

    inputFader = std::make_unique<LevelFader>(juce::Colour(0xffef554f));
    addAndMakeVisible(*inputFader);
    inputLabel.setText("INPUT", juce::dontSendNotification);
    inputLabel.setJustificationType(juce::Justification::centred);
    inputLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(inputLabel);
    inputAttachment = std::make_unique<SliderAttachment>(processor.parameters, "input", *inputFader);

    outputFader = std::make_unique<LevelFader>(juce::Colour(0xffffa62b));
    addAndMakeVisible(*outputFader);
    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    outputLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(outputLabel);
    outputAttachment = std::make_unique<SliderAttachment>(processor.parameters, "output", *outputFader);
    gridButton.setButtonText("GRID");
    gridButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff292d35));
    gridButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffc77d));
    gridButton.onClick = [this] { showGridSelector(); };
    addAndMakeVisible(gridButton);
    setResizable(true, true);
    setSize(1100, 760);
    updateResizeLimitsForGrid(true);
    startTimerHz(20);
}

StompForgeAudioProcessorEditor::~StompForgeAudioProcessorEditor() = default;

void StompForgeAudioProcessorEditor::showGridSelector()
{
    auto safeThis = juce::Component::SafePointer<StompForgeAudioProcessorEditor>(this);
    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Pedalboard grid";
    options.dialogBackgroundColour = juce::Colour(0xff15181e);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;
    options.content.setOwned(new GridSelector(processor.getGridRows(), processor.getGridColumns(),
        [safeThis] (int rows, int columns) {
            if (safeThis == nullptr) return;
            if (safeThis->processor.setGridSize(rows, columns)) {
                safeThis->updateResizeLimitsForGrid(true);
                safeThis->layoutPedals();
            }
        }));
    options.launchAsync();
}

void StompForgeAudioProcessorEditor::showEffectMenu(int slot)
{
    using Id = StompForgeAudioProcessor::PedalId;
    auto item = [] (Id id, const juce::String& name) {
        juce::PopupMenu menu; menu.addItem(static_cast<int>(id) + 1, name); return menu;
    };
    juce::PopupMenu menu;
    menu.addSubMenu("Dynamic", item(Id::gate, "STARGATE"));
    menu.addSubMenu("Distortion", item(Id::ds1, "DEIMOS-1"));
    menu.addSubMenu("Modulation", item(Id::chorus, "CERES-2"));
    juce::PopupMenu amps;
    amps.addItem(static_cast<int>(Id::jcm800) + 1, "MARS-8");
    amps.addItem(static_cast<int>(Id::amp5150) + 1, "VULCAN-5");
    menu.addSubMenu("Amp", amps);
    menu.addSubMenu("Reverb", item(Id::reverb, "VOID CHAMBER"));
    menu.addSubMenu("Delay", item(Id::delay, "PULSAR"));
    menu.addSubMenu("EQ", item(Id::tone, "FREQUENCY"));
    menu.addSubMenu("Cabs", item(Id::impulseCab, "IMPULSE"));
    menu.addSubMenu("Utils", item(Id::tuner, "LUNER"));
    const auto currentOrder = processor.getPedalOrder();
    menu.addSeparator();
    menu.addItem(1000, "Clear current cell",
        currentOrder[static_cast<size_t>(slot)] != Id::empty);
    auto safeThis = juce::Component::SafePointer<StompForgeAudioProcessorEditor>(this);
    menu.showMenuAsync(juce::PopupMenu::Options(), [safeThis, slot] (int result) {
        if (safeThis != nullptr && result == 1000) {
            safeThis->processor.clearPedal(slot);
            safeThis->layoutPedals();
        } else if (safeThis != nullptr && result > 0) {
            safeThis->processor.replacePedal(slot, static_cast<Id>(result - 1));
            safeThis->layoutPedals();
        }
    });
}

void StompForgeAudioProcessorEditor::timerCallback()
{
    inputFader->setSignalLevel(processor.consumeInputLevel());
    outputFader->setSignalLevel(processor.consumeOutputLevel());
    if (pedals[static_cast<size_t>(StompForgeAudioProcessor::PedalId::tuner)]->isVisible())
        pedals[static_cast<size_t>(StompForgeAudioProcessor::PedalId::tuner)]->repaint();
}

void StompForgeAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff111318));
    juce::ColourGradient glow(juce::Colour(0xff3b2513), getWidth() * 0.5f, 0.0f,
                              juce::Colour(0xff111318), getWidth() * 0.5f, static_cast<float>(getHeight()), false);
    g.setGradientFill(glow); g.fillAll();
    g.setColour(juce::Colour(0xffffa62b));
    g.setFont(juce::FontOptions(29.0f, juce::Font::bold));
    g.drawText("STOMPFORGE", 24, 12, getWidth() - 160, 38, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffaeb2bb));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("PEDALBOARD  |  INPUT BOTTOM-RIGHT  >  OUTPUT TOP-LEFT", 27, 47, getWidth() - 180, 18,
               juce::Justification::centredLeft);
    const auto order = processor.getPedalOrder();
    const auto capacity = processor.getGridRows() * processor.getGridColumns();
    std::vector<int> activeSlots;
    activeSlots.reserve(static_cast<size_t>(capacity));
    for (int slot = 0; slot < capacity; ++slot)
        if (order[static_cast<size_t>(slot)] != StompForgeAudioProcessor::PedalId::empty)
            activeSlots.push_back(slot);
    if (!activeSlots.empty()) {
        std::vector<juce::Point<float>> routePoints;
        routePoints.reserve(activeSlots.size());
        for (const auto slot : activeSlots)
            routePoints.push_back(getGridCellBounds(slot).toFloat().getCentre());
        auto portTowards = [] (const juce::Rectangle<float>& bounds, juce::Point<float> direction) {
            const auto centre = bounds.getCentre();
            if (std::abs(direction.x) >= std::abs(direction.y))
                return juce::Point<float>(direction.x >= 0.0f ? bounds.getRight() : bounds.getX(), centre.y);
            return juce::Point<float>(centre.x, direction.y >= 0.0f ? bounds.getBottom() : bounds.getY());
        };
        const auto firstBounds = getGridCellBounds(activeSlots.front()).toFloat();
        const auto lastBounds = getGridCellBounds(activeSlots.back()).toFloat();
        const auto startPort = routePoints.size() > 1
            ? portTowards(firstBounds, { routePoints.front().x - routePoints[1].x,
                                         routePoints.front().y - routePoints[1].y })
            : juce::Point<float>(firstBounds.getRight(), firstBounds.getCentreY());
        const auto endPort = routePoints.size() > 1
            ? portTowards(lastBounds, { routePoints.back().x - routePoints[routePoints.size() - 2].x,
                                        routePoints.back().y - routePoints[routePoints.size() - 2].y })
            : juce::Point<float>(lastBounds.getX(), lastBounds.getCentreY());
        juce::Path signalPath;
        signalPath.startNewSubPath(startPort);
        signalPath.lineTo(routePoints.front());
        auto appendRoundedRoute = [&signalPath] (juce::Point<float> from, juce::Point<float> to) {
            signalPath.lineTo(from);
            const juce::Point<float> delta { to.x - from.x, to.y - from.y };
            if (std::abs(delta.x) < 1.0f || std::abs(delta.y) < 1.0f) {
                signalPath.lineTo(to); return;
            }
            const auto verticalDirection = to.y >= from.y ? 1.0f : -1.0f;
            const auto horizontalDirection = to.x >= from.x ? 1.0f : -1.0f;
            const auto midY = (from.y + to.y) * 0.5f;
            const auto radius = juce::jmin(18.0f, std::abs(midY - from.y) * 0.45f,
                                           juce::jmax(4.0f, std::abs(to.x - from.x) * 0.25f));
            signalPath.lineTo(from.x, midY - verticalDirection * radius);
            signalPath.quadraticTo(from.x, midY,
                                   from.x + horizontalDirection * radius, midY);
            signalPath.lineTo(to.x - horizontalDirection * radius, midY);
            signalPath.quadraticTo(to.x, midY,
                                   to.x, midY + verticalDirection * radius);
            signalPath.lineTo(to);
        };
        for (size_t i = 1; i < routePoints.size(); ++i)
            appendRoundedRoute(routePoints[i - 1], routePoints[i]);
        signalPath.lineTo(endPort);
        g.setColour(juce::Colour(0x3328b8e8)); g.strokePath(signalPath, juce::PathStrokeType(8.0f));
        g.setColour(juce::Colour(0xcc65d9ff)); g.strokePath(signalPath, juce::PathStrokeType(2.0f));
        for (size_t i = 1; i < routePoints.size(); ++i) {
            juce::Point<float> direction { routePoints[i].x - routePoints[i - 1].x,
                                           routePoints[i].y - routePoints[i - 1].y };
            juce::Point<float> tip {
                routePoints[i - 1].x + direction.x * 0.62f,
                routePoints[i - 1].y + direction.y * 0.62f };
            if (std::abs(direction.x) >= 1.0f && std::abs(direction.y) >= 1.0f) {
                direction = { direction.x >= 0.0f ? 1.0f : -1.0f, 0.0f };
                tip = { (routePoints[i - 1].x + routePoints[i].x) * 0.5f,
                        (routePoints[i - 1].y + routePoints[i].y) * 0.5f };
            }
            const auto length = direction.getDistanceFromOrigin();
            if (length < 1.0f) continue;
            direction /= length;
            const juce::Point<float> normal { -direction.y, direction.x };
            juce::Path arrow; arrow.startNewSubPath(tip);
            arrow.lineTo(tip.x - direction.x * 10.0f + normal.x * 5.0f,
                         tip.y - direction.y * 10.0f + normal.y * 5.0f);
            arrow.lineTo(tip.x - direction.x * 10.0f - normal.x * 5.0f,
                         tip.y - direction.y * 10.0f - normal.y * 5.0f); arrow.closeSubPath();
            g.fillPath(arrow);
        }

        // Empty slots act like frosted glass over a cable passing beneath them.
        // The opaque veil first mutes the normal cable, then several clipped,
        // progressively wider strokes recreate a soft liquid-glass blur.
        for (int slot = 0; slot < capacity; ++slot) {
            if (order[static_cast<size_t>(slot)] != StompForgeAudioProcessor::PedalId::empty)
                continue;
            const auto glassBounds = getGridCellBounds(slot).reduced(5);
            juce::Graphics::ScopedSaveState savedState(g);
            g.reduceClipRegion(glassBounds);
            g.setColour(juce::Colour(0xb811151b));
            g.fillRoundedRectangle(glassBounds.toFloat(), 12.0f);
            g.setColour(juce::Colour(0x1028b8e8));
            g.strokePath(signalPath, juce::PathStrokeType(18.0f));
            g.setColour(juce::Colour(0x1e65d9ff));
            g.strokePath(signalPath, juce::PathStrokeType(10.0f));
            g.setColour(juce::Colour(0x5265d9ff));
            g.strokePath(signalPath, juce::PathStrokeType(2.0f));
            juce::ColourGradient glassSheen(juce::Colour(0x24ffffff), static_cast<float>(glassBounds.getX()),
                static_cast<float>(glassBounds.getY()), juce::Colour(0x08000000),
                static_cast<float>(glassBounds.getRight()), static_cast<float>(glassBounds.getBottom()), false);
            g.setGradientFill(glassSheen);
            g.fillRoundedRectangle(glassBounds.toFloat(), 12.0f);
        }
    }
    g.setColour(juce::Colour(0xff2b2e36));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(10.0f), 10.0f, 1.5f);
}

void StompForgeAudioProcessorEditor::layoutPedals()
{
    const auto order = processor.getPedalOrder();
    for (auto& pedal : pedals) pedal->setVisible(false);
    for (auto& cell : gridCells) cell->setVisible(false);
    const int capacity = processor.getGridRows() * processor.getGridColumns();
    for (int slot = 0; slot < capacity; ++slot) {
        auto& cell = gridCells[static_cast<size_t>(slot)];
        cell->setSlot(slot); cell->setBounds(getGridCellBounds(slot));
        const auto id = order[static_cast<size_t>(slot)];
        if (id == StompForgeAudioProcessor::PedalId::empty) {
            cell->setVisible(true); cell->toFront(false); continue;
        }
        auto& pedal = pedals[static_cast<size_t>(id)];
        pedal->setSlot(slot);
        pedal->setVisible(true);
        pedal->setBounds(getGridCellBounds(slot));
        pedal->toFront(false);
    }
    repaint();
}

void StompForgeAudioProcessorEditor::updateResizeLimitsForGrid(bool growIfNeeded)
{
    // These dimensions keep rotary controls, labels and the two-row amp layouts
    // large enough for accurate mouse operation in every effect module.
    constexpr int horizontalChrome = 18 * 2 + 100 * 2;
    constexpr int verticalChrome = 18 * 2 + 70 + 12;
    const auto columns = processor.getGridColumns();
    const auto rows = processor.getGridRows();
    const auto minimumWidth = horizontalChrome + columns * pedalCellWidth + (columns - 1) * pedalCellGap;
    const auto minimumHeight = verticalChrome + rows * pedalCellHeight + (rows - 1) * pedalCellGap;

    setResizeLimits(minimumWidth, minimumHeight, 1920, 1200);
    if (growIfNeeded && (getWidth() < minimumWidth || getHeight() < minimumHeight))
        setSize(juce::jmax(getWidth(), minimumWidth), juce::jmax(getHeight(), minimumHeight));
}

juce::Rectangle<int> StompForgeAudioProcessorEditor::getGridCellBounds(int slot) const
{
    auto area = getLocalBounds().reduced(18).withTrimmedTop(70).withTrimmedBottom(12);
    area.removeFromLeft(100); area.removeFromRight(100);
    const int columns = processor.getGridColumns(), rows = processor.getGridRows();
    const auto matrixWidth = columns * pedalCellWidth + (columns - 1) * pedalCellGap;
    const auto matrixHeight = rows * pedalCellHeight + (rows - 1) * pedalCellGap;
    area = area.withSizeKeepingCentre(matrixWidth, matrixHeight);
    const int capacity = rows * columns;
    slot = juce::jlimit(0, capacity - 1, slot);
    const auto visualIndex = capacity - 1 - slot;
    const auto column = visualIndex % columns, row = visualIndex / columns;
    return { area.getX() + column * (pedalCellWidth + pedalCellGap),
             area.getY() + row * (pedalCellHeight + pedalCellGap),
             pedalCellWidth, pedalCellHeight };
}

void StompForgeAudioProcessorEditor::resized()
{
    layoutPedals();
    auto inputArea = getLocalBounds().removeFromRight(108).withTrimmedTop(75).withTrimmedBottom(20);
    inputLabel.setBounds(inputArea.removeFromTop(25));
    inputFader->setBounds(inputArea.reduced(4));
    auto outputArea = getLocalBounds().removeFromLeft(108).withTrimmedTop(75).withTrimmedBottom(20);
    outputLabel.setBounds(outputArea.removeFromTop(25));
    outputFader->setBounds(outputArea.reduced(4));
    gridButton.setBounds(getWidth() - 96, 20, 70, 34);
}
