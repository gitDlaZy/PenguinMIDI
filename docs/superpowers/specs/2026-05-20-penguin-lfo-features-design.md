# PenguinLFO Feature Set Design

**Date:** 2026-05-20
**Status:** Approved

## Overview

Four feature additions to PenguinLFO:

1. **Smoothing** — per-LFO adjustable de-click fade (eliminates saw/square pops)
2. **Custom shape** — 7th LFO shape with breakpoint editor and step sequencer mode
3. **Frequency filter section** — static bandpass with LFO-sweepable range
4. **Pitch center offset** — bias control so "no pitch change" can sit anywhere in the LFO range

Plus a large batch of factory presets showcasing all features.

---

## 1. Data Model

### LFOInstance (additions)

```cpp
float smoothing    = 0.0f;   // 0–1, maps to 0–20ms fade at phase wrap
float pitchCenter  = 0.0f;   // -1–+1, bias for Pitch target only
CustomWaveform customWave;   // used when shape == LFOShape::Custom
```

### CustomWaveform struct (new)

```cpp
struct WaveNode { float x; float y; }; // x in [0,1], y in [-1,1]

struct CustomWaveform {
    bool  isStepMode  = false;
    int   stepCount   = 16;          // 8, 16, or 32
    float steps[32]   = {};          // y values in [-1,1], used in step mode
    std::vector<WaveNode> nodes;     // used in breakpoint mode; always sorted by x
};
```

### LFOShape enum (addition)

```cpp
enum class LFOShape { Sine, Square, SawUp, SawDown, Triangle, SampleAndHold, Custom };
```

### Processor-level filter params (new fields on PenguinLFOProcessor)

```cpp
float filterLowCut  = 20.0f;      // Hz, 20–20000
float filterHighCut = 20000.0f;   // Hz, 20–20000, must be >= filterLowCut
```

Serialized in `getStateInformation` / `setStateInformation`.

---

## 2. Audio Processing

### 2.1 Smoothing (de-click)

Applied in `lfoAdvance`. The smoothing value maps to a fade duration:

```
fadeMs   = smoothing * 20.0f
fadeSamples = fadeMs * sampleRate / 1000.0
```

At phase wrap (`phase >= 1.0`), a rising ramp is applied for `fadeSamples` samples after the wrap. A matching falling ramp is applied for `fadeSamples` samples before phase reaches 1.0 (detectable when `phase > 1.0 - fadePhase`).

Ramp multiplier is linear (0→1 on entry, 1→0 on exit). The `LFOInstance` needs one additional runtime field:

```cpp
float rampGain = 1.0f; // current ramp multiplier, runtime only (not serialized)
```

### 2.2 Custom waveform playback

**Breakpoint mode:** Linear interpolation between sorted nodes. Phase 0–1 maps to X axis. If no nodes exist, output 0. Edge cases: phase before first node or after last node clamps to nearest node's Y.

**Step mode:** `stepIndex = floor(phase * stepCount)`. Output is `steps[stepIndex]`. Steps also benefit from smoothing at the transition boundaries — the existing ramp logic covers this.

### 2.3 Filter section

The existing `filterLeft` / `filterRight` (`StateVariableTPTFilter`) currently runs as a single lowpass. This changes to a **two-filter chain** per channel:

- `hpLeft` / `hpRight` — highpass at `filterLowCut`
- `lpLeft` / `lpRight` — lowpass at `filterHighCut`

Signal flows: input → highpass → lowpass → output.

When an LFO targets `Filter`, it modulates the **lowpass cutoff** between `filterLowCut` and `filterHighCut`:

```
t = val * 0.5f + 0.5f  // remap [-1,1] → [0,1]
cutoff = filterLowCut * pow(filterHighCut / filterLowCut, t * depth + (1-depth)*0.5)
```

(Log-scale sweep so movement feels even across the spectrum.)

### 2.4 Pitch center offset

```cpp
float effectivePitch = (val + pitchCenter) * depth;
effectivePitch = jlimit(-0.95f, 0.95f, effectivePitch);
```

Replaces the current `pitchMod += val * lfos[i].depth` line.

---

## 3. UI Layout

### Plugin size

Grows from 520×320 to **520×560** to accommodate the expanded row area and filter section.

### LFO row (normal — shape ≠ Custom)

```
[LFO 1] [Shape▼] [Rate▼] [Target▼] [Depth ──────] [Smooth ──── 2.4ms] [ON]
```

When target = **Pitch**, an additional inline control appears:

```
[LFO 1] [Shape▼] [Rate▼] [Pitch▼] [Depth ──] [Center ── +0.50] [Smooth ── 2ms] [ON]
```

Center label displays the raw -1.0 to +1.0 value (2 decimal places). At +1.0 the LFO only goes upward; at -1.0 only downward; at 0.0 symmetric (default).

### LFO row (expanded — shape = Custom)

Row expands downward by ~120px:

```
[LFO 1] [Custom▼] [Rate▼] [Target▼] [Depth ──────] [Smooth ──── 2.4ms] [ON]
┌────────────────────────────────────────────────────────────┐
│  [Breakpoint] [Step: 16▼]   [Clear]                       │
│                                                            │
│  (node editor or step bars rendered here)                  │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

Mode toggle switches between Breakpoint and Step views. Step count dropdown: 8, 16, 32. Clear button resets to empty/flat.

Only one row can be expanded at a time. Selecting Custom on a second row collapses any other open editor.

### Filter section (bottom, always visible)

```
┌─── Filter ──────────────────────────────────────────────────────┐
│  Low Cut  [────────────] [  80 Hz]    High Cut [────────] [8000 Hz]  │
└─────────────────────────────────────────────────────────────────┘
```

Both sliders use a logarithmic scale (20–20000 Hz). The value labels are click-to-edit text boxes showing Hz. High Cut is clamped to always be ≥ Low Cut.

### Waveform visualizer (existing, additions)

- For Pitch LFOs: draw a thin horizontal dashed line at the pitch-center position to indicate "no pitch change".
- For Custom LFOs: the visualizer renders the actual custom waveform instead of a computed shape.

---

## 4. Preset Serialization

`PresetManager` JSON format additions per LFO:

```json
{
  "smoothing": 0.05,
  "pitchCenter": 0.0,
  "customWave": {
    "isStepMode": false,
    "stepCount": 16,
    "steps": [],
    "nodes": [{"x": 0.0, "y": -1.0}, {"x": 0.5, "y": 1.0}, {"x": 1.0, "y": -1.0}]
  }
}
```

Processor-level filter params added at preset root:

```json
{
  "filterLowCut": 80.0,
  "filterHighCut": 8000.0
}
```

**Backwards compatibility:** Old presets without these fields load fine. Missing fields default to `smoothing=0`, `pitchCenter=0`, empty custom waveform, `filterLowCut=20`, `filterHighCut=20000`.

---

## 5. Factory Presets

Minimum 15 new presets covering:

- Smooth volume tremolo (sine, low smoothing)
- Hard gated stutter (square, no smoothing vs. square with smoothing)
- Soft saw sweep up / down (saw + smoothing)
- Custom step rhythm patterns (step mode, 16 steps)
- Custom breakpoint "heartbeat" shape
- Filter sweep narrow / wide band
- Filter + volume combo
- Pitch vibrato centered
- Pitch vibrato biased up / biased down
- Pitch + filter combined
- Odd-timing triplet filter sweep
- Custom waveform with off-beat step pattern
- Deep pan + pitch
- Gentle all-four combo

---

## 6. Files Affected

| File | Change |
|---|---|
| `LFOEngine.h` | Add `Custom` to `LFOShape`, add `CustomWaveform` struct, add `smoothing`/`pitchCenter`/`rampGain` to `LFOInstance` |
| `LFOEngine.cpp` | Implement custom playback, smoothing ramp, pitch center in `lfoAdvance` |
| `PluginProcessor.h` | Add `filterLowCut`, `filterHighCut`, add hp filter objects |
| `PluginProcessor.cpp` | Two-filter chain, LFO filter sweep uses low/high range, pitch center applied |
| `PluginEditor.h/.cpp` | Smoothing slider + ms label, pitch center slider + st label, expandable custom waveform panel, filter section |
| `PresetManager.h/.cpp` | Serialize/deserialize new fields, backwards-compatible parsing |
| `Presets/presets.json` | 15+ new factory presets |

---

## 7. Testing

Build: open `build/PenguinLFO.sln` in Visual Studio, Ctrl+Shift+B. No CMake re-run needed unless `CMakeLists.txt` changes (only needed when adding new `.cpp` files).

Manual test checklist:
- Saw wave with smoothing=0 still clicks; smoothing>0 removes click
- Custom breakpoint: draw nodes, verify visualizer matches audio output
- Custom step: drag steps, verify rhythm in output
- Filter low-cut > high-cut is impossible (UI enforces it)
- LFO Filter target sweeps correctly between low/high cut values
- Pitch center = 0 matches old behavior
- Old presets load without error with all new fields defaulted
