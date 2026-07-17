# StompForge 0.0.2-alpha

[Русский](RELEASE_NOTES_0.0.2-alpha.md)

The second StompForge alpha improves the iPhone/iPad project foundation and
touch workflow while retaining the verified Windows x64 VST3 and Standalone
formats.

## Highlights

- Experimental iOS/iPadOS Standalone and AUv3 targets in the shared CMake
  project.
- Adaptive landscape pedalboard, touch effect menus and buffer-size selection.
- App Group storage shared by the containing app and AUv3.
- Automatic iOS Standalone state persistence.
- Improved linked-channel NAM processing in Standalone.
- New default chain: LUNER, STARGATE, DEIMOS-1, VULCAN-5, VOID CHAMBER and
  PULSAR, with reverb and delay initially bypassed.
- Removed the estimated `LATENCY` label.

## Compatibility

- Release formats: Windows x64 VST3 and Standalone.
- Existing APVTS parameter IDs remain stable.
- MODELER remains available on Windows VST3/Standalone and is temporarily
  disabled on iOS/iPadOS.
- Selected IR/NAM files are imported into managed StompForge storage.

## Known limitations

- This is a test alpha and is not intended for critical live use.
- iOS/iPadOS targets have not been signed or tested on physical devices.
- Loading a large `.nam` file may briefly block the UI.
- Binaries are not digitally signed.
- Public Windows binaries are built without the ASIO SDK.

## Release files

- `StompForge-0.0.2-alpha-Windows-x64-Setup.exe`
- `StompForge-0.0.2-alpha-Windows-x64-Portable.zip`
- `SHA256SUMS.txt`

StompForge and its corresponding source are distributed under
AGPL-3.0-or-later.
