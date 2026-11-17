# Implementation plan and roadmap

## Initial plan and repository layout

The checkout started empty except for a 14-byte README. The implementation plan is incremental and buildable:

1. **Foundation:** C++20/CMake/vcpkg/CI, strict core/UI/IO boundaries, licensing record, testable core, optional Qt shell.
2. **Audio editing core:** sample-accurate EDL, undo/redo, WAV codec, crash-safe stream writer, SPSC queue, DSP primitives and tests.
3. **Script and projects:** Unicode-aware import, chapter hints, alignment helper, basic project persistence.
4. **Desktop application:** Qt split-pane waveform preview, import/edit/export UI.
5. **Recording:** WASAPI device layer, first-run mic/output wizard, live capture/playback, meters, crash recovery UI.
6. **Full editing and production:** format suite, complete importers, effect rack, multi-track mixer, loudness/ACX checks, chapter export.
7. **Optional features and packaging:** consent-gated local Whisper, alignment UI, signed installer/portable package, accessibility and localization.

## Current status

- [x] C++20 core + CMake presets and standalone tests; Linux core build passes.
- [x] Sample-based non-destructive EDL: split, ripple delete, silence, trim, duplicate, insert silence, markers, undo/redo and rendering.
- [x] Float/PCM WAV read/write and streamed float writer with periodically updated headers. Classic RIFF only; no RF64 or >4 GB support.
- [x] Lock-free SPSC ring-buffer primitive; not yet connected to an audio callback.
- [x] DSP primitives: measurements, peak normalization, fades, biquad lowpass/notch, compressor, hum notches, sample-domain gate, crossfade.
- [x] UTF-8/UTF-16 BOM decoding, Unicode preservation, plaintext/Markdown/basic HTML, and token-alignment helper.
- [x] `.ndproj` JSON metadata/tracks/markers save/load.
- [x] Optional Qt 6 preview with split panes, waveform selection, WAV import, basic EDL actions, WAV export.
- [x] Unit tests pass locally: `ctest --test-dir build --output-on-failure`.
- [ ] WAV playback and live audio capture via WASAPI/miniaudio; device picker, hotplug, first-run wizard, monitoring, pre-roll/count-in, punch-in, takes and recovery UI.
- [ ] RF64, chunked/multi-hour rendering and peak mipmaps.
- [ ] Complete import suite (RTF/HTML rendering/DOCX/ODT/PDF/EPUB), encoding override, chapter nav, bundled Noto fonts and rich read-along tools.
- [ ] EDL/media bundling and project reopen/autosave/recovery.
- [ ] Complete effects (preview/presets/A-B, profile spectral denoise, hiss/AI denoise, EQ, limiter, reverb, de-esser, tempo/pitch), track racks and worker progress.
- [ ] Multi-track mixer, automation, ducking, ambience library, loudness/true-peak meter and ACX report.
- [ ] MP3/AAC/FLAC/OGG/Opus export, per-chapter splits, tags, podcast package and embedded chapter metadata.
- [ ] Whisper.cpp opt-in consent/model-download/hash flow, speech recognition, Indic-language quality review, word alignment UI, remapping and transcript exports.
- [ ] Installer dependency deployment verification, portable ZIP, signed release, accessibility and localization QA.

No row is considered done just because a type or documentation heading exists. The current preview is useful for exercising the core but is not yet a production recorder; see README for precise limitations.
