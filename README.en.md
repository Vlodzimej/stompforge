# StompForge

![StompForge icon](Assets/stompforge-icon.png)

[Русский](README.md) | English

**Alpha 0.0.2** is an early test release of the modular guitar audio processor
by Vlodzimej Garlic. Verified Windows x64 builds are available as VST3 and
Standalone. The source tree also contains experimental iOS/iPadOS Standalone
and AUv3 targets. Do not rely on this alpha as the only processor in a critical
recording or live setup.

## Features

- Pedalboard matrix from 1x1 to 4x3 with empty cells and drag-and-drop.
- Up to three main controls per pedal; additional parameters are under `MORE`.
- Input and output faders with live level meters.
- DAW state persistence for parameters, grid geometry and effect order.
- Matching mono input/output and stereo input/output layouts.
- WAV/AIFF cabinet impulse response loading.
- Neural Amp Modeler `.nam` loading with host sample-rate conversion.
- Protection against enabling an amp CABSIM and IMPULSE at the same time.
- Touch-oriented landscape UI and buffer-size selection on iOS/iPadOS
  Standalone.
- Managed IR/NAM storage, shared by the iOS app and AUv3 through an App Group.

## Modules

- **STARGATE** — noise gate.
- **DEIMOS-1** — circuit-inspired distortion with 4x oversampling.
- **FREQUENCY** — three-band EQ.
- **CERES-2** — BBD-inspired chorus.
- **MARS-8** — British-style tube amp with switchable 4x12 cabsim.
- **VULCAN-5** — Clean/Crunch/Lead amp with a 6L6-inspired power stage.
- **MODELER** — `.nam` player based on NeuralAmpModelerCore.
- **IMPULSE** — cabinet IR player with Low Cut, High Cut, Level and Mix.
- **VOID CHAMBER** — algorithmic reverb.
- **PULSAR** — digital delay.
- **LUNER** — pass-through chromatic tuner.

MODELER does not ship with amplifier models. Users are responsible for their
right to use and distribute selected `.nam` models and impulse responses.

## Windows installation

The installer places:

- the VST3 bundle in the system Common Files/VST3 directory;
- the Standalone application in Program Files/StompForge.

Restart the DAW or rescan plug-ins after installation. In Standalone, select
the audio interface, guitar input, output and a safe buffer size. Start at a
low monitoring volume.

## Alpha limitations

- The verified release platform is Windows 10/11 x64.
- iOS/iPadOS targets have not yet been signed or tested on physical hardware.
- MODELER is temporarily unavailable on iOS/iPadOS.
- `.nam` models load synchronously and a large model may briefly block the UI.
- Imported `.nam` and IR files are managed by StompForge but are not embedded
  into the DAW project state.
- Preset management and automated recovery of missing external assets are not
  implemented yet.
- Standalone expects the guitar on hardware input 1.
- Release binaries are currently unsigned.

## Windows build

Requirements: Git, CMake 3.22+ and Visual Studio 2022 with the
`Desktop development with C++` workload.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

To explicitly enable ASIO for a local Standalone build:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 `
  -DASIO_SDK_PATH="C:\SDKs\asiosdk"
cmake --build build --config Release
```

Public 0.0.2-alpha release binaries are built without the ASIO SDK. The
artifacts are generated under:

- `build/StompForge_artefacts/Release/VST3/StompForge.vst3`
- `build/StompForge_artefacts/Release/Standalone/StompForge.exe`

Run the offline DSP validation with:

```powershell
build\StompForgeDspSmokeTest.exe
```

## iOS/iPadOS build

The iOS build requires macOS and the Xcode generator. It produces a Standalone
container app with an embedded AUv3 extension:

```bash
cmake -S . -B build-ios \
  -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0

cmake --build build-ios --config Debug --target StompForge_Standalone \
  -- -sdk iphonesimulator
```

A physical-device build also requires an Apple Development Team and registered
bundle/App Group identifiers. See
[Platforms/Apple/README.md](Platforms/Apple/README.md).

## Licensing

StompForge is free software licensed under the
[GNU Affero General Public License v3 or later](LICENSE).

NeuralAmpModelerCore and the selected NAM components use permissive MIT terms;
Eigen is primarily MPL-2.0. JUCE 8 is used through its AGPLv3 open-source
route. See [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) for required
notices. This summary is not legal advice.
