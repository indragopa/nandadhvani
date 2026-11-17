# Decisions

## Scope and honesty

The source prompt describes a full Windows recording studio; an empty repository cannot responsibly be called complete after only a skeleton. This branch establishes a buildable core and a small Qt editing preview. Deferred features are explicitly identified in `ROADMAP.md` and the UI reports device recording/playback limitations rather than simulating them.

## Current architecture

- **C++20 + CMake.** The core has no third-party runtime dependency; Linux CI and bare compiler tests work without Qt or vcpkg.
- **Qt 6 Widgets (optional, shared).** Provides the preview UI when present; shared linking is selected to satisfy the LGPL-compatible distribution model. The full desktop workflow is not yet wired.
- **Non-destructive EDL.** Clips reference immutable `AudioBuffer` sources, edits update timeline mapping, and undo/redo store snapshots. Render sums clips at sample precision; tests check expected samples after a cut.
- **WAV formats.** Core read/write supports classic RIFF PCM 8/16/24/32 and 32-bit float. A worker-side streamed float writer periodically updates RIFF headers for interrupted recordings. RF64 and multi-hour/over-4-GB files are not supported yet.
- **Real-time foundation only.** `SpscRingBuffer` is a lock-free SPSC queue; the app does not yet connect it to a WASAPI/miniaudio callback or a recorder.
- **DSP fallback primitives.** Own small biquad, compressor, fade, peak-normalise, hum-notch and sample-domain gate implementations. The gate is not a spectral-profile denoiser, and the compressor is not a mastered loudness/true-peak chain.
- **Unicode.** UTF-8 is preserved; UTF-16 LE/BE BOM inputs convert to UTF-8. Markdown/HTML/plaintext support is modest; complex import formats and bundled fonts are deferred.
- **Alignment.** Token-level fuzzy Needleman–Wunsch helper is implemented, but there is no Whisper recognizer or UI, and Indic tokenisation is currently a coarse byte-span fallback. No audio leaves the machine because no network or recognition code is present.
- **Project format.** `.ndproj` stores metadata/tracks/markers as JSON. It does not yet bundle audio or persist the full EDL.
- **Licensing.** No Audacity/GPL code is copied. The only optional runtime dependency is Qt 6 shared. Evaluate every future dependency and record the exact version/license before linking it.

## Deliberately not chosen yet

Audio backend, compressed codecs, neural denoisers, time stretch, PDF/Office import, Whisper, installer dependency deployment, and VST3 hosting need a deliberate license/maintenance and Windows integration review before adoption. Do not treat names in the original product prompt as already-selected or included dependencies.
