#include "PluginProcessor.h"
#include "PluginEditor.h"

StompForgeAudioProcessor::StompForgeAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    effects[0] = std::make_unique<StargateEffect>(*parameters.getRawParameterValue("gate"), *parameters.getRawParameterValue("gateBypass"));
    effects[1] = std::make_unique<Deimos1Effect>(*parameters.getRawParameterValue("ds1Dist"),
        *parameters.getRawParameterValue("ds1Tone"), *parameters.getRawParameterValue("ds1Level"),
        *parameters.getRawParameterValue("ds1Bypass"));
    effects[2] = std::make_unique<FrequencyEffect>(*parameters.getRawParameterValue("bass"), *parameters.getRawParameterValue("mid"),
                                            *parameters.getRawParameterValue("treble"), *parameters.getRawParameterValue("toneBypass"));
    effects[3] = std::make_unique<Mars8Effect>(*parameters.getRawParameterValue("jcmPreamp"),
        *parameters.getRawParameterValue("jcmBass"), *parameters.getRawParameterValue("jcmMiddle"),
        *parameters.getRawParameterValue("jcmTreble"), *parameters.getRawParameterValue("jcmMaster"),
        *parameters.getRawParameterValue("jcmPresence"), *parameters.getRawParameterValue("jcmSag"),
        *parameters.getRawParameterValue("jcmCab"), *parameters.getRawParameterValue("jcmBypass"));
    effects[4] = std::make_unique<Ceres2Effect>(*parameters.getRawParameterValue("chorusRate"),
        *parameters.getRawParameterValue("chorusDepth"), *parameters.getRawParameterValue("chorusMix"),
        *parameters.getRawParameterValue("chorusBypass"));
    effects[5] = std::make_unique<VoidChamberEffect>(*parameters.getRawParameterValue("reverbSize"),
        *parameters.getRawParameterValue("reverbDamping"), *parameters.getRawParameterValue("reverbMix"),
        *parameters.getRawParameterValue("reverbBypass"));
    effects[6] = std::make_unique<PulsarEffect>(*parameters.getRawParameterValue("delayTime"),
        *parameters.getRawParameterValue("delayFeedback"), *parameters.getRawParameterValue("delayMix"),
        *parameters.getRawParameterValue("delayBypass"));
    auto tuner = std::make_unique<LunerEffect>(*parameters.getRawParameterValue("tunerBypass"));
    luner = tuner.get(); effects[7] = std::move(tuner);
    effects[8] = std::make_unique<Vulcan5Effect>(*parameters.getRawParameterValue("5150Channel"),
        *parameters.getRawParameterValue("5150Gain"), *parameters.getRawParameterValue("5150Bass"),
        *parameters.getRawParameterValue("5150Middle"), *parameters.getRawParameterValue("5150Treble"),
        *parameters.getRawParameterValue("5150Master"), *parameters.getRawParameterValue("5150Presence"),
        *parameters.getRawParameterValue("5150Resonance"), *parameters.getRawParameterValue("5150Bias"),
        *parameters.getRawParameterValue("5150Sag"), *parameters.getRawParameterValue("5150Cab"),
        *parameters.getRawParameterValue("5150Bypass"));
    auto cabPlayer = std::make_unique<ImpulseCabEffect>(*parameters.getRawParameterValue("irLevel"),
        *parameters.getRawParameterValue("irLowCut"), *parameters.getRawParameterValue("irHighCut"),
        *parameters.getRawParameterValue("irMix"), *parameters.getRawParameterValue("irBypass"));
    impulseCab = cabPlayer.get(); effects[9] = std::move(cabPlayer);
    setPedalOrder({PedalId::gate, PedalId::ds1, PedalId::chorus, PedalId::jcm800,
                   PedalId::reverb, PedalId::delay, PedalId::empty, PedalId::empty,
                   PedalId::empty, PedalId::empty, PedalId::empty, PedalId::empty}, true);
}

StompForgeAudioProcessor::APVTS::ParameterLayout StompForgeAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"gate", 1}, "STARGATE Threshold",
        juce::NormalisableRange<float>{-80.0f, -20.0f, 0.1f}, -55.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"drive", 1}, "Drive",
        juce::NormalisableRange<float>{0.0f, 30.0f, 0.1f}, 10.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 100.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"bass", 1}, "FREQUENCY Bass",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"mid", 1}, "FREQUENCY Mid",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"treble", 1}, "FREQUENCY Treble",
        juce::NormalisableRange<float>{-12.0f, 12.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"input", 1}, "Input Gain",
        juce::NormalisableRange<float>{-24.0f, 24.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"output", 1}, "Output",
        juce::NormalisableRange<float>{-24.0f, 12.0f, 0.1f}, -3.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"gateBypass", 1}, "STARGATE Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"driveBypass", 1}, "Drive Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"toneBypass", 1}, "FREQUENCY Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ds1Dist", 1}, "DEIMOS-1 Distortion",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 55.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ds1Tone", 1}, "DEIMOS-1 Tone",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 50.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ds1Level", 1}, "DEIMOS-1 Level",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 65.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"ds1Bypass", 1}, "DEIMOS-1 Bypass", false));
    auto addJcm = [&p] (const char* id, const char* name, float initial) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, initial)); };
    addJcm("jcmPreamp", "MARS-8 Preamp", 65.0f); addJcm("jcmBass", "MARS-8 Bass", 55.0f);
    addJcm("jcmMiddle", "MARS-8 Middle", 65.0f); addJcm("jcmTreble", "MARS-8 Treble", 55.0f);
    addJcm("jcmMaster", "MARS-8 Master", 55.0f); addJcm("jcmPresence", "MARS-8 Presence", 50.0f);
    addJcm("jcmSag", "MARS-8 Power Sag", 45.0f);
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"jcmCab", 1}, "MARS-8 Cab", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"jcmBypass", 1}, "MARS-8 Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"5150Channel", 1},
        "VULCAN-5 Channel", juce::StringArray{"CLEAN", "CRUNCH", "LEAD"}, 2));
    auto add5150 = [&p] (const char* id, const char* name, float initial) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, initial)); };
    add5150("5150Gain", "VULCAN-5 Gain", 68.0f); add5150("5150Bass", "VULCAN-5 Bass", 48.0f);
    add5150("5150Middle", "VULCAN-5 Middle", 42.0f); add5150("5150Treble", "VULCAN-5 Treble", 58.0f);
    add5150("5150Master", "VULCAN-5 Master", 52.0f); add5150("5150Presence", "VULCAN-5 Presence", 55.0f);
    add5150("5150Resonance", "VULCAN-5 Resonance", 58.0f); add5150("5150Bias", "VULCAN-5 Bias", 22.0f);
    add5150("5150Sag", "VULCAN-5 Power Supply", 35.0f);
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"5150Cab", 1}, "VULCAN-5 Cab", true));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"5150Bypass", 1}, "VULCAN-5 Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"irLevel", 1}, "IMPULSE Level",
        juce::NormalisableRange<float>{-24.0f, 12.0f, 0.1f}, 0.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"irLowCut", 1}, "IMPULSE Low Cut",
        juce::NormalisableRange<float>{20.0f, 500.0f, 1.0f, 0.4f}, 70.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"irHighCut", 1}, "IMPULSE High Cut",
        juce::NormalisableRange<float>{2000.0f, 22000.0f, 1.0f, 0.4f}, 12000.0f));
    p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"irMix", 1}, "IMPULSE Mix",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 100.0f));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"irBypass", 1}, "IMPULSE Bypass", false));
    auto addPercent = [&p] (const char* id, const char* name, float initial) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, name,
            juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, initial)); };
    addPercent("chorusRate", "CERES-2 Rate", 35.0f); addPercent("chorusDepth", "CERES-2 Depth", 55.0f);
    addPercent("chorusMix", "CERES-2 Mix", 50.0f);
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"chorusBypass", 1}, "CERES-2 Bypass", false));
    addPercent("reverbSize", "VOID CHAMBER Size", 45.0f); addPercent("reverbDamping", "VOID CHAMBER Damping", 55.0f);
    addPercent("reverbMix", "VOID CHAMBER Mix", 24.0f);
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"reverbBypass", 1}, "VOID CHAMBER Bypass", false));
    addPercent("delayTime", "PULSAR Time", 28.0f); addPercent("delayFeedback", "PULSAR Feedback", 32.0f);
    addPercent("delayMix", "PULSAR Mix", 22.0f);
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"delayBypass", 1}, "PULSAR Bypass", false));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"tunerBypass", 1}, "LUNER Bypass", false));
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
    inputGain.reset(sampleRate, 0.02);
    inputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("input")->load()));
    outputGain.reset(sampleRate, 0.02);
    outputGain.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("output")->load()));
}

void StompForgeAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // A guitar interface normally uses input 1. Standalone hosts expose a
    // stereo bus even when input 2 is unconnected, so discard that noisy
    // hardware channel and centre the mono guitar signal before processing.
    if (wrapperType == wrapperType_Standalone && buffer.getNumChannels() >= 2)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

    inputGain.setTargetValue(juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("input")->load()));
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        buffer.applyGain(sample, 1, inputGain.getNextValue());

    const auto order = getPedalOrder();
    for (auto id : order)
        if (id != PedalId::empty) effects[static_cast<size_t>(id)]->process(buffer);
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
        // Old sessions predate the DEIMOS-1/MARS-8 parameters. Some hosts restore
        // missing APVTS children as zero, which makes Level or Master mute the
        // module. Restore only genuinely absent parameters to their declared
        // defaults; explicit user zero values remain untouched.
        constexpr std::array<const char*, 44> addedParameters {
            "ds1Dist", "ds1Tone", "ds1Level", "ds1Bypass",
            "jcmPreamp", "jcmBass", "jcmMiddle", "jcmTreble", "jcmMaster",
            "jcmPresence", "jcmSag", "jcmCab", "jcmBypass",
            "chorusRate", "chorusDepth", "chorusMix", "chorusBypass",
            "reverbSize", "reverbDamping", "reverbMix", "reverbBypass",
            "delayTime", "delayFeedback", "delayMix", "delayBypass", "tunerBypass", "input",
            "5150Channel", "5150Gain", "5150Bass", "5150Middle", "5150Treble", "5150Master",
            "5150Presence", "5150Resonance", "5150Bias", "5150Sag", "5150Cab", "5150Bypass",
            "irLevel", "irLowCut", "irHighCut", "irMix", "irBypass" };
        for (const auto* id : addedParameters)
            if (!savedXml.contains("id=\"" + juce::String(id) + "\""))
                if (auto* parameter = parameters.getParameter(id))
                    parameter->setValueNotifyingHost(parameter->getDefaultValue());
        const auto text = parameters.state.getProperty("pedalOrder", "0,1,4,3,5,6").toString();
        juce::StringArray values; values.addTokens(text, ",", "");
        if (values.size() == 6 || values.size() == static_cast<int>(numSlots)) {
            std::array<PedalId, numSlots> restored{};
            restored.fill(PedalId::empty);
            for (size_t i = 0; i < static_cast<size_t>(values.size()); ++i)
                restored[i] = static_cast<PedalId>(values[static_cast<int>(i)].getIntValue());
            setPedalOrder(restored, false);
        } else {
            setPedalOrder({PedalId::gate, PedalId::ds1, PedalId::chorus, PedalId::jcm800,
                           PedalId::reverb, PedalId::delay, PedalId::empty, PedalId::empty,
                           PedalId::empty, PedalId::empty, PedalId::empty, PedalId::empty}, false);
        }
        const auto original = juce::File(parameters.state.getProperty("irOriginalPath").toString());
        const auto cached = juce::File(parameters.state.getProperty("irCachedPath").toString());
        if (original.existsAsFile()) loadCabImpulse(original);
        else if (cached.existsAsFile() && impulseCab != nullptr) impulseCab->loadImpulse(cached);
    }
}

bool StompForgeAudioProcessor::loadCabImpulse(const juce::File& source)
{
    if (impulseCab == nullptr || !source.existsAsFile()) return false;
    auto cache = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("StompForge").getChildFile("ImpulseResponses");
    if (cache.createDirectory().failed()) return false;
    const auto safeName = source.getFileNameWithoutExtension().retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_ ");
    const auto cached = cache.getChildFile(safeName + "-" + juce::String(source.getSize()) + source.getFileExtension());
    if (source != cached) {
        cached.deleteFile();
        if (!source.copyFileTo(cached)) return false;
    }
    impulseCab->loadImpulse(cached);
    parameters.state.setProperty("irOriginalPath", source.getFullPathName(), nullptr);
    parameters.state.setProperty("irCachedPath", cached.getFullPathName(), nullptr);
    parameters.state.setProperty("irDisplayName", source.getFileNameWithoutExtension(), nullptr);
    return true;
}

juce::String StompForgeAudioProcessor::getCabImpulseName() const
{
    return parameters.state.getProperty("irDisplayName", "NO IR LOADED").toString();
}

int StompForgeAudioProcessor::getGridRows() const noexcept
{
    const auto rows = juce::jlimit(1, 3, static_cast<int>(parameters.state.getProperty("gridRows", 2)));
    const auto columns = juce::jlimit(1, 4, static_cast<int>(parameters.state.getProperty("gridColumns", 3)));
    return rows;
}

int StompForgeAudioProcessor::getGridColumns() const noexcept
{
    const auto rows = juce::jlimit(1, 3, static_cast<int>(parameters.state.getProperty("gridRows", 2)));
    const auto columns = juce::jlimit(1, 4, static_cast<int>(parameters.state.getProperty("gridColumns", 3)));
    return columns;
}

bool StompForgeAudioProcessor::setGridSize(int rows, int columns)
{
    rows = juce::jlimit(1, 3, rows); columns = juce::jlimit(1, 4, columns);
    const auto capacity = rows * columns;
    auto order = getPedalOrder();
    std::array<PedalId, numSlots> compacted{}; compacted.fill(PedalId::empty);
    std::array<PedalId, numEffects> activeEffects{}; size_t activeIndex = 0;
    for (auto id : order) if (id != PedalId::empty) activeEffects[activeIndex++] = id;
    activeIndex = juce::jmin(activeIndex, static_cast<size_t>(capacity));
    for (size_t i = 0; i < activeIndex; ++i) {
        const auto position = activeIndex <= 1 ? 0 : juce::roundToInt(
            i * static_cast<float>(capacity - 1) / static_cast<float>(activeIndex - 1));
        compacted[static_cast<size_t>(position)] = activeEffects[i];
    }
    setPedalOrder(compacted, true);
    parameters.state.setProperty("gridRows", rows, nullptr);
    parameters.state.setProperty("gridColumns", columns, nullptr);
    return true;
}

juce::uint64 StompForgeAudioProcessor::packOrder(const std::array<PedalId, numSlots>& order) noexcept
{
    juce::uint64 packed = 0;
    for (size_t i = 0; i < numSlots; ++i)
        packed |= static_cast<juce::uint64>(order[i]) << static_cast<juce::uint64>(i * 4u);
    return packed;
}

std::array<StompForgeAudioProcessor::PedalId, StompForgeAudioProcessor::numSlots>
StompForgeAudioProcessor::getPedalOrder() const noexcept
{
    const auto value = packedOrder.load(std::memory_order_acquire);
    std::array<PedalId, numSlots> order {};
    for (size_t i = 0; i < numSlots; ++i)
        order[i] = static_cast<PedalId>((value >> static_cast<juce::uint64>(i * 4u)) & 15u);
    return order;
}

void StompForgeAudioProcessor::setPedalOrder(const std::array<PedalId, numSlots>& order, bool saveToState)
{
    std::array<bool, numEffects> seen{};
    for (auto id : order) {
        if (id == PedalId::empty) continue;
        const auto index = static_cast<size_t>(id);
        if (index >= seen.size() || seen[index]) return;
        seen[index] = true;
    }
    packedOrder.store(packOrder(order), std::memory_order_release);
    if (saveToState) {
        juce::StringArray values;
        for (auto id : order) values.add(juce::String(static_cast<int>(id)));
        parameters.state.setProperty("pedalOrder", values.joinIntoString(","), nullptr);
    }
}

void StompForgeAudioProcessor::movePedal(int sourceSlot, int targetSlot)
{
    auto order = getPedalOrder();
    sourceSlot = juce::jlimit(0, static_cast<int>(numSlots) - 1, sourceSlot);
    targetSlot = juce::jlimit(0, static_cast<int>(numSlots) - 1, targetSlot);
    if (sourceSlot == targetSlot) return;
    std::swap(order[static_cast<size_t>(sourceSlot)], order[static_cast<size_t>(targetSlot)]);
    setPedalOrder(order, true);
}

void StompForgeAudioProcessor::replacePedal(int slot, PedalId replacement)
{
    auto order = getPedalOrder(); slot = juce::jlimit(0, static_cast<int>(numSlots) - 1, slot);
    const auto existing = std::find(order.begin(), order.end(), replacement);
    if (existing != order.end()) std::swap(order[static_cast<size_t>(slot)], *existing);
    else order[static_cast<size_t>(slot)] = replacement;
    setPedalOrder(order, true);
}

void StompForgeAudioProcessor::clearPedal(int slot)
{
    auto order = getPedalOrder();
    slot = juce::jlimit(0, static_cast<int>(numSlots) - 1, slot);
    order[static_cast<size_t>(slot)] = PedalId::empty;
    setPedalOrder(order, true);
}

StompForgeAudioProcessor::TunerState StompForgeAudioProcessor::getTunerState() const noexcept
{
    if (luner == nullptr) return {};
    return {luner->getMidiNote(), luner->getCents(), luner->getFrequency(), luner->getConfidence()};
}

juce::AudioProcessorEditor* StompForgeAudioProcessor::createEditor() { return new StompForgeAudioProcessorEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new StompForgeAudioProcessor(); }
