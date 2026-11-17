# User guide — current preview

This is a practical guide to the features that are implemented in the current build. Planned product features are listed in [ROADMAP.md](ROADMAP.md), not described here as if they already work.

## Build and launch

Build the core with CMake and a C++20 compiler, or build the Qt preview if Qt 6 Widgets is installed. The Qt application is an editor preview; it does **not** currently record or play audio through devices. See the exact commands in the top-level README.

## Open and edit a script

Choose **File → Open Script…**. UTF-8 (with or without BOM) and UTF-16 LE/BE with BOM are decoded to UTF-8. The source Unicode text, including Indic characters and Latin diacritics, is preserved. Plain text, Markdown, and basic HTML are supported. Markdown heading lines are exposed as chapter metadata; HTML tags are stripped. Other selected formats currently show an explicit unsupported-format message rather than pretending to parse them.

The core `decode_script` utility also accepts byte strings directly, making encoding and Unicode behaviour testable without launching the UI.

## Import a WAV and edit

Choose **File → Import Audio…** and select a WAV. The preview displays the waveform; click and drag to select a sample range.

- `S` splits the EDL at the cursor.
- `Delete` performs a non-destructive ripple delete over the selected range.
- `Ctrl+L` silences the selected range without removing its duration.
- `Ctrl+Z` / `Ctrl+Y` undo and redo EDL changes.
- **File → Export WAV…** renders the current edit decision list to a WAV.

The core library also exposes trim, duplicate, join adjacent source-contiguous clips, insert silence, markers, PCM 16/24/32 and float WAV support. Tests assert exact sample values around edits. The UI currently offers only WAV import/export and does not yet play the waveform.

## Save project

**File → Save Project…** writes `.ndproj` JSON with project metadata, script reference, track metadata, and markers. Media is currently referenced by path rather than packaged into the project; project reopen and full edit-list serialization are still roadmap work.

## Current UI

The preview has a draggable script/waveform splitter, swap and stacked-layout toggles, a dark theme, keyboard actions, a waveform selection and a status bar. Settings/layout persistence, track/mixer controls, device selection, accessibility review and the first-run wizard remain future work.

## Planned beginner workflow

The product target is to choose a mic and output once, then press Record; read the script while recording, mark mistakes, edit with shortcuts, apply effects, mix ambience, and export. Real device capture/playback, pre-roll, punch-in, effects UI, mixer, multi-format export and optional local speech sync are not part of the current preview. Consult `ROADMAP.md` for scope and implementation status.
