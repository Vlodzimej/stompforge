#include "PluginProcessor.h"
#include "PluginEditor.h"

StompForgeAudioProcessor::StompForgeAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    effects[0] = std::make_unique<GateEffect>(*parameters.getRawParameterValue("gate"), *parameters.getRawParameterValue("gateBypass"));
    effects[1] = std::make_unique<Dist1Effect>(*parameters.getRawParameterValue("ds1Dist"),
        *parameters.getRawParameterValue("ds1Tone"), *parameters.getRawParameterValue("ds1Level"),
        *parameters.getRawParameterValue("ds1Bypass"));
    effects[2] = std::make_unique<ToneEffect>(*parameters.getRawParameterValue("bass"), *parameters.getRawParameterValue("mid"),
                                            *parameters.getRawParameterValue("treble"), *parameters.getRawParameterValue("toneBypass"));
    effects[3] = std::make_unique<Mars8Effect>(*parameters.getRawParameterValue("jcmPreamp"),
        *parameters.getRawParameterValue("jcmBass"), *parameters.getRawParameterValue("jcmMiddle"),
        *parameters.getRawParameterValue("jcmTreble"), *parameters.getRawParameterValue("jcmMaster"),
        *parameters.getRawParameterValue("jcmPresence"), *parameters.getRawParameterValue("jcmSag"),
        *parameters.getRawParameterValue("jcmCab"), *parameters.getRawParameterValue("jcmBypass"));
    setPedalOrder({PedalId::gate, PedalId::ds1, PedalId::jcm800}, true);
}

StompForgeAudioProcessor::APVTS::ParameterLayout StompForgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gate", 1}, "Gate",
        juce::NormalisableRange<float>{-80.0f, -20.0f, 0.1f}, -55.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"drive", 1}, "Drive",
        juce::NormalisableRange<float>{0.0f, 30.0f, 0.1f}, 10.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 100.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bass", 1}, "Bass",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mid", 1}, "Mid",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"treble", 1}, "Treble",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output", 1}, "Output",
        juce::NormalisableRange<float>{-24.0f, 12.0f, 0.1f}, -3.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"gateBypass", 1}, "Gate Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"driveBypass", 1}, "Drive Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"toneBypass", 1}, "Tone Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ds1Dist", 1}, "DIST-1 Distortion",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 55.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ds1Tone", 1}, "DIST-1 Tone",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 50.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ds1Level", 1}, "DIST-1 Level",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 65.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"ds1Bypass", 1}, "DIST-1 Bypass", false));
    auto addJcm = [&p] (const char* id, const char* name, float initial) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, initial)); };
    addJcm("jcmPreamp", "MARS-8 Preamp", 65.0f); addJcm("jcmBass", "MARS-8 Bass", 55.0f);
    addJcm("jcmMiddle", "MARS-8 Middle", 65.0f); addJcm("jcmTreble", "MARS-8 Treble", 55.0f);
    addJcm("jcmMaster", "MARS-8 Master", 55.0f); addJcm("jcmPresence", "MARS-8 Presence", 50.0f);
    addJcm("jcmSag", "MARS-8 Power Sag", 45.0f);
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"jcmCab", 1}, "MARS-8 Cab", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"jcmBypass", 1}, "MARS-8 Bypass", false));
    return {p.begin(), p.end()};
}

bool StompForgeAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && out == layouts.getMainInputChannelSet();
}

void StompForgeAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(samplesPerBlock),
                               static_cast<juce::uint32>(getTotalNumOutputChannels())};
    for (auto& effect : effects) effect->prepare(spec);
    outputGain.reset(sampleRate, 0.02);
}

void StompForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const auto order = getPedalOrder();
    for (auto id : order) effects[static_cast<size_t>(id)]->process(buffer);
    outputGain.setTargetValue(juce::Decibels::decibelsToGain(parameters.getRawParameterValue("output")->load()));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        buffer.applyGain(sample, 1, outputGain.getNextValue());
}

void StompForgeAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void StompForgeAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size); xml && xml->hasTagName(parameters.state.getType())) {
        const auto savedXml = xml->toString();
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
        // Old sessions predate the DIST-1/MARS-8 parameters. Some hosts restore
        // missing APVTS children as zero, which makes Level or Master mute the
        // module. Restore only genuinely absent parameters to their declared
        // defaults; explicit user zero values remain untouched.
        constexpr std::array<const char*, 13> addedParameters {
            "ds1Dist", "ds1Tone", "ds1Level", "ds1Bypass",
            "jcmPreamp", "jcmBass", "jcmMiddle", "jcmTreble", "jcmMaster",
            "jcmPresence", "jcmSag", "jcmCab", "jcmBypass" };
        for (const auto* id : addedParameters)
            if (!savedXml.contains("id=\"" + juce::String(id) + "\""))
                if (auto* parameter = parameters.getParameter(id))
                    parameter->setValueNotifyingHost(parameter->getDefaultValue());
        const auto text = parameters.state.getProperty("pedalOrder", "0,1,2").toString();
        juce::StringArray values; values.addTokens(text, ",", "");
        if (values.size() == 3) {
            std::array<PedalId, 3> restored{};
            for (int i = 0; i < 3; ++i) restored[static_cast<size_t>(i)] = static_cast<PedalId>(values[i].getIntValue());
            setPedalOrder(restored, false);
        }
    }
}

juce::uint32 StompForgeAudioProcessor::packOrder(const std::array<PedalId, 3>& order) noexcept
{
    return static_cast<juce::uint32>(order[0]) | (static_cast<juce::uint32>(order[1]) << 2u)
        | (static_cast<juce::uint32>(order[2]) << 4u);
}

std::array<StompForgeAudioProcessor::PedalId, 3> StompForgeAudioProcessor::getPedalOrder() const noexcept
{
    const auto value = packedOrder.load(std::memory_order_acquire);
    return {static_cast<PedalId>(value & 3u), static_cast<PedalId>((value >> 2u) & 3u),
            static_cast<PedalId>((value >> 4u) & 3u)};
}

void StompForgeAudioProcessor::setPedalOrder(const std::array<PedalId, 3>& order, bool saveToState)
{
    std::array<bool, 4> seen{};
    for (auto id : order) {
        const auto index = static_cast<size_t>(id);
        if (index >= seen.size() || seen[index]) return;
        seen[index] = true;
    }
    packedOrder.store(packOrder(order), std::memory_order_release);
    if (saveToState)
        parameters.state.setProperty("pedalOrder", juce::String(static_cast<int>(order[0])) + ","
            + juce::String(static_cast<int>(order[1])) + "," + juce::String(static_cast<int>(order[2])), nullptr);
}

void StompForgeAudioProcessor::movePedal(PedalId dragged, int targetSlot)
{
    auto order = getPedalOrder();
    const auto from = static_cast<int>(std::find(order.begin(), order.end(), dragged) - order.begin());
    targetSlot = juce::jlimit(0, 2, targetSlot);
    if (from == targetSlot || from >= 3) return;
    const auto item = order[static_cast<size_t>(from)];
    if (from < targetSlot)
        std::move(order.begin() + from + 1, order.begin() + targetSlot + 1, order.begin() + from);
    else
        std::move_backward(order.begin() + targetSlot, order.begin() + from, order.begin() + from + 1);
    order[static_cast<size_t>(targetSlot)] = item;
    setPedalOrder(order, true);
}

void StompForgeAudioProcessor::replacePedal(int slot, PedalId replacement)
{
    auto order = getPedalOrder(); slot = juce::jlimit(0, 2, slot);
    const auto existing = std::find(order.begin(), order.end(), replacement);
    if (existing != order.end()) std::swap(order[static_cast<size_t>(slot)], *existing);
    else order[static_cast<size_t>(slot)] = replacement;
    setPedalOrder(order, true);
}

juce::AudioProcessorEditor* StompForgeAudioProcessor::createEditor() { return new StompForgeAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StompForgeAudioProcessor(); }
