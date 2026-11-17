# Third-party notices

## Dependencies currently used

| Component | URL | License | Version | Use |
|---|---|---|---|---|
| CMake | https://github.com/Kitware/CMake | BSD-3-Clause | 3.20+ | Build system |
| Qt 6 Widgets (optional) | https://github.com/qt/qtbase | LGPL-3.0 / GPL-3.0 / commercial; this project requires the LGPL-compatible shared build | 6.5+ | Optional desktop preview; dynamically linked |

The core library and tests have **no third-party runtime or test dependencies**. CI uses GitHub-hosted runners and the `actions/checkout`, `lukka/run-vcpkg`, `ilammy/msvc-dev-cmd`, `lukka/run-cmake`, `Minionguyjpro/Inno-Setup-Action`, and `actions/upload-artifact` GitHub Actions for automation; these are build-time workflow dependencies, not bundled runtime code.

## Planned, not currently linked

miniaudio (WASAPI), libsndfile/FFmpeg, libsamplerate/SpeexDSP, SoundTouch, libebur128, whisper.cpp, RNNoise/DeepFilterNet, PDFium, Noto OFL fonts, and CC0 ambience assets are **not currently used or included**. Before adding any, verify the exact repository/license and maintenance status, pin the version, and update this notice and `docs/DECISIONS.md`. GPL Audacity source was not copied; Rubber Band and JUCE were not adopted.

Qt is only linked when found, as a shared library. Qt DLLs and the corresponding LGPL notices must accompany any Windows distribution of the preview.
