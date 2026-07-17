#pragma once

#include <juce_core/juce_core.h>

namespace stompforge
{
class AssetRepository final
{
public:
    static juce::File getRootDirectory();

    // Copies a user-selected file into storage owned by StompForge. On iOS this
    // is the App Group container shared by the standalone app and AUv3 process.
    static juce::File importFile(const juce::File& source,
                                 const juce::String& category,
                                 juce::String& error);

private:
    static juce::String makeSafeFileName(const juce::File& source);
};
}
