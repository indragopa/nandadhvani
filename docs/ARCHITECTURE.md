# Architecture

## Repository layout

```
core/include/nandadhvani/core/   Dependency-free public C++20 APIs
core/src/                        Audio buffers, EDL, WAV, DSP, alignment, project, recorder writer
io/include/nandadhvani/io/       Script import APIs
core/src/script.cpp              UTF-8/UTF-16, Markdown headings, basic HTML/plaintext
ui/include/                      Optional Qt Widgets shell and waveform widget
ui/src/                          Main window, custom QPainter waveform, application entry point
tests/test_main.cpp              Standalone test harness, no test-framework dependency
docs/                            User guide, architecture, decisions and roadmap
.github/workflows/ci.yml         Windows build/test/package workflow
```

## Implemented core modules

| Module | Responsibility / current boundary |
|---|---|
| `AudioBuffer` | Interleaved float samples, sample rate/channels, checked frame access and slicing |
| `EditDecisionList` | Immutable source references, sample-based clips, split/ripple delete/silence/trim/duplicate/insert, markers, snapshot undo/redo and render |
| `WAV` | Classic RIFF PCM 8/16/24/32 and float32 read; PCM16/24/32 and float32 write |
| `CrashSafeWavWriter` | Worker-thread streaming float WAV writer with periodic RIFF header updates; not RF64 / >4 GB yet |
| `SpscRingBuffer` | Lock-free bounded single-producer/single-consumer queue template |
| `DSP` | Peak/RMS, peak normalisation, fade, biquad LP/notch, compressor, hum notch cascade, basic gate, equal-power crossfade |
| `Script` | BOM-aware UTF-8/UTF-16 decode, plaintext/Markdown/basic HTML; unsupported formats report explicitly |
| `Alignment` | Tokenization + fuzzy Needleman–Wunsch word alignment helper; no ASR integration |
| `Project` | `.ndproj` metadata/tracks/markers JSON read/write; not full EDL/media packaging |
| `Qt preview` | Optional split-pane script/waveform preview, WAV import, selection/edit actions and WAV export; no device capture/playback |

## Real-time boundaries

The ring buffer is ready to connect an audio callback to a worker, but no callback is currently present. `CrashSafeWavWriter` performs disk IO and must only be called by a worker, never by a real-time callback. The eventual backend must obey: no allocations, locks, file IO, or logging in the callback; use bounded queues and perform disk/render work on worker threads.

## Rendering and size limits

EDL positions are integer sample frames and render output is deterministic for a fixed source/EDL. The current renderer materializes the entire result in memory. The current WAV reader also materializes a whole file; classic RIFF is limited to 4 GB. Large-session chunked rendering, RF64, and multi-resolution peak cache are future work, not performance claims.

## Future target architecture

The intended full product keeps `core/` Qt-free, with separate format importers, a WASAPI audio backend, Qt UI, cancellable worker pool, effect graph, per-track mixer, and optional consent-gated local speech-recognition module. These components are roadmap items and must not be implied by the present APIs.

## Build and tests

```sh
cmake -S . -B build -DND_BUILD_UI=OFF -DND_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

See README for the optional Qt build and Windows presets.
