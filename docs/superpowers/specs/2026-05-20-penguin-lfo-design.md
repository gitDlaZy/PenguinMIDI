# PenguinLFO — Design Spec

**Date:** 2026-05-20
**Status:** Approved

## Overview

PenguinLFO is a VST2 audio effect plugin for MPC 2 on Windows. It runs 4 independent LFOs simultaneously, each modulating a different audio parameter at a different rate. The goal is to turn sustained sounds (pads, synths) into rhythmically animated, percussive textures — in one click via presets.

Install path: `C:\Program Files\Steinberg\vstplugins`

## Architecture

**Stack:** JUCE (C++), VST2, compiled via Docker (CMake)

**Project structure:**
```
src/
  penguin-lfo/
    CMakeLists.txt
    Source/
      PluginProcessor.cpp   ← audio engine, LFO math
      PluginProcessor.h
      PluginEditor.cpp      ← UI
      PluginEditor.h
    Presets/
      presets.json          ← factory preset definitions
    JUCE/                   ← JUCE submodule
```

**Data flow:**
```
Audio in
  → LFO 1 (shape, rate, target, depth)
  → LFO 2 (shape, rate, target, depth)
  → LFO 3 (shape, rate, target, depth)
  → LFO 4 (shape, rate, target, depth)
  → combined modulation applied per sample
→ Audio out
```

**Build flow:**
```
docker-compose run app (CMake build) → PenguinLFO.dll
→ copy to C:\Program Files\Steinberg\vstplugins
```

## LFO Engine

Each of the 4 LFOs has 4 independent parameters:

### Shape
- Sine
- Square
- Saw up
- Saw down
- Triangle
- Sample & Hold (random step)

### Rate (BPM-synced to host)
```
Straight:  1/1  1/2  1/4  1/8  1/16  1/32  1/64
Dotted:    1/1d 1/2d 1/4d 1/8d 1/16d
Triplet:   1/1t 1/2t 1/4t 1/8t 1/16t 1/32t
Long:      2/1  4/1  8/1
Odd:       3/16  5/16  7/16
```
28 timing options total.

### Target (what the LFO modulates)
- Volume (tremolo / rhythmic gating)
- Filter cutoff (built-in low-pass filter sweep)
- Pan (left/right movement)
- Pitch (vibrato)

### Depth
0–100% — how strongly the LFO affects the target parameter.

Each LFO also has an on/off toggle. LFOs run independently; their combination creates polyrhythmic textures.

## Preset System

Presets are stored in `presets.json` bundled with the plugin. Each preset defines the full state of all 4 LFOs.

**Factory presets:**

| Name | Feel |
|---|---|
| Slow Breathe | Long sine volume pulse + subtle filter |
| Hard Gate | Square 1/8 volume chop |
| Stutter 16ths | Fast square gate + triplet filter flutter |
| Polyrhythm | 4 LFOs at 1/4, 1/8t, 3/16, 1/16d |
| Pan Drift | Slow sine pan + volume swell |
| Glitch | S&H volume + S&H filter at odd rates |
| Half Time Feel | Slow gate 1/2 + pitch vibrato 1/4t |

Users can save their own presets from within the plugin. User presets are stored separately from factory presets and persist across sessions.

## UI

Dark minimal interface. Preset selection is the primary action — everything else is secondary.

```
┌─────────────────────────────────────────┐
│  PenguinLFO          [Preset ▼] [Save]  │
├─────────────────────────────────────────┤
│  LFO 1  [Shape ▼] [Rate ▼] [Target ▼] [Depth] [ON]  │
│  LFO 2  [Shape ▼] [Rate ▼] [Target ▼] [Depth] [ON]  │
│  LFO 3  [Shape ▼] [Rate ▼] [Target ▼] [Depth] [ON]  │
│  LFO 4  [Shape ▼] [Rate ▼] [Target ▼] [Depth] [ON]  │
├─────────────────────────────────────────┤
│       waveform visualizer (combined)                 │
└─────────────────────────────────────────┘
```

- **Preset dropdown** — selecting a preset updates all 4 LFOs instantly
- **Per-LFO row** — shape, rate, target, depth, on/off toggle
- **Waveform visualizer** — shows combined LFO output (primarily volume modulation) in real time
- **Save button** — saves current state as a named user preset

## Build & Distribution

- Full build runs inside Docker via CMake
- Output: `PenguinLFO.dll` (VST2, 64-bit Windows)
- A small helper script copies the `.dll` to the VST folder after build
