# PenguinMIDI VST — Design Spec
Date: 2026-05-31

## What We're Building

PenguinMIDI as a VST2 + VST3 plugin for Windows. The UI is the existing web prototype (`packages/penguin-midi/web-prototype/index.html`) running inside a JUCE 8 WebView2 component. The plugin outputs MIDI to the DAW track — whatever instrument is loaded on the track (e.g. Serum) makes the sound.

## Plugin Format

- **Formats:** VST2 + VST3 (Windows)
- **Type:** Instrument (synth flag = true) — outputs MIDI notes to DAW
- **JUCE version:** 8.0.13 (already in repo)
- **Target DAW:** MPC2 (Akai) — VST2 required

## Architecture

Three layers:

1. **UI layer** — `index.html` (the web prototype) runs inside `juce::WebBrowserComponent` (WebView2). No changes to layout, CSS, grid, drum machine, generate logic, edit mode, or export MIDI.

2. **Bridge layer** — JUCE 8 native function handlers wire JS calls to C++:
   - `midiNoteOn(ch, note, vel, durationMs)` — adds NoteOn + NoteOff to MIDI output buffer
   - `getBpm()` — returns current DAW host BPM from `AudioPlayHead`
   - `savePattern(name, data)` — writes JSON to `%APPDATA%/PenguinSuite/midi-patterns/`
   - `loadPatterns()` — reads all saved pattern JSON files and returns array

3. **Processor layer** — `PluginProcessor` handles MIDI output buffer, reads host BPM/transport, manages file I/O.

## What Changes in index.html

Only three things change from the web prototype:

| Feature | Web Prototype | VST |
|---|---|---|
| Sound playback | Web Audio API (`playNote`, `playDrum`) | Bridge call → DAW MIDI output |
| BPM | Manual input | Auto-read from DAW host, editable as override |
| Pattern storage | `localStorage` | Bridge call → files in `%APPDATA%/PenguinSuite/` |

Everything else is untouched: grid UI, drum machine, scale/key picker, generate/density/feel, edit mode, note picker, export MIDI (.mid download), note length pills, all CSS.

## What's Deferred

- **Hum / Listen** — microphone access via `getUserMedia()` inside WebView2 inside a DAW host is unreliable across hosts. Deferred to v1.1.

## File Structure

```
packages/penguin-midi/
  CMakeLists.txt
  Source/
    PluginProcessor.h / .cpp   ← MIDI output, BPM read, file I/O
    PluginEditor.h / .cpp      ← WebBrowserComponent, bridge wiring
    BridgeHandler.h / .cpp     ← JS↔C++ function handlers
  web/
    index.html                 ← web prototype (modified: 3 bridge calls)
    (copied from web-prototype/ and modified)
```

## Drum MIDI Mapping

Drum machine outputs on MIDI channel 10 (GM drum standard):
- Kick → note 36 (C2)
- Snare → note 38 (D2)
- Hi-hat closed → note 42 (F#2)
- Hi-hat open → note 46 (A#2)
- Clap → note 39 (D#2)

Melody outputs on MIDI channel 1.

## Techno Preset

Per suite requirement, a Techno drum style is added alongside the existing Boom Bap / House / Pop / D&B styles:
- BPM: 138
- Kick: four-on-the-floor with eighth-note offbeats
- Hi-hat: 16th-note grid, full
- Snare: beats 2 and 4 with ghost notes

## Success Criteria

- Plugin loads in MPC2 as VST2 and VST3
- Opening the plugin shows the identical UI from the web prototype
- Clicking Generate fills the grid; clicking Play sends MIDI notes to the instrument on the track
- BPM syncs from DAW on open
- Patterns save and reload correctly across DAW sessions
- Export MIDI downloads a valid .mid file
