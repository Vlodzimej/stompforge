# Changelog

All notable StompForge changes are recorded here. The format follows Keep a
Changelog and the project uses Semantic Versioning.

## [Unreleased]

No changes yet.

## [0.0.2-alpha] — 2026-07-17

The second alpha expands the cross-platform architecture and touch-device
workflow. The binary and installer version is `0.0.2`.

### Added

- iOS/iPadOS CMake targets for a Standalone app with an embedded AUv3
  extension.
- App Group storage shared by the iOS app and AUv3 for NAM models and cabinet
  IR files; other platforms use the managed StompForge application-data
  directory.
- Apple build instructions, signing settings and platform release gates.
- Touch-oriented effect menus, adaptive pedalboard grid and landscape
  iPhone/iPad layout.
- Buffer-size selection and Standalone state persistence on iOS/iPadOS.
- Linked-channel NAM processing coverage in the DSP smoke test.

### Changed

- The default chain starts with LUNER, STARGATE, DEIMOS-1 and VULCAN-5; VOID
  CHAMBER and PULSAR start bypassed.
- Standalone can process the common mono input through one NAM instance and
  route its result to both output channels.
- Pedal parameter labels now match their controls.
- MODELER is explicitly unavailable on iOS/iPadOS pending a supported mobile
  backend.
- The estimated `LATENCY` label was removed while buffer-size selection
  remains available.

### Fixed

- Security-scoped URL loading and managed import of selected files.
- Finite-output checks and safe handling of an absent NAM channel model.
- Windows compilation of the `JUCE_IOS` platform guard.

## [0.0.1-alpha] — 2026-07-17

The first public alpha of the modular StompForge guitar processor.

### Added

- Configurable 1x1–4x3 pedalboard with twelve physical slots, empty cells,
  drag-and-drop and persistent state.
- Dynamic, Distortion, Modulation, Amp, Reverb, Delay, EQ, Cabs and Utils
  categories.
- STARGATE, DEIMOS-1, FREQUENCY, CERES-2, MARS-8, VULCAN-5, VOID CHAMBER,
  PULSAR and LUNER.
- IMPULSE WAV/AIFF loader and MODELER based on NeuralAmpModelerCore v0.5.4.
- Windows x64 VST3, Standalone and Inno Setup installer.

### Fixed

- Retained NAM architecture parser translation units during VST3 and
  Standalone linking.
- Prevented gradual signal loss in nonlinear modules.
- Routed Standalone hardware input 1 to both channels.
- Restored defaults for parameters absent from older DAW sessions.
