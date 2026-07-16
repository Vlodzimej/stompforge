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

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    juce::dsp::ProcessSpec spec { 48000.0, 4096, 2 };
    std::atomic<float> dist{55}, tone{50}, level{65}, off{0};
    Deimos1Effect ds1(dist, tone, level, off); ds1.prepare(spec);
    auto a = makeSignal(); ds1.process(a);

    std::atomic<float> pre{65}, bass{55}, mid{65}, treble{55}, master{55};
    std::atomic<float> presence{50}, sag{45}, cab{1};
    Mars8Effect amp(pre, bass, mid, treble, master, presence, sag, cab, off); amp.prepare(spec);
    auto b = makeSignal(); amp.process(b);
    auto chain = makeSignal();
    ds1.reset(); amp.reset(); ds1.process(chain); amp.process(chain);

    std::atomic<float> chorusRate{35}, chorusDepth{55}, chorusMix{35};
    std::atomic<float> reverbSize{45}, reverbDamping{55}, reverbMix{24};
    std::atomic<float> delayTime{28}, delayFeedback{32}, delayMix{22};
    Ceres2Effect chorus(chorusRate, chorusDepth, chorusMix, off); chorus.prepare(spec);
    ReverbEffect reverb(reverbSize, reverbDamping, reverbMix, off); reverb.prepare(spec);
    DelayEffect delay(delayTime, delayFeedback, delayMix, off); delay.prepare(spec);
    auto sixModuleChain = makeSignal();
    ds1.reset(); amp.reset();
    ds1.process(sixModuleChain); chorus.process(sixModuleChain); amp.process(sixModuleChain);
    reverb.process(sixModuleChain); delay.process(sixModuleChain);
    bool shortBlocksOk = true;
    juce::dsp::ProcessSpec shortSpec { 44100.0, 32, 2 };
    Deimos1Effect shortDs1(dist, tone, level, off); shortDs1.prepare(shortSpec);
    Ceres2Effect shortCeres(chorusRate, chorusDepth, chorusMix, off); shortCeres.prepare(shortSpec);
    Mars8Effect shortAmp(pre, bass, mid, treble, master, presence, sag, cab, off); shortAmp.prepare(shortSpec);
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
    return valid("DEIMOS-1", a) && valid("MARS-8", b) && valid("DEIMOS-1 -> MARS-8", chain)
        && valid("six-module chain", sixModuleChain) && shortBlocksOk ? 0 : 1;
}
