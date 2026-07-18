# Apple mobile build

StompForge uses the same JUCE processor and editor for the iOS/iPadOS
standalone app and its embedded AUv3 extension. CMake enables these two targets
when `CMAKE_SYSTEM_NAME=iOS`; Windows continues to build VST3 and Standalone.

## Prerequisites

- macOS with a current Xcode installation;
- CMake 3.22 or newer;
- an Apple Development team for physical-device builds;
- registered identifiers matching the CMake cache values:
  - app bundle: `STOMPFORGE_IOS_BUNDLE_ID`;
  - App Group: `STOMPFORGE_IOS_APP_GROUP_ID`.

The defaults are `audio.stompforge.app` and
`group.audio.stompforge.shared`. Change them during configuration if those
identifiers belong to another Apple Developer account.

## Configure

Simulator builds do not require signing:

```bash
cmake -S . -B build-ios \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

cmake --build build-ios \
  --config Debug \
  --target StompForge_Standalone \
  -- -sdk iphonesimulator
```

For a physical device, add the development team at configure time:

```bash
cmake -S . -B build-ios \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY="Apple Development" \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM="<TEAM_ID>"
```

Building `StompForge_Standalone` also builds and embeds
`StompForge_AUv3`. CMake generates the app and extension entitlements from the
shared App Group configuration.

The current mobile UI is landscape-only. MODELER is intentionally unavailable
on iOS/iPadOS; its pedal remains disabled in the effect menu until the NAM
backend has a supported mobile integration.

## Shared assets

Files selected by the user are imported immediately:

- IR files go to `StompForge/ImpulseResponses`;
- NAM models go to `StompForge/NAMModels`.

On iOS these directories live in the App Group container, so the standalone
app and the out-of-process AUv3 extension can both restore them. Other
platforms use the normal StompForge application-data directory.

## Release gates not covered by a Windows build

- replace or validate the App Store icon; Apple distribution rejects icon
  artwork with an alpha channel;
- exercise audio-session interruptions and route changes;
- verify touch drag-and-drop, long press and all supported window sizes;
- profile NAM and oversampled effects on physical iPhone and iPad hardware;
- test multiple AUv3 instances in at least one production host;
- archive and validate the containing app in Xcode.
