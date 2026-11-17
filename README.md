# NandaDhvani

A C++20 foundation for a Windows audiobook and podcast studio. The product goal is a beginner-friendly script-and-waveform recorder with optional advanced editing; the current checkout provides a **working audio-editing core** and a Qt desktop preview when Qt 6 is available.

## What works today

- C++20 dependency-free core; builds on Windows, Linux, and macOS with CMake.
- Non-destructive sample-accurate EDL clips: split, ripple delete, silence, trim, duplicate, insert silence, markers, undo/redo, rendering.
- Float WAV and PCM 16/24/32 read/write; streamed float WAV writer periodically updates headers for interrupted-session recovery (classic RIFF files remain limited to 4 GB).
- Lock-free SPSC ring-buffer primitive for a real-time audio callback/worker boundary.
- DSP primitives: peak/RMS, peak normalization, fades, biquad low-pass/notch, compressor, hum notches, basic sample-domain gate, equal-power crossfade.
- Script UTF-8/UTF-16 decoding (including BOM), Unicode preservation, plaintext/Markdown chapter headings, and basic HTML tag stripping.
- Token alignment utility with fuzzy word matching, timestamps, missing/different/repeated annotations.
- `.ndproj` JSON persistence for project metadata, tracks, and markers.
- Unit tests for EDL sample correctness/undo, WAV round-trip, DSP, Unicode decoding, alignment, streaming writer, SPSC and project persistence.
- Qt 6 Widgets preview (optional): dark split script/waveform layout, WAV import, waveform selection, basic edit actions and WAV export.

## Not yet implemented

**This is not yet a production recorder or a complete Windows audio editor.** The Qt preview does not yet capture or play through audio devices; Record/Play controls explain that limitation. It supports WAV rather than the requested full codec set, and only the basic text importers above are wired. First-run device wizard, live meters/monitoring, crash recovery UI, full importer suite (DOCX/ODT/PDF/EPUB/RTF), bundled Noto fonts, complete effect UX, multi-track mixer, chapter export, whisper.cpp, and signed/release-ready Windows packaging remain future work. CI includes an unverified preview installer/portable-package scaffold; test the generated artifacts before distributing. See [docs/ROADMAP.md](docs/ROADMAP.md).

## Build

Core + tests (no third-party libraries required):

```sh
cmake -S . -B build -DND_BUILD_UI=OFF -DND_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Qt desktop preview (Qt 6 Widgets required):

```sh
cmake -S . -B build -DND_BUILD_UI=ON -DND_BUILD_TESTS=ON
cmake --build build
```

Windows preset (requires Visual Studio 2022, CMake, Ninja and vcpkg; set `VCPKG_ROOT`):

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
ctest --preset windows-release --output-on-failure
```

Linux CI preset: `cmake --preset linux-release`, then build and test. Qt is optional and skipped when absent.

## Architecture and docs

- [docs/USER_GUIDE.md](docs/USER_GUIDE.md) — current preview usage
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) — implemented core and future target boundaries
- [docs/DECISIONS.md](docs/DECISIONS.md) — design choices and deferred dependency evaluation
- [docs/ROADMAP.md](docs/ROADMAP.md) — plan and honest implementation status
- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) — dependencies actually used

## Privacy and license

No telemetry or network code. Whisper recognition is not implemented. MIT; see [LICENSE](LICENSE). Qt is an optional, dynamically linked LGPL dependency.
