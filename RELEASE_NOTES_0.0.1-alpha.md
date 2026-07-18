# StompForge 0.0.1-alpha

The first public alpha release of the StompForge modular guitar processor.

## Highlights

- Windows x64 VST3 and Standalone formats.
- Pedalboard matrix up to 4x3 with drag-and-drop and empty cells.
- Eleven module types, including two amps, an IR player, a tuner and NAM
  MODELER.
- Up to three main controls per card, with advanced parameters under `MORE`.
- Vertical Input and Output faders with level meters.
- Effect-chain and parameter persistence in DAW sessions.

## MODELER

MODELER uses NeuralAmpModelerCore v0.5.4, loads mono-in/mono-out `.nam` files,
matches the host sample rate and maintains independent state for both stereo
channels. Models are not included in the package.

## Known limitations

- This is a test alpha release and is not intended for critical live use.
- `.nam` models and IRs remain external files and must be retained by the user.
- Loading a large `.nam` file may briefly delay the UI.
- The verified release platform is Windows 10/11 x64.
- The binaries are not digitally signed.

## Planned files

- `StompForge-0.0.1-alpha-Windows-x64-Setup.exe`
- `StompForge-0.0.1-alpha-Windows-x64-VST3.zip`
- `SHA256SUMS.txt`

StompForge and its corresponding source code are distributed under
AGPL-3.0-or-later. This alpha was built without the ASIO SDK.
