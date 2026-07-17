#include "AssetRepository.h"

#ifndef STOMPFORGE_APP_GROUP_ID
 #define STOMPFORGE_APP_GROUP_ID ""
#endif

namespace stompforge
{
juce::File AssetRepository::getRootDirectory()
{
   #if JUCE_IOS
    const auto sharedContainer =
        juce::File::getContainerForSecurityApplicationGroupIdentifier(STOMPFORGE_APP_GROUP_ID);

    if (sharedContainer.getFullPathName().isNotEmpty())
        return sharedContainer.getChildFile("StompForge");

    return {};
   #else
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("StompForge");
   #endif
}

juce::String AssetRepository::makeSafeFileName(const juce::File& source)
{
    auto stem = source.getFileNameWithoutExtension()
        .retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_ ");

    if (stem.isEmpty())
        stem = "Imported";

    return stem + "-" + juce::String(source.getSize()) + "-"
        + juce::String(source.getLastModificationTime().toMilliseconds())
        + source.getFileExtension().toLowerCase();
}

juce::File AssetRepository::importFile(const juce::File& source,
                                       const juce::String& category,
                                       juce::String& error)
{
    if (!source.existsAsFile())
    {
        error = "The selected file does not exist.";
        return {};
    }

    const auto root = getRootDirectory();
    if (root.getFullPathName().isEmpty())
    {
        error = "The StompForge App Group container is unavailable. Check the app entitlement.";
        return {};
    }

    const auto directory = root.getChildFile(category);
    const auto createResult = directory.createDirectory();
    if (createResult.failed())
    {
        error = "Unable to create the StompForge asset directory: "
            + createResult.getErrorMessage();
        return {};
    }

    const auto destination = directory.getChildFile(makeSafeFileName(source));
    if (source == destination)
        return destination;

    if (destination.existsAsFile() && !destination.deleteFile())
    {
        error = "Unable to replace the previously imported asset.";
        return {};
    }

    if (!source.copyFileTo(destination))
    {
        error = "Unable to copy the selected file into StompForge storage.";
        return {};
    }

    return destination;
}

juce::File AssetRepository::importURL(const juce::URL& source,
                                      const juce::String& category,
                                      juce::String& error)
{
    if (!source.isLocalFile())
    {
        error = "The selected asset is not a local file.";
        return {};
    }

    auto input = source.createInputStream(
        juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress));
    if (input == nullptr)
    {
        error = "Unable to access the selected file. Download it locally in Files and try again.";
        return {};
    }

    const auto root = getRootDirectory();
    if (root.getFullPathName().isEmpty())
    {
        error = "The StompForge App Group container is unavailable. Check the app entitlement.";
        return {};
    }

    const auto directory = root.getChildFile(category);
    const auto createResult = directory.createDirectory();
    if (createResult.failed())
    {
        error = "Unable to create the StompForge asset directory: "
            + createResult.getErrorMessage();
        return {};
    }

    const auto sourceFile = source.getLocalFile();
    auto stem = sourceFile.getFileNameWithoutExtension()
        .retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_ ");
    if (stem.isEmpty())
        stem = "Imported";
    const auto destination = directory.getChildFile(
        stem + "-" + juce::String(input->getTotalLength()) + "-"
        + juce::String::toHexString(source.toString(false).hashCode64())
        + sourceFile.getFileExtension().toLowerCase());

    if (destination.existsAsFile() && !destination.deleteFile())
    {
        error = "Unable to replace the previously imported asset.";
        return {};
    }

    juce::FileOutputStream output(destination);
    const auto bytesWritten = output.failedToOpen() ? -1 : output.writeFromInputStream(*input, -1);
    output.flush();
    if (bytesWritten <= 0 || output.getStatus().failed())
    {
        destination.deleteFile();
        error = "Unable to copy the selected file into StompForge storage.";
        return {};
    }

    return destination;
}
}
