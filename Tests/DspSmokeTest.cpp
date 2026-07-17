#include "Effects.h"
#include <iostream>
#include <chrono>

static juce::AudioBuffer<float> makeSignal()
{
    juce::AudioBuffer<float> b(2, 4096);
    for (int i = 0; i < b.getNumSamples(); ++i) {
        const auto x = 0.12f * std::sin(juce::MathConstants<float>::twoPi * 220.0f * i / 48000.0f);
        b.setSample(0, i, x); b.setSample(1, i, x);
    }
    return b;
}

static bool valid(const char* name, const juce::AudioBuffer<float>& b)
{
    const auto rms = b.getRMSLevel(0, 512, b.getNumSamples() - 512);
    const auto peak = b.getMagnitude(0, b.getNumSamples());
    std::cout << name << " RMS=" << rms << " peak=" << peak << '\n';
    return std::isfinite(rms) && rms > 1.0e-6f && rms < 10.0f;
}

int main(int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    juce::dsp::ProcessSpec spec { 48000.0, 4096, 2 };
    std::atomic<float> dist{55}, tone{50}, level{65}, off{0};
    Deimos1Effect ds1(dist, tone, level, off); ds1.prepare(spec);
    auto a = makeSignal(); ds1.process(a);

    std::atomic<float> pre{65}, bass{55}, mid{65}, treble{55}, master{55};
    std::atomic<float> presence{50}, sag{45}, cab{1};
    std::atomic<bool> cabSuppressed{false};
    Mars8Effect amp(pre, bass, mid, treble, master, presence, sag, cab, off, &cabSuppressed); amp.prepare(spec);
    auto b = makeSignal(); amp.process(b);
    std::atomic<float> ampChannel{2}, ampGain{68}, ampBass{48}, ampMid{42}, ampTreble{58};
    std::atomic<float> ampMaster{52}, ampPresence{55}, resonance{58}, bias{22}, supply{35};
    Vulcan5Effect vulcan(ampChannel, ampGain, ampBass, ampMid, ampTreble, ampMaster,
        ampPresence, resonance, bias, supply, cab, off, &cabSuppressed); vulcan.prepare(spec);
    auto c = makeSignal(); vulcan.process(c);
    std::atomic<float> irLevel{0}, irLowCut{20}, irHighCut{22000}, irMix{100};
    ImpulseCabEffect impulse(irLevel, irLowCut, irHighCut, irMix, off);
    juce::AudioBuffer<float> deltaIr(2, 64); deltaIr.clear();
    deltaIr.setSample(0, 0, 1.0f); deltaIr.setSample(1, 0, 1.0f);
    impulse.loadImpulse(std::move(deltaIr), 48000.0); impulse.prepare(spec);
    auto irSignal = makeSignal(); impulse.process(irSignal);
    std::atomic<float> modelerInput{0}, modelerOutput{0}, modelerMix{100};
    ModelerEffect modeler(modelerInput, modelerOutput, modelerMix, off); modeler.prepare(spec);
    auto modelerSignal = makeSignal();
    const auto modelerReference = makeSignal();
    modeler.process(modelerSignal);
    const auto unloadedModelerOk = modelerSignal.getMagnitude(0, modelerSignal.getNumSamples())
        == modelerReference.getMagnitude(0, modelerReference.getNumSamples());
    juce::String modelerError;
    const auto modelPath = argc > 1 ? juce::File(argv[1]) : juce::File(NAM_TEST_MODEL_PATH);
    const auto modelerLoaded = modeler.loadModel(modelPath, modelerError);
    auto loadedModelerSignal = makeSignal();
    if (modelerLoaded) modeler.process(loadedModelerSignal);
    const auto loadedModelerOk = modelerLoaded && valid("MODELER loaded model", loadedModelerSignal);
    if (!modelerLoaded) std::cout << "MODELER load failed: " << modelerError << '\n';
    auto chain = makeSignal();
    ds1.reset(); amp.reset(); ds1.process(chain); amp.process(chain);

    std::atomic<float> chorusRate{35}, chorusDepth{55}, chorusMix{35};
    std::atomic<float> reverbSize{45}, reverbDamping{55}, reverbMix{24};
    std::atomic<float> delayTime{28}, delayFeedback{32}, delayMix{22};
    Ceres2Effect chorus(chorusRate, chorusDepth, chorusMix, off); chorus.prepare(spec);
    VoidChamberEffect reverb(reverbSize, reverbDamping, reverbMix, off); reverb.prepare(spec);
    PulsarEffect delay(delayTime, delayFeedback, delayMix, off); delay.prepare(spec);
    auto sixModuleChain = makeSignal();
    ds1.reset(); amp.reset();
    ds1.process(sixModuleChain); chorus.process(sixModuleChain); amp.process(sixModuleChain);
    reverb.process(sixModuleChain); delay.process(sixModuleChain);

    LunerEffect luner(off); luner.prepare(spec);
    juce::AudioBuffer<float> tunerSignal(2, 8192);
    for (int i = 0; i < tunerSignal.getNumSamples(); ++i) {
        const auto sample = 0.15f * std::sin(juce::MathConstants<float>::twoPi * 110.0f * i / 48000.0f);
        tunerSignal.setSample(0, i, sample); tunerSignal.setSample(1, i, sample);
    }
    luner.process(tunerSignal);
    const auto tunerOk = luner.getMidiNote() == 45 && std::abs(luner.getCents()) < 2.0f
        && luner.getConfidence() >= 0.55f;
    std::cout << "LUNER note=" << luner.getMidiNote() << " cents=" << luner.getCents()
              << " confidence=" << luner.getConfidence() << '\n';
    auto quietSignal = [] (float amplitude) {
        juce::AudioBuffer<float> buffer(2, 4096);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            const auto sample = amplitude * std::sin(juce::MathConstants<float>::twoPi * 375.0f * i / 48000.0f);
            buffer.setSample(0, i, sample); buffer.setSample(1, i, sample);
        }
        return buffer;
    };
    pre.store(0.0f); cab.store(0.0f); amp.reset();
    auto marsQuietA = quietSignal(0.001f); amp.process(marsQuietA); amp.reset();
    auto marsQuietB = quietSignal(0.0005f); amp.process(marsQuietB);
    ampChannel.store(0.0f); ampGain.store(68.0f); vulcan.reset();
    auto vulcanQuietA = quietSignal(0.001f); vulcan.process(vulcanQuietA); vulcan.reset();
    auto vulcanQuietB = quietSignal(0.0005f); vulcan.process(vulcanQuietB);
    const auto quietRatio = [] (const auto& loud, const auto& quiet) {
        return loud.getRMSLevel(0, 2048, 2048) / juce::jmax(1.0e-12f, quiet.getRMSLevel(0, 2048, 2048));
    };
    const auto marsQuietRatio = quietRatio(marsQuietA, marsQuietB);
    const auto vulcanQuietRatio = quietRatio(vulcanQuietA, vulcanQuietB);
    const auto quietLinearityOk = marsQuietRatio > 1.9f && marsQuietRatio < 2.1f
        && vulcanQuietRatio > 1.9f && vulcanQuietRatio < 2.1f;
    std::cout << "quiet-signal ratios MARS-8=" << marsQuietRatio
              << " VULCAN-5=" << vulcanQuietRatio << '\n';
    pre.store(65.0f); cab.store(1.0f); ampChannel.store(2.0f);
    bool shortBlocksOk = true;
    juce::dsp::ProcessSpec shortSpec { 44100.0, 32, 2 };
    Deimos1Effect shortDs1(dist, tone, level, off); shortDs1.prepare(shortSpec);
    Ceres2Effect shortCeres(chorusRate, chorusDepth, chorusMix, off); shortCeres.prepare(shortSpec);
    Mars8Effect shortAmp(pre, bass, mid, treble, master, presence, sag, cab, off, &cabSuppressed); shortAmp.prepare(shortSpec);
    Vulcan5Effect shortVulcan(ampChannel, ampGain, ampBass, ampMid, ampTreble, ampMaster,
        ampPresence, resonance, bias, supply, cab, off, &cabSuppressed); shortVulcan.prepare(shortSpec);
    const auto started = std::chrono::steady_clock::now();
    constexpr int numBlocks = 4096;
    float minimumBlockRms = std::numeric_limits<float>::max();
    float minimumDs1Rms = std::numeric_limits<float>::max();
    float maximumBlockPeak = 0.0f;
    int firstSilentBlock = -1;
    float lastBlockRms = 0.0f;
    for (int n = 0; n < numBlocks; ++n) {
        juce::AudioBuffer<float> shortBuffer(2, 32);
        for (int i = 0; i < 32; ++i) {
            const auto sample = 0.12f * std::sin(0.07f * (i + n * 32));
            shortBuffer.setSample(0, i, sample); shortBuffer.setSample(1, i, sample);
        }
        shortDs1.process(shortBuffer);
        if (n > 128)
            minimumDs1Rms = juce::jmin(minimumDs1Rms, shortBuffer.getRMSLevel(0, 0, 32));
        shortCeres.process(shortBuffer);
        shortAmp.process(shortBuffer);
        shortVulcan.process(shortBuffer);
        const auto blockRms = shortBuffer.getRMSLevel(0, 0, 32);
        const auto blockPeak = shortBuffer.getMagnitude(0, 32);
        lastBlockRms = blockRms;
        if (n > 128 && blockRms <= 1.0e-5f && firstSilentBlock < 0)
            firstSilentBlock = n;
        if (n > 128)
            minimumBlockRms = juce::jmin(minimumBlockRms, blockRms);
        maximumBlockPeak = juce::jmax(maximumBlockPeak, blockPeak);
        shortBlocksOk &= std::isfinite(blockPeak) && (n <= 128 || blockRms > 1.0e-5f) && blockPeak < 10.0f;
    }
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    const auto realtimeDuration = numBlocks * 32.0 / 44100.0;
    std::cout << "32-sample stereo blocks=" << (shortBlocksOk ? "OK" : "FAILED")
              << " DSP load=" << (100.0 * elapsed / realtimeDuration) << "%"
              << " min DEIMOS-1 RMS=" << minimumDs1Rms << " min RMS=" << minimumBlockRms
              << " max peak=" << maximumBlockPeak << '\n';
    std::cout << "first silent block=" << firstSilentBlock << " last RMS=" << lastBlockRms << '\n';
    return valid("DEIMOS-1", a) && valid("MARS-8", b) && valid("VULCAN-5", c)
        && valid("IMPULSE delta IR", irSignal)
        && unloadedModelerOk && loadedModelerOk
        && valid("DEIMOS-1 -> MARS-8", chain)
        && valid("six-module chain", sixModuleChain) && tunerOk && quietLinearityOk && shortBlocksOk ? 0 : 1;
}
