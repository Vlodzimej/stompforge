# StompForge 0.0.2-alpha release checklist

[Русский](RELEASE_CHECKLIST.md)

Audit date: 2026-07-17. This is an engineering checklist, not legal advice.

## 1. Blocking license decisions

- [x] The AGPL-3.0-or-later open-source JUCE route is selected.
- [x] The matching `LICENSE` is present at the repository root.
- [ ] An ASIO proprietary or GPLv3 distribution route has been confirmed.
- [x] Because ASIO distribution is not confirmed, public release binaries must
  be rebuilt without `ASIO_SDK_PATH`.

## 2. Dependency compatibility

| Component | License | Result |
|---|---|---|
| NeuralAmpModelerCore v0.5.4 | MIT | Compatible when notices are retained |
| AudioDSPTools | MIT and embedded notices | Compatible when notices are retained |
| Eigen | Primarily MPL-2.0 | Allowed in a larger work; retain notices |
| nlohmann/json | MIT | Compatible when notice is retained |
| JUCE 8.0.10 | AGPLv3 or JUCE 8 EULA | AGPLv3 route selected |
| JUCE VST3 SDK | MIT | Compatible |
| ASIO SDK, when enabled | Steinberg proprietary or GPLv3 | Not included in this public release |

## 3. Content rights

- [ ] Rights to `Assets/stompforge-icon-source.ai` are confirmed.
- [x] No third-party `.nam` or IR files are shipped without permission.
- [x] README files state that users are responsible for selected model and IR
  licenses.

## 4. Technical validation

- [x] Clean Windows x64 Release configuration without the ASIO SDK.
- [x] VST3 and Standalone version 0.0.2 built successfully.
- [x] `StompForgeDspSmokeTest.exe` exits with code 0.
- [x] WaveNet `.nam` and linked-channel processing are validated.
- [x] Mono/stereo, bypass, varying block sizes and NaN/Inf safety are checked.
- [x] `git diff --check` reports no errors.
- [x] `build/`, `dist/` and `.tools/` are not tracked.
- [x] iOS/iPadOS targets are clearly experimental and are not distributed as
  binaries in this release.

## 5. Packaging

- [x] Installer built from `Packaging/StompForge.iss` using Inno Setup 6.7.3.
- [ ] Installer tested on a clean Windows machine or VM.
- [ ] VST3 rescan tested in a supported DAW.
- [x] `THIRD_PARTY_NOTICES.md` and `LICENSE` are included in installer and
  portable package.
- [x] Installer is named
  `StompForge-0.0.2-alpha-Windows-x64-Setup.exe`.
- [x] Portable package is named
  `StompForge-0.0.2-alpha-Windows-x64-Portable.zip`.
- [x] SHA-256 values are recorded in `SHA256SUMS.txt`.

## 6. Publication

- [x] Russian and English documentation matches the release.
- [x] Russian and English changelogs match the release notes.
- [x] Annotated tag `v0.0.2-alpha` created.
- [x] GitHub Release marked as a pre-release.
- [x] Installer, portable package, checksums and known limitations attached.
- [x] Matching source archive retained.
