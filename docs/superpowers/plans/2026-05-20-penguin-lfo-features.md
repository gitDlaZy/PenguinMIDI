# PenguinLFO Feature Set Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add smoothing/de-click, custom waveform editor (breakpoint + step), frequency filter section with LFO-sweepable range, and pitch center offset to PenguinLFO, plus 15+ factory presets.

**Architecture:** All new state lives on `LFOInstance` (smoothing, pitchCenter, customWave) and `PresetData` (filterLowCut, filterHighCut). The processor gains four filters (hp+lp per channel). A new `CustomWaveformEditor` component handles node and step drawing. LFO rows expand inline when Custom shape is selected.

**Tech Stack:** C++17, JUCE 7, Catch2 v3.4 for tests, nlohmann/json for presets.

**Build:** Open `src/penguin-lfo/build/PenguinLFO.sln` in Visual Studio → Ctrl+Shift+B. CMake re-run only needed after adding new `.cpp` files (Tasks 6 and 9).

**Test:** Enable tests at CMake time: `-DPENGUIN_BUILD_TESTS=ON`. Run with `ctest` from the build dir, or run `PenguinLFO_Tests.exe` directly.

---

## Progress Log

Update this section after each task. Format: `[x] Task N — done` or `[ ] Task N — pending`.

- [x] Task 1 — Data model: LFOEngine.h
- [x] Task 2 — Custom waveform playback + tests
- [x] Task 3 — Smoothing ramp + tests
- [x] Task 4 — Pitch center + two-filter chain (PluginProcessor)
- [x] Task 5 — PresetManager: serialize new fields + update tests
- [x] Task 6 — CustomWaveformEditor component (new files + CMakeLists)
- [x] Task 7 — LFORow UI: smoothing slider + pitch center slider
- [x] Task 8 — LFORow UI: expandable Custom shape panel
- [x] Task 9 — Editor UI: filter section at bottom
- [x] Task 10 — WaveformVisualizer: custom waveform + pitch center line
- [x] Task 11 — Factory presets (22 total — 7 original + 15 new)

---

## File Map

| File | Action | What changes |
|---|---|---|
| `Source/LFOEngine.h` | Modify | Add `WaveNode`, `CustomWaveform`, `Custom` to enum, `smoothing`/`pitchCenter` to `LFOInstance` |
| `Source/LFOEngine.cpp` | Modify | Custom playback in `lfoValueAtPhase`, smoothing ramp + sampleRate param in `lfoAdvance` |
| `Source/PluginProcessor.h` | Modify | Add `filterLowCut/High`, 4 filter objects, pending filter fields |
| `Source/PluginProcessor.cpp` | Modify | Two-filter chain, pitch center, filter LFO sweep uses range, applyPreset updates filter |
| `Source/PresetManager.h` | Modify | Add `filterLowCut/High` to `PresetData` |
| `Source/PresetManager.cpp` | Modify | Serialize/deserialize all new fields, backwards compatible |
| `Source/CustomWaveformEditor.h` | **Create** | Component declaration for breakpoint+step editor |
| `Source/CustomWaveformEditor.cpp` | **Create** | Full waveform editor implementation |
| `Source/PluginEditor.h` | Modify | Add smoothing/pitchCenter controls to `LFORow`, filter section, expand logic |
| `Source/PluginEditor.cpp` | Modify | Wire all new UI controls |
| `Source/WaveformVisualizer.h` | Modify | Update `paint` for custom waveform + pitch center line |
| `Source/WaveformVisualizer.cpp` | Modify | Render custom shape, dashed center line |
| `CMakeLists.txt` | Modify | Add `CustomWaveformEditor.cpp` to `target_sources` |
| `Presets/presets.json` | Modify | 15+ factory presets with new fields |
| `tests/test_lfo_engine.cpp` | Modify | Add tests for custom waveform, smoothing |
| `tests/test_preset_manager.cpp` | Modify | Update preset count, add tests for new fields |

---

## Task 1: Data model — LFOEngine.h

**Files:**
- Modify: `Source/LFOEngine.h`

- [ ] **Step 1: Replace the full contents of `Source/LFOEngine.h` with the following**

```cpp
#pragma once
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

enum class LFOShape  { Sine, Square, SawUp, SawDown, Triangle, SampleAndHold, Custom };
enum class LFOTarget { Volume, Filter, Pan, Pitch };

struct LFORateEntry {
    float       beats;
    const char* name;
};

constexpr int LFO_RATE_8_1   = 0;
constexpr int LFO_RATE_4_1   = 1;
constexpr int LFO_RATE_2_1   = 2;
constexpr int LFO_RATE_1_1   = 3;
constexpr int LFO_RATE_1_2   = 4;
constexpr int LFO_RATE_1_4   = 5;
constexpr int LFO_RATE_1_8   = 6;
constexpr int LFO_RATE_1_16  = 7;
constexpr int LFO_RATE_1_32  = 8;
constexpr int LFO_RATE_1_64  = 9;
constexpr int LFO_RATE_1_1D  = 10;
constexpr int LFO_RATE_1_2D  = 11;
constexpr int LFO_RATE_1_4D  = 12;
constexpr int LFO_RATE_1_8D  = 13;
constexpr int LFO_RATE_1_16D = 14;
constexpr int LFO_RATE_1_2T  = 15;
constexpr int LFO_RATE_1_4T  = 16;
constexpr int LFO_RATE_1_8T  = 17;
constexpr int LFO_RATE_1_16T = 18;
constexpr int LFO_RATE_1_32T = 19;
constexpr int LFO_RATE_3_16  = 20;
constexpr int LFO_RATE_5_16  = 21;
constexpr int LFO_RATE_7_16  = 22;
constexpr int LFO_RATE_5_8   = 23;
constexpr int LFO_RATE_7_8   = 24;
constexpr int LFO_RATE_COUNT = 25;

struct WaveNode { float x; float y; }; // x in [0,1], y in [-1,1]

// Fixed-size (no heap) so LFOInstance stays copyable on the audio thread.
struct CustomWaveform {
    bool     isStepMode = false;
    int      stepCount  = 16;     // 8, 16, or 32
    float    steps[32]  = {};     // y values in [-1,1]
    WaveNode nodes[32]  = {};     // sorted by x
    int      nodeCount  = 0;
};

struct LFOInstance {
    LFOShape      shape              = LFOShape::Sine;
    LFOTarget     target             = LFOTarget::Volume;
    int           rateIndex          = LFO_RATE_1_4;
    float         depth              = 1.0f;
    bool          enabled            = true;
    float         phase              = 0.0f;       // [0,1)
    float         sampleAndHoldValue = 0.0f;
    float         smoothing          = 0.0f;       // 0–1, maps to 0–20 ms at phase wrap
    float         pitchCenter        = 0.0f;       // -1–+1, bias for Pitch target
    CustomWaveform customWave;
};

// Returns LFO value in [-1, 1] for the given phase [0, 1]
float lfoValueAtPhase(const LFOInstance& lfo, float phase);

// Advances phase, applies smoothing ramp, returns value in [-1,1].
// sampleRate needed to convert smoothing (0-1) to a fade duration in samples.
float lfoAdvance(LFOInstance& lfo, float phaseIncrement, float sampleRate = 44100.0f);

extern const LFORateEntry LFO_RATES[LFO_RATE_COUNT];

float lfoPhaseIncrement(int rateIndex, float bpm, float sampleRate);
```

- [ ] **Step 2: Commit**

```bash
git add Source/LFOEngine.h
git commit -m "feat: LFOEngine data model — Custom shape, smoothing, pitchCenter"
```

---

## Task 2: Custom waveform playback + tests

**Files:**
- Modify: `Source/LFOEngine.cpp`
- Modify: `tests/test_lfo_engine.cpp`

- [ ] **Step 1: Write failing tests first** — add to `tests/test_lfo_engine.cpp`

```cpp
// ── Custom waveform ───────────────────────────────────────────────────

TEST_CASE("Custom breakpoint: no nodes returns 0", "[custom]") {
    LFOInstance lfo; lfo.shape = LFOShape::Custom;
    lfo.customWave.isStepMode = false;
    lfo.customWave.nodeCount  = 0;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.5f), WithinAbs(0.0f, 0.001f));
}

TEST_CASE("Custom breakpoint: two nodes interpolates linearly", "[custom]") {
    LFOInstance lfo; lfo.shape = LFOShape::Custom;
    lfo.customWave.isStepMode   = false;
    lfo.customWave.nodeCount    = 2;
    lfo.customWave.nodes[0]     = {0.0f, -1.0f};
    lfo.customWave.nodes[1]     = {1.0f,  1.0f};
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(-1.0f, 0.001f));
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.5f),  WithinAbs( 0.0f, 0.001f));
    REQUIRE_THAT(lfoValueAtPhase(lfo, 1.0f),  WithinAbs( 1.0f, 0.001f));
}

TEST_CASE("Custom breakpoint: phase before first node clamps to first node y", "[custom]") {
    LFOInstance lfo; lfo.shape = LFOShape::Custom;
    lfo.customWave.isStepMode   = false;
    lfo.customWave.nodeCount    = 2;
    lfo.customWave.nodes[0]     = {0.3f, -0.5f};
    lfo.customWave.nodes[1]     = {0.8f,  0.5f};
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(-0.5f, 0.001f));
}

TEST_CASE("Custom step: 4 steps returns correct step value", "[custom]") {
    LFOInstance lfo; lfo.shape = LFOShape::Custom;
    lfo.customWave.isStepMode = true;
    lfo.customWave.stepCount  = 4;
    lfo.customWave.steps[0]   = -1.0f;
    lfo.customWave.steps[1]   =  0.0f;
    lfo.customWave.steps[2]   =  0.5f;
    lfo.customWave.steps[3]   =  1.0f;
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.1f),  WithinAbs(-1.0f, 0.001f)); // step 0
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.3f),  WithinAbs( 0.0f, 0.001f)); // step 1
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.6f),  WithinAbs( 0.5f, 0.001f)); // step 2
    REQUIRE_THAT(lfoValueAtPhase(lfo, 0.9f),  WithinAbs( 1.0f, 0.001f)); // step 3
}
```

- [ ] **Step 2: Verify tests fail** (build with `PENGUIN_BUILD_TESTS=ON`)

Expected: compile error or FAIL — `LFOShape::Custom` not handled in `lfoValueAtPhase`.

- [ ] **Step 3: Implement custom playback in `Source/LFOEngine.cpp`**

Add a `case LFOShape::Custom:` block to `lfoValueAtPhase` and update `lfoAdvance` signature:

```cpp
// In lfoValueAtPhase, add case before the closing return 0.0f:
case LFOShape::Custom: {
    const auto& cw = lfo.customWave;
    if (cw.isStepMode) {
        int idx = static_cast<int>(phase * cw.stepCount);
        idx = std::max(0, std::min(idx, cw.stepCount - 1));
        return cw.steps[idx];
    }
    // Breakpoint mode
    if (cw.nodeCount == 0) return 0.0f;
    if (phase <= cw.nodes[0].x) return cw.nodes[0].y;
    if (phase >= cw.nodes[cw.nodeCount - 1].x) return cw.nodes[cw.nodeCount - 1].y;
    for (int i = 0; i < cw.nodeCount - 1; ++i) {
        if (phase >= cw.nodes[i].x && phase <= cw.nodes[i + 1].x) {
            float span = cw.nodes[i + 1].x - cw.nodes[i].x;
            if (span < 0.0001f) return cw.nodes[i].y;
            float t = (phase - cw.nodes[i].x) / span;
            return cw.nodes[i].y * (1.0f - t) + cw.nodes[i + 1].y * t;
        }
    }
    return 0.0f;
}
```

Also update `lfoAdvance` signature to accept `sampleRate`:

```cpp
// Change signature from:
float lfoAdvance(LFOInstance& lfo, float phaseIncrement)
// To:
float lfoAdvance(LFOInstance& lfo, float phaseIncrement, float sampleRate)
```

Add default value in the .h (already done in Task 1). No body change yet — smoothing ramp goes in Task 3.

- [ ] **Step 4: Run tests — expect PASS for custom tests**

- [ ] **Step 5: Commit**

```bash
git add Source/LFOEngine.cpp tests/test_lfo_engine.cpp
git commit -m "feat: custom waveform playback in LFOEngine (breakpoint + step)"
```

---

## Task 3: Smoothing ramp (de-click) + tests

**Files:**
- Modify: `Source/LFOEngine.cpp`
- Modify: `tests/test_lfo_engine.cpp`

- [ ] **Step 1: Write failing tests** — add to `tests/test_lfo_engine.cpp`

```cpp
// ── Smoothing ─────────────────────────────────────────────────────────

TEST_CASE("Smoothing=0 does not change output", "[smoothing]") {
    LFOInstance lfo; lfo.shape = LFOShape::SawUp; lfo.smoothing = 0.0f;
    float inc = lfoPhaseIncrement(LFO_RATE_1_4, 120.0f, 44100.0f);
    // Advance to middle of cycle (phase ~0.5), away from ramp zones
    for (int i = 0; i < 11025; ++i) lfoAdvance(lfo, inc, 44100.0f);
    float withSmooth    = lfoAdvance(lfo, inc, 44100.0f);
    LFOInstance lfo2 = lfo; lfo2.smoothing = 0.0f;
    float withoutSmooth = lfoValueAtPhase(lfo2, lfo2.phase);
    REQUIRE_THAT(withSmooth, WithinAbs(withoutSmooth, 0.001f));
}

TEST_CASE("Smoothing ramp: output near 0 just after phase wrap", "[smoothing]") {
    LFOInstance lfo; lfo.shape = LFOShape::SawUp; lfo.smoothing = 1.0f; // 20ms
    lfo.phase = 0.999f; // just before wrap
    float inc = lfoPhaseIncrement(LFO_RATE_1_4, 120.0f, 44100.0f);
    float val = lfoAdvance(lfo, inc, 44100.0f); // wraps, enters fade-in zone
    // SawUp just after phase=0 raw value is ~-1, ramp should bring it toward 0
    REQUIRE(std::abs(val) < 0.5f);
}

TEST_CASE("Smoothing ramp: output near 0 just before phase wrap", "[smoothing]") {
    LFOInstance lfo; lfo.shape = LFOShape::SawUp; lfo.smoothing = 1.0f;
    float inc = lfoPhaseIncrement(LFO_RATE_1_4, 120.0f, 44100.0f);
    // Advance to just before wrap (phase ~0.998)
    float target = 1.0f - 2.0f * inc;
    while (lfo.phase < target) lfoAdvance(lfo, inc, 44100.0f);
    float val = lfoAdvance(lfo, inc, 44100.0f);
    REQUIRE(std::abs(val) < 0.5f); // fading out toward wrap
}
```

- [ ] **Step 2: Verify tests fail**

Expected: FAIL — smoothing has no effect yet.

- [ ] **Step 3: Add smoothing ramp to `lfoAdvance` in `Source/LFOEngine.cpp`**

Replace the existing `lfoAdvance` function body:

```cpp
float lfoAdvance(LFOInstance& lfo, float phaseIncrement, float sampleRate) {
    lfo.phase += phaseIncrement;
    if (lfo.phase >= 1.0f) {
        lfo.phase -= 1.0f;
        lfo.sampleAndHoldValue =
            (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
    }
    float raw = lfoValueAtPhase(lfo, lfo.phase);

    if (lfo.smoothing > 0.0f && phaseIncrement > 0.0f) {
        // fadePhase: fraction of the cycle used for the ramp (capped at 0.49 so ramps don't overlap)
        float fadeSamples = lfo.smoothing * 20.0f / 1000.0f * sampleRate;
        float fadePhase   = fadeSamples * phaseIncrement;
        fadePhase = std::min(fadePhase, 0.49f);
        if (lfo.phase < fadePhase)
            raw *= lfo.phase / fadePhase;
        else if (lfo.phase > (1.0f - fadePhase))
            raw *= (1.0f - lfo.phase) / fadePhase;
    }
    return raw;
}
```

- [ ] **Step 4: Update all callers of `lfoAdvance` to pass `sampleRate`**

In `Source/PluginProcessor.cpp`, the call site is in processBlock:
```cpp
// Before (old):
float val = lfoAdvance(lfos[i], incs[i]);
// After:
float val = lfoAdvance(lfos[i], incs[i], static_cast<float>(currentSampleRate));
```

In `Source/WaveformVisualizer.cpp`, the call site:
```cpp
// Before:
lfoAdvance(sim[i], inc);
// After:
lfoAdvance(sim[i], inc, sampleRate);
```

- [ ] **Step 5: Run tests — expect all PASS**

- [ ] **Step 6: Commit**

```bash
git add Source/LFOEngine.cpp Source/PluginProcessor.cpp Source/WaveformVisualizer.cpp tests/test_lfo_engine.cpp
git commit -m "feat: LFO smoothing ramp (de-click) with sampleRate-aware fade"
```

---

## Task 4: Pitch center + two-filter chain (PluginProcessor)

**Files:**
- Modify: `Source/PluginProcessor.h`
- Modify: `Source/PluginProcessor.cpp`

- [ ] **Step 1: Update `Source/PluginProcessor.h`**

Replace the filter and related declarations:

```cpp
// Remove:
juce::dsp::StateVariableTPTFilter<float> filterLeft, filterRight;

// Add:
float filterLowCut  = 20.0f;
float filterHighCut = 20000.0f;
// Pending filter values for preset handoff
float pendingFilterLowCut  = 20.0f;
float pendingFilterHighCut = 20000.0f;

juce::dsp::StateVariableTPTFilter<float> hpLeft, hpRight;
juce::dsp::StateVariableTPTFilter<float> lpLeft, lpRight;
```

Also make `filterLowCut` and `filterHighCut` public (above the `private:` line) so the editor can read/write them:
```cpp
// Public section (after lfos, factoryPresets, etc.):
float filterLowCut  = 20.0f;
float filterHighCut = 20000.0f;
```

And the pending ones stay private.

- [ ] **Step 2: Update `prepareToPlay` in `Source/PluginProcessor.cpp`**

Replace the filterLeft/filterRight setup:

```cpp
// Remove old filter setup, replace with:
hpLeft.prepare(spec);
hpRight.prepare(spec);
lpLeft.prepare(spec);
lpRight.prepare(spec);
hpLeft.setType(juce::dsp::StateVariableTPTFilterType::highpass);
hpRight.setType(juce::dsp::StateVariableTPTFilterType::highpass);
lpLeft.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
lpRight.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
hpLeft.setCutoffFrequency(filterLowCut);
hpRight.setCutoffFrequency(filterLowCut);
lpLeft.setCutoffFrequency(filterHighCut);
lpRight.setCutoffFrequency(filterHighCut);
```

- [ ] **Step 3: Update `applyPreset` to also hand off filter values**

```cpp
void PenguinLFOProcessor::applyPreset(const PresetData& preset) {
    pendingLfos          = preset.lfos;
    pendingFilterLowCut  = preset.filterLowCut;
    pendingFilterHighCut = preset.filterHighCut;
    presetPending.store(true, std::memory_order_release);
}
```

- [ ] **Step 4: Update `processBlock` — apply pending filter, pitch center, two-filter chain**

In the preset-apply block at the top of processBlock:
```cpp
if (presetPending.exchange(false, std::memory_order_acquire)) {
    lfos            = pendingLfos;
    filterLowCut    = pendingFilterLowCut;
    filterHighCut   = pendingFilterHighCut;
}
```

For the pitch target, replace:
```cpp
// Old:
pitchMod += val * lfos[i].depth;
// New:
pitchMod += (val + lfos[i].pitchCenter) * lfos[i].depth;
```

For the filter per-sample processing, replace the two `filterLeft/filterRight.setCutoffFrequency` + `processSample` lines:

```cpp
// Old filter code (remove):
filterLeft.setCutoffFrequency(filterCutoff);
filterRight.setCutoffFrequency(filterCutoff);
// ...
L[s] = filterLeft.processSample(0, Ls) * gainMod * leftGain;
if (R) R[s] = filterRight.processSample(0, Rs) * gainMod * rightGain;

// New:
// filterCutoff is now the LFO-modulated LP cutoff (clamped between filterLowCut and filterHighCut)
float lpCutoff = filterCutoff; // already set by LFO loop (or 20000 if no Filter LFO)
// Clamp LFO-driven cutoff to user range
lpCutoff = juce::jlimit(filterLowCut, filterHighCut, lpCutoff);

hpLeft.setCutoffFrequency(filterLowCut);
hpRight.setCutoffFrequency(filterLowCut);
lpLeft.setCutoffFrequency(lpCutoff);
lpRight.setCutoffFrequency(lpCutoff);

float filtL = lpLeft.processSample(0, hpLeft.processSample(0, Ls));
float filtR = R ? lpRight.processSample(0, hpRight.processSample(0, Rs)) : 0.0f;

L[s] = filtL * gainMod * leftGain;
if (R) R[s] = filtR * gainMod * rightGain;
```

Also update the Filter LFO target case to use filterLowCut/filterHighCut for the sweep range:

```cpp
case LFOTarget::Filter: {
    float t = val * 0.5f + 0.5f; // [-1,1] → [0,1]
    float logRange = std::log(filterHighCut / std::max(filterLowCut, 20.0f));
    filterCutoff = filterLowCut * std::exp(logRange * (t * lfos[i].depth + (1.0f - lfos[i].depth) * 0.5f));
    filterCutoff = juce::jlimit(filterLowCut, filterHighCut, filterCutoff);
    break;
}
```

Also update `filterCutoff` initialization to use `filterHighCut` instead of hardcoded 20000:
```cpp
float filterCutoff = filterHighCut; // was: 20000.0f
```

- [ ] **Step 5: Update `getStateInformation` / `setStateInformation`**

In `getStateInformation`, add filter params to the serialized preset:
```cpp
current.filterLowCut  = filterLowCut;
current.filterHighCut = filterHighCut;
```

In `setStateInformation`, after restoring LFO fields add:
```cpp
filterLowCut  = presets[0].filterLowCut;
filterHighCut = presets[0].filterHighCut;
```

- [ ] **Step 6: Build in Visual Studio (Ctrl+Shift+B) — fix any compile errors**

- [ ] **Step 7: Commit**

```bash
git add Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat: pitch center offset, two-filter chain (hp+lp), LFO filter sweeps user range"
```

---

## Task 5: PresetManager — serialize new fields + update tests

**Files:**
- Modify: `Source/PresetManager.h`
- Modify: `Source/PresetManager.cpp`
- Modify: `tests/test_preset_manager.cpp`

- [ ] **Step 1: Add filter fields to `PresetData` in `Source/PresetManager.h`**

```cpp
struct PresetData {
    std::string                name;
    std::array<LFOInstance, 4> lfos;
    float filterLowCut  = 20.0f;
    float filterHighCut = 20000.0f;
};
```

- [ ] **Step 2: Write failing tests** — add to `tests/test_preset_manager.cpp`

```cpp
TEST_CASE("Preset round-trips smoothing and pitchCenter", "[presets]") {
    PresetData p;
    p.name = "Smooth Test";
    p.lfos[0].smoothing    = 0.5f;
    p.lfos[0].pitchCenter  = -0.3f;
    p.lfos[0].enabled      = true;
    p.lfos[1].enabled = p.lfos[2].enabled = p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE_THAT(loaded[0].lfos[0].smoothing,   WithinAbs(0.5f,  0.001f));
    REQUIRE_THAT(loaded[0].lfos[0].pitchCenter, WithinAbs(-0.3f, 0.001f));
}

TEST_CASE("Preset round-trips filter range", "[presets]") {
    PresetData p;
    p.name          = "Filter Test";
    p.filterLowCut  = 200.0f;
    p.filterHighCut = 8000.0f;
    p.lfos[0].enabled = false;
    p.lfos[1].enabled = false;
    p.lfos[2].enabled = false;
    p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE_THAT(loaded[0].filterLowCut,  WithinAbs(200.0f,  0.1f));
    REQUIRE_THAT(loaded[0].filterHighCut, WithinAbs(8000.0f, 0.1f));
}

TEST_CASE("Old preset without new fields loads with defaults", "[presets]") {
    const char* oldJson = R"({
      "presets": [{
        "name": "Old Preset",
        "lfos": [
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.5,"enabled":true},
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false},
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false},
          {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false}
        ]
      }]
    })";
    auto presets = parsePresetsJson(oldJson);
    REQUIRE(presets.size() == 1);
    REQUIRE_THAT(presets[0].lfos[0].smoothing,   WithinAbs(0.0f,  0.001f));
    REQUIRE_THAT(presets[0].lfos[0].pitchCenter, WithinAbs(0.0f,  0.001f));
    REQUIRE_THAT(presets[0].filterLowCut,        WithinAbs(20.0f, 0.1f));
    REQUIRE_THAT(presets[0].filterHighCut,       WithinAbs(20000.0f, 1.0f));
}

TEST_CASE("Preset round-trips custom waveform (step mode)", "[presets]") {
    PresetData p;
    p.name = "Step Test";
    p.lfos[0].shape               = LFOShape::Custom;
    p.lfos[0].customWave.isStepMode = true;
    p.lfos[0].customWave.stepCount  = 4;
    p.lfos[0].customWave.steps[0]   = -1.0f;
    p.lfos[0].customWave.steps[1]   =  0.5f;
    p.lfos[0].customWave.steps[2]   =  0.0f;
    p.lfos[0].customWave.steps[3]   =  1.0f;
    p.lfos[0].enabled = true;
    p.lfos[1].enabled = p.lfos[2].enabled = p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE(loaded[0].lfos[0].shape == LFOShape::Custom);
    REQUIRE(loaded[0].lfos[0].customWave.isStepMode == true);
    REQUIRE(loaded[0].lfos[0].customWave.stepCount  == 4);
    REQUIRE_THAT(loaded[0].lfos[0].customWave.steps[0], WithinAbs(-1.0f, 0.001f));
    REQUIRE_THAT(loaded[0].lfos[0].customWave.steps[3], WithinAbs( 1.0f, 0.001f));
}

TEST_CASE("Preset round-trips custom waveform (breakpoint mode)", "[presets]") {
    PresetData p;
    p.name = "Node Test";
    p.lfos[0].shape               = LFOShape::Custom;
    p.lfos[0].customWave.isStepMode = false;
    p.lfos[0].customWave.nodeCount  = 3;
    p.lfos[0].customWave.nodes[0]   = {0.0f, -1.0f};
    p.lfos[0].customWave.nodes[1]   = {0.5f,  1.0f};
    p.lfos[0].customWave.nodes[2]   = {1.0f, -1.0f};
    p.lfos[0].enabled = true;
    p.lfos[1].enabled = p.lfos[2].enabled = p.lfos[3].enabled = false;

    std::string wrapped = "{\"presets\":[" + serializePreset(p) + "]}";
    auto loaded = parsePresetsJson(wrapped);

    REQUIRE(loaded[0].lfos[0].shape == LFOShape::Custom);
    REQUIRE(loaded[0].lfos[0].customWave.isStepMode == false);
    REQUIRE(loaded[0].lfos[0].customWave.nodeCount  == 3);
    REQUIRE_THAT(loaded[0].lfos[0].customWave.nodes[1].x, WithinAbs(0.5f, 0.001f));
    REQUIRE_THAT(loaded[0].lfos[0].customWave.nodes[1].y, WithinAbs(1.0f, 0.001f));
}
```

Also update the factory preset count test (will be 22+ after Task 11):
```cpp
// Change from:
REQUIRE(presets.size() == 7);
// To:
REQUIRE(presets.size() >= 7); // will grow in Task 11
```

- [ ] **Step 3: Verify new tests fail**

- [ ] **Step 4: Update `Source/PresetManager.cpp`**

Add `"Custom"` to `shapeFromString` and `shapeToString`:
```cpp
// shapeFromString: add
if (s == "Custom") return LFOShape::Custom;

// shapeToString: add
case LFOShape::Custom: return "Custom";
```

Update `parsePresetsJson` to read new per-LFO fields (inside the lfo loop, after existing reads):
```cpp
preset.lfos[i].smoothing   = l.value("smoothing",   0.0f);
preset.lfos[i].pitchCenter = l.value("pitchCenter",  0.0f);

// Custom waveform
if (l.contains("customWave")) {
    auto& cw = preset.lfos[i].customWave;
    auto& jcw = l["customWave"];
    cw.isStepMode = jcw.value("isStepMode", false);
    cw.stepCount  = jcw.value("stepCount",  16);
    if (jcw.contains("steps")) {
        int n = 0;
        for (auto& v : jcw["steps"]) {
            if (n >= 32) break;
            cw.steps[n++] = v.get<float>();
        }
    }
    if (jcw.contains("nodes")) {
        cw.nodeCount = 0;
        for (auto& nd : jcw["nodes"]) {
            if (cw.nodeCount >= 32) break;
            cw.nodes[cw.nodeCount++] = { nd["x"].get<float>(), nd["y"].get<float>() };
        }
    }
}
```

Read filter fields after the lfo loop (before `result.push_back`):
```cpp
preset.filterLowCut  = p.value("filterLowCut",   20.0f);
preset.filterHighCut = p.value("filterHighCut", 20000.0f);
```

Update `serializePreset` to write new fields (inside the lfo loop):
```cpp
json lfoObj = {
    {"shape",       shapeToString(lfo.shape)},
    {"rate",        LFO_RATES[rateIdx].name},
    {"target",      targetToString(lfo.target)},
    {"depth",       lfo.depth},
    {"enabled",     lfo.enabled},
    {"smoothing",   lfo.smoothing},
    {"pitchCenter", lfo.pitchCenter}
};

// Custom waveform
if (lfo.shape == LFOShape::Custom) {
    json jcw;
    jcw["isStepMode"] = lfo.customWave.isStepMode;
    jcw["stepCount"]  = lfo.customWave.stepCount;
    json steps = json::array();
    for (int s = 0; s < lfo.customWave.stepCount; ++s)
        steps.push_back(lfo.customWave.steps[s]);
    jcw["steps"] = steps;
    json nodes = json::array();
    for (int n = 0; n < lfo.customWave.nodeCount; ++n)
        nodes.push_back({{"x", lfo.customWave.nodes[n].x}, {"y", lfo.customWave.nodes[n].y}});
    jcw["nodes"]  = nodes;
    lfoObj["customWave"] = jcw;
}
j["lfos"].push_back(lfoObj);
```

Write filter fields after the lfos loop:
```cpp
j["filterLowCut"]  = preset.filterLowCut;
j["filterHighCut"] = preset.filterHighCut;
```

- [ ] **Step 5: Run all tests — expect PASS**

- [ ] **Step 6: Commit**

```bash
git add Source/PresetManager.h Source/PresetManager.cpp tests/test_preset_manager.cpp
git commit -m "feat: PresetManager serializes smoothing, pitchCenter, customWave, filter range"
```

---

## Task 6: CustomWaveformEditor component (new files)

**Files:**
- Create: `Source/CustomWaveformEditor.h`
- Create: `Source/CustomWaveformEditor.cpp`
- Modify: `CMakeLists.txt`

Note: After this task you must **re-run CMake** because a new `.cpp` file is added. In Visual Studio: close the solution, run CMake from the terminal (`cmake -B build -S .` from `src/penguin-lfo/`), reopen the solution.

- [ ] **Step 1: Create `Source/CustomWaveformEditor.h`**

```cpp
#pragma once
#include <JuceHeader.h>
#include "LFOEngine.h"
#include <functional>

class CustomWaveformEditor : public juce::Component {
public:
    CustomWaveformEditor();

    void setWaveform(const CustomWaveform& wf);
    const CustomWaveform& getWaveform() const { return waveform; }

    std::function<void()> onChange;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    CustomWaveform waveform;

    // UI controls
    juce::TextButton breakpointBtn { "Breakpoint" };
    juce::TextButton stepBtn       { "Step" };
    juce::ComboBox   stepCountBox;
    juce::TextButton clearBtn      { "Clear" };

    int dragNodeIndex = -1;  // which node is being dragged (breakpoint mode)
    int dragStepIndex = -1;  // which step bar is being dragged (step mode)

    juce::Rectangle<float> editorArea() const;
    juce::Point<float> nodeToScreen(float x, float y) const;
    juce::Point<float> screenToNorm(float sx, float sy) const; // returns {x in [0,1], y in [-1,1]}

    int hitTestNode(juce::Point<float> screenPt) const;  // returns node index or -1
    int hitTestStep(juce::Point<float> screenPt) const;  // returns step index or -1

    void sortNodes();
    void paintBreakpointEditor(juce::Graphics& g, juce::Rectangle<float> area);
    void paintStepEditor(juce::Graphics& g, juce::Rectangle<float> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomWaveformEditor)
};
```

- [ ] **Step 2: Create `Source/CustomWaveformEditor.cpp`**

```cpp
#include "CustomWaveformEditor.h"

static const juce::Colour BG     { 0xff0d1117 };
static const juce::Colour GRID   { 0xff2a2a3a };
static const juce::Colour LINE   { 0xff7ec8e3 };
static const juce::Colour NODE   { 0xffffffff };
static const juce::Colour STEP   { 0xff4a9eff };

CustomWaveformEditor::CustomWaveformEditor() {
    stepCountBox.addItem("8 steps",  8);
    stepCountBox.addItem("16 steps", 16);
    stepCountBox.addItem("32 steps", 32);
    stepCountBox.setSelectedId(16, juce::dontSendNotification);

    breakpointBtn.setClickingTogglesState(true);
    stepBtn.setClickingTogglesState(true);
    breakpointBtn.setToggleState(true, juce::dontSendNotification);

    breakpointBtn.onClick = [this] {
        if (!breakpointBtn.getToggleState()) { breakpointBtn.setToggleState(true, juce::dontSendNotification); return; }
        stepBtn.setToggleState(false, juce::dontSendNotification);
        waveform.isStepMode = false;
        repaint();
        if (onChange) onChange();
    };
    stepBtn.onClick = [this] {
        if (!stepBtn.getToggleState()) { stepBtn.setToggleState(true, juce::dontSendNotification); return; }
        breakpointBtn.setToggleState(false, juce::dontSendNotification);
        waveform.isStepMode = true;
        repaint();
        if (onChange) onChange();
    };
    stepCountBox.onChange = [this] {
        waveform.stepCount = stepCountBox.getSelectedId();
        repaint();
        if (onChange) onChange();
    };
    clearBtn.onClick = [this] {
        if (waveform.isStepMode) {
            for (int i = 0; i < 32; ++i) waveform.steps[i] = 0.0f;
        } else {
            waveform.nodeCount = 0;
        }
        repaint();
        if (onChange) onChange();
    };

    addAndMakeVisible(breakpointBtn);
    addAndMakeVisible(stepBtn);
    addAndMakeVisible(stepCountBox);
    addAndMakeVisible(clearBtn);
}

void CustomWaveformEditor::setWaveform(const CustomWaveform& wf) {
    waveform = wf;
    breakpointBtn.setToggleState(!wf.isStepMode, juce::dontSendNotification);
    stepBtn.setToggleState(wf.isStepMode, juce::dontSendNotification);
    stepCountBox.setSelectedId(wf.stepCount, juce::dontSendNotification);
    repaint();
}

void CustomWaveformEditor::resized() {
    auto r = getLocalBounds();
    auto toolbar = r.removeFromTop(24);
    breakpointBtn.setBounds(toolbar.removeFromLeft(80));
    toolbar.removeFromLeft(4);
    stepBtn.setBounds(toolbar.removeFromLeft(50));
    toolbar.removeFromLeft(4);
    stepCountBox.setBounds(toolbar.removeFromLeft(80));
    toolbar.removeFromLeft(4);
    clearBtn.setBounds(toolbar.removeFromLeft(50));
}

juce::Rectangle<float> CustomWaveformEditor::editorArea() const {
    return getLocalBounds().toFloat().withTrimmedTop(28.0f).reduced(4.0f, 4.0f);
}

juce::Point<float> CustomWaveformEditor::nodeToScreen(float x, float y) const {
    auto a = editorArea();
    return { a.getX() + x * a.getWidth(),
             a.getY() + (1.0f - (y + 1.0f) * 0.5f) * a.getHeight() };
}

juce::Point<float> CustomWaveformEditor::screenToNorm(float sx, float sy) const {
    auto a = editorArea();
    float x = juce::jlimit(0.0f, 1.0f, (sx - a.getX()) / a.getWidth());
    float y = juce::jlimit(-1.0f, 1.0f, 1.0f - (sy - a.getY()) / a.getHeight() * 2.0f);
    return { x, y };
}

int CustomWaveformEditor::hitTestNode(juce::Point<float> pt) const {
    for (int i = 0; i < waveform.nodeCount; ++i) {
        auto sp = nodeToScreen(waveform.nodes[i].x, waveform.nodes[i].y);
        if (sp.getDistanceFrom(pt) < 8.0f) return i;
    }
    return -1;
}

int CustomWaveformEditor::hitTestStep(juce::Point<float> pt) const {
    auto a = editorArea();
    if (!a.contains(pt)) return -1;
    float stepW = a.getWidth() / waveform.stepCount;
    int idx = static_cast<int>((pt.x - a.getX()) / stepW);
    return juce::jlimit(0, waveform.stepCount - 1, idx);
}

void CustomWaveformEditor::sortNodes() {
    std::sort(waveform.nodes, waveform.nodes + waveform.nodeCount,
              [](const WaveNode& a, const WaveNode& b){ return a.x < b.x; });
}

void CustomWaveformEditor::mouseDown(const juce::MouseEvent& e) {
    auto pt = e.position;
    if (waveform.isStepMode) {
        dragStepIndex = hitTestStep(pt);
        if (dragStepIndex >= 0) {
            auto norm = screenToNorm(pt.x, pt.y);
            waveform.steps[dragStepIndex] = norm.y;
            repaint();
            if (onChange) onChange();
        }
    } else {
        dragNodeIndex = hitTestNode(pt);
        if (dragNodeIndex < 0 && waveform.nodeCount < 32) {
            // Add new node
            auto norm = screenToNorm(pt.x, pt.y);
            waveform.nodes[waveform.nodeCount++] = { norm.x, norm.y };
            sortNodes();
            // Find the new node's index after sorting
            for (int i = 0; i < waveform.nodeCount; ++i)
                if (std::abs(waveform.nodes[i].x - norm.x) < 0.001f) { dragNodeIndex = i; break; }
            repaint();
            if (onChange) onChange();
        }
    }
}

void CustomWaveformEditor::mouseDrag(const juce::MouseEvent& e) {
    auto norm = screenToNorm(e.position.x, e.position.y);
    if (waveform.isStepMode) {
        int idx = hitTestStep(e.position);
        if (idx >= 0) {
            waveform.steps[idx] = norm.y;
            repaint();
            if (onChange) onChange();
        }
    } else if (dragNodeIndex >= 0) {
        waveform.nodes[dragNodeIndex] = { norm.x, norm.y };
        sortNodes();
        repaint();
        if (onChange) onChange();
    }
}

void CustomWaveformEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    if (waveform.isStepMode) return;
    int idx = hitTestNode(e.position);
    if (idx >= 0 && waveform.nodeCount > 1) {
        // Remove node by shifting
        for (int i = idx; i < waveform.nodeCount - 1; ++i)
            waveform.nodes[i] = waveform.nodes[i + 1];
        --waveform.nodeCount;
        repaint();
        if (onChange) onChange();
    }
}

void CustomWaveformEditor::paintBreakpointEditor(juce::Graphics& g, juce::Rectangle<float> area) {
    // Zero line
    float midY = area.getY() + area.getHeight() * 0.5f;
    g.setColour(GRID);
    g.drawHorizontalLine(static_cast<int>(midY), area.getX(), area.getRight());

    if (waveform.nodeCount < 1) return;

    juce::Path path;
    for (int i = 0; i < waveform.nodeCount; ++i) {
        auto sp = nodeToScreen(waveform.nodes[i].x, waveform.nodes[i].y);
        i == 0 ? path.startNewSubPath(sp) : path.lineTo(sp);
    }
    g.setColour(LINE);
    g.strokePath(path, juce::PathStrokeType(1.5f));

    // Draw nodes
    for (int i = 0; i < waveform.nodeCount; ++i) {
        auto sp = nodeToScreen(waveform.nodes[i].x, waveform.nodes[i].y);
        g.setColour(NODE);
        g.fillEllipse(sp.x - 5.0f, sp.y - 5.0f, 10.0f, 10.0f);
    }
}

void CustomWaveformEditor::paintStepEditor(juce::Graphics& g, juce::Rectangle<float> area) {
    float stepW = area.getWidth() / waveform.stepCount;
    float midY  = area.getY() + area.getHeight() * 0.5f;
    g.setColour(GRID);
    g.drawHorizontalLine(static_cast<int>(midY), area.getX(), area.getRight());

    for (int i = 0; i < waveform.stepCount; ++i) {
        float x   = area.getX() + i * stepW + 1.0f;
        float w   = stepW - 2.0f;
        float val = waveform.steps[i]; // [-1, 1]
        float barY, barH;
        if (val >= 0.0f) {
            barH = val * area.getHeight() * 0.5f;
            barY = midY - barH;
        } else {
            barH = -val * area.getHeight() * 0.5f;
            barY = midY;
        }
        g.setColour(STEP);
        g.fillRect(x, barY, w, barH);
    }
}

void CustomWaveformEditor::paint(juce::Graphics& g) {
    auto area = editorArea();
    g.setColour(BG);
    g.fillRect(area);
    g.setColour(GRID);
    g.drawRect(area);

    if (waveform.isStepMode)
        paintStepEditor(g, area);
    else
        paintBreakpointEditor(g, area);
}
```

- [ ] **Step 3: Add `CustomWaveformEditor.cpp` to `CMakeLists.txt`**

In `src/penguin-lfo/CMakeLists.txt`, inside `target_sources`:
```cmake
# Add:
Source/CustomWaveformEditor.cpp
```

- [ ] **Step 4: Re-run CMake from the `src/penguin-lfo/` directory**

```bash
cmake -B build -S .
```

Then reopen `build/PenguinLFO.sln` and build (Ctrl+Shift+B). Fix any compile errors.

- [ ] **Step 5: Commit**

```bash
git add Source/CustomWaveformEditor.h Source/CustomWaveformEditor.cpp CMakeLists.txt
git commit -m "feat: CustomWaveformEditor component (breakpoint + step modes)"
```

---

## Task 7: LFORow UI — smoothing slider + pitch center slider

**Files:**
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`

The pitch center slider is only visible when target = Pitch. We handle this by adding both controls to the row and showing/hiding as needed.

- [ ] **Step 1: Update `LFORow` class declaration in `Source/PluginEditor.h`**

```cpp
class LFORow : public juce::Component {
public:
    explicit LFORow(int index);
    void resized() override;

    juce::ComboBox     shapeBox, rateBox, targetBox;
    juce::Slider       depthSlider;
    juce::ToggleButton enableToggle;
    juce::Slider       smoothingSlider;
    juce::Label        smoothingLabel;   // shows "2.4 ms"
    juce::Slider       pitchCenterSlider;
    juce::Label        pitchCenterLabel; // shows "+0.50"

    void updateControlVisibility(); // call when target changes

private:
    juce::Label label;
};
```

- [ ] **Step 2: Update `LFORow` constructor and `resized` in `Source/PluginEditor.cpp`**

```cpp
LFORow::LFORow(int index) {
    // ... (existing label + combo setup unchanged) ...

    smoothingSlider.setRange(0.0, 1.0, 0.001);
    smoothingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    smoothingSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    smoothingSlider.setValue(0.0);
    smoothingLabel.setText("0.0 ms", juce::dontSendNotification);
    smoothingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    smoothingLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));

    pitchCenterSlider.setRange(-1.0, 1.0, 0.01);
    pitchCenterSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchCenterSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    pitchCenterSlider.setValue(0.0);
    pitchCenterLabel.setText("+0.00", juce::dontSendNotification);
    pitchCenterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    pitchCenterLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));

    smoothingSlider.onValueChange = [this] {
        float ms = static_cast<float>(smoothingSlider.getValue()) * 20.0f;
        smoothingLabel.setText(juce::String(ms, 1) + " ms", juce::dontSendNotification);
    };
    pitchCenterSlider.onValueChange = [this] {
        float v = static_cast<float>(pitchCenterSlider.getValue());
        pitchCenterLabel.setText((v >= 0 ? "+" : "") + juce::String(v, 2), juce::dontSendNotification);
    };

    addAndMakeVisible(smoothingSlider);
    addAndMakeVisible(smoothingLabel);
    addAndMakeVisible(pitchCenterSlider);
    addAndMakeVisible(pitchCenterLabel);

    updateControlVisibility();
}

void LFORow::updateControlVisibility() {
    bool isPitch = (static_cast<LFOTarget>(targetBox.getSelectedId() - 1) == LFOTarget::Pitch);
    pitchCenterSlider.setVisible(isPitch);
    pitchCenterLabel.setVisible(isPitch);
}
```

Update `resized`:
```cpp
void LFORow::resized() {
    auto r = getLocalBounds().reduced(2);
    label.setBounds(r.removeFromLeft(40));
    r.removeFromLeft(4);
    enableToggle.setBounds(r.removeFromRight(36));
    r.removeFromRight(4);
    // Smoothing: 60px slider + 40px label
    smoothingLabel.setBounds(r.removeFromRight(40));
    smoothingSlider.setBounds(r.removeFromRight(60));
    r.removeFromRight(4);
    // Pitch center (only visible when Pitch target): 60px slider + 40px label
    if (pitchCenterSlider.isVisible()) {
        pitchCenterLabel.setBounds(r.removeFromRight(40));
        pitchCenterSlider.setBounds(r.removeFromRight(60));
        r.removeFromRight(4);
    }
    depthSlider.setBounds(r.removeFromRight(80));
    r.removeFromRight(4);
    int cw = r.getWidth() / 3;
    shapeBox.setBounds(r.removeFromLeft(cw));
    rateBox.setBounds(r.removeFromLeft(cw));
    targetBox.setBounds(r);
}
```

- [ ] **Step 3: Wire smoothing + pitchCenter in `syncLFOFromRow` and `syncRowFromLFO` in `PluginEditor.cpp`**

```cpp
void PenguinLFOEditor::syncLFOFromRow(int i) {
    auto& row = lfoRows[i];
    row.updateControlVisibility();
    row.resized();
    processor.updateLFOParam(
        i,
        static_cast<LFOShape>(row.shapeBox.getSelectedId() - 1),
        row.rateBox.getSelectedId() - 1,
        static_cast<LFOTarget>(row.targetBox.getSelectedId() - 1),
        static_cast<float>(row.depthSlider.getValue()),
        row.enableToggle.getToggleState(),
        static_cast<float>(row.smoothingSlider.getValue()),
        static_cast<float>(row.pitchCenterSlider.getValue())
    );
}

void PenguinLFOEditor::syncRowFromLFO(int i) {
    auto& lfo = processor.lfos[i];
    auto& row = lfoRows[i];
    row.shapeBox.setSelectedId(static_cast<int>(lfo.shape) + 1,  juce::dontSendNotification);
    row.rateBox.setSelectedId(lfo.rateIndex + 1,                  juce::dontSendNotification);
    row.targetBox.setSelectedId(static_cast<int>(lfo.target) + 1, juce::dontSendNotification);
    row.depthSlider.setValue(lfo.depth,                           juce::dontSendNotification);
    row.enableToggle.setToggleState(lfo.enabled,                  juce::dontSendNotification);
    row.smoothingSlider.setValue(lfo.smoothing,                   juce::dontSendNotification);
    float ms = lfo.smoothing * 20.0f;
    row.smoothingLabel.setText(juce::String(ms, 1) + " ms",       juce::dontSendNotification);
    row.pitchCenterSlider.setValue(lfo.pitchCenter,               juce::dontSendNotification);
    float pc = lfo.pitchCenter;
    row.pitchCenterLabel.setText((pc >= 0 ? "+" : "") + juce::String(pc, 2), juce::dontSendNotification);
    row.updateControlVisibility();
    row.resized();
}
```

- [ ] **Step 4: Update `updateLFOParam` in `PluginProcessor.h` / `PluginProcessor.cpp` to accept new params**

```cpp
// Header — new signature:
void updateLFOParam(int index, LFOShape shape, int rateIndex,
                    LFOTarget target, float depth, bool enabled,
                    float smoothing, float pitchCenter);

// Implementation — add the two new lines:
pendingParamUpdates[index].smoothing    = smoothing;
pendingParamUpdates[index].pitchCenter  = pitchCenter;
```

Note: `customWave` is NOT passed through `updateLFOParam` — it goes through a dedicated method added in Task 8.

- [ ] **Step 5: Add `"Custom"` as 7th item in `shapeBox` in LFORow constructor**

```cpp
// The existing loop:
for (int i = 0; i < 6; ++i) shapeBox.addItem(SHAPE_NAMES[i], i + 1);
// Change to 7:
for (int i = 0; i < 7; ++i) shapeBox.addItem(SHAPE_NAMES[i], i + 1);
```

And add `"Custom"` to `SHAPE_NAMES` at the top of `PluginEditor.cpp`:
```cpp
static const char* SHAPE_NAMES[] = {"Sine","Square","Saw Up","Saw Down","Triangle","S&H","Custom"};
```

- [ ] **Step 6: Build — fix any compile errors**

- [ ] **Step 7: Commit**

```bash
git add Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat: LFORow smoothing slider, pitch center slider, Custom shape option"
```

---

## Task 8: LFORow UI — expandable Custom shape panel

**Files:**
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`

- [ ] **Step 1: Add `CustomWaveformEditor` to `LFORow` in `Source/PluginEditor.h`**

```cpp
#include "CustomWaveformEditor.h"

class LFORow : public juce::Component {
public:
    // ... (existing public members)
    CustomWaveformEditor waveEditor;
    bool isExpanded() const { return waveEditor.isVisible(); }
    // ... rest unchanged
};
```

- [ ] **Step 2: In `LFORow` constructor, add waveEditor setup**

```cpp
waveEditor.setVisible(false);
addChildComponent(waveEditor); // addChildComponent keeps it invisible but owned
```

- [ ] **Step 3: Override `getIdealHeight` helper + update resized**

Add to `LFORow`:
```cpp
int getIdealHeight() const {
    return waveEditor.isVisible() ? 148 : 28;
}
```

Update `resized()`:
```cpp
void LFORow::resized() {
    auto r = getLocalBounds();
    auto topRow = r.removeFromTop(28).reduced(2, 0);
    // ... (all existing control layout using topRow instead of r) ...
    if (waveEditor.isVisible())
        waveEditor.setBounds(r.reduced(2));
}
```

- [ ] **Step 4: Add expand/collapse logic to `PenguinLFOEditor`**

In `Source/PluginEditor.h`, add to `PenguinLFOEditor`:
```cpp
void onShapeChanged(int rowIndex);
void updateLFORowHeights();
void updateCustomWaveform(int rowIndex);
```

In `Source/PluginEditor.cpp`:

```cpp
void PenguinLFOEditor::onShapeChanged(int i) {
    bool isCustom = (lfoRows[i].shapeBox.getSelectedId() - 1 == static_cast<int>(LFOShape::Custom));
    // Collapse all other custom editors
    for (int j = 0; j < 4; ++j) {
        if (j != i) lfoRows[j].waveEditor.setVisible(false);
    }
    lfoRows[i].waveEditor.setVisible(isCustom);
    if (isCustom) {
        // Sync waveform from processor into the editor
        lfoRows[i].waveEditor.setWaveform(processor.lfos[i].customWave);
    }
    updateLFORowHeights();
    syncLFOFromRow(i);
}

void PenguinLFOEditor::updateLFORowHeights() {
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(30); // header
    area.removeFromBottom(60); // visualizer
    area.removeFromBottom(50); // filter section (Task 9)

    for (auto& row : lfoRows) {
        int h = row.getIdealHeight();
        row.setBounds(area.removeFromTop(h));
    }
    // Update total editor height
    int totalH = 30 + 8 + 60 + 50 + 8; // header + visualizer + filter + padding
    for (auto& row : lfoRows) totalH += row.getIdealHeight();
    setSize(520, totalH);
}

void PenguinLFOEditor::updateCustomWaveform(int i) {
    processor.updateCustomWaveform(i, lfoRows[i].waveEditor.getWaveform());
}
```

Wire `onChange` callbacks:
```cpp
// In PenguinLFOEditor constructor, after existing callback wiring:
for (int i = 0; i < 4; ++i) {
    lfoRows[i].shapeBox.onChange = [this, i] {
        onShapeChanged(i);
    };
    lfoRows[i].waveEditor.onChange = [this, i] {
        updateCustomWaveform(i);
    };
}
```

- [ ] **Step 5: Add `updateCustomWaveform` to `PluginProcessor`**

In `Source/PluginProcessor.h`:
```cpp
void updateCustomWaveform(int index, const CustomWaveform& wf);
```

In `Source/PluginProcessor.cpp`:
```cpp
void PenguinLFOProcessor::updateCustomWaveform(int index, const CustomWaveform& wf) {
    if (index < 0 || index >= 4) return;
    pendingParamUpdates[index]            = lfos[index];
    pendingParamUpdates[index].customWave = wf;
    paramUpdateMask.fetch_or(1u << index, std::memory_order_release);
}
```

- [ ] **Step 6: Build and test manually** — select Custom shape on any row, verify the editor expands; select a different shape, verify it collapses.

- [ ] **Step 7: Commit**

```bash
git add Source/PluginEditor.h Source/PluginEditor.cpp Source/PluginProcessor.h Source/PluginProcessor.cpp
git commit -m "feat: LFO row expands with waveform editor when Custom shape selected"
```

---

## Task 9: Editor UI — filter section at bottom

**Files:**
- Modify: `Source/PluginEditor.h`
- Modify: `Source/PluginEditor.cpp`

- [ ] **Step 1: Add filter controls to `PenguinLFOEditor` in `Source/PluginEditor.h`**

```cpp
// Inside PenguinLFOEditor private section:
juce::Slider   lowCutSlider, highCutSlider;
juce::Label    lowCutLabel,  highCutLabel;  // show Hz value
juce::Label    lowCutTitle,  highCutTitle;  // static "Low Cut" / "High Cut" text
```

- [ ] **Step 2: Constructor setup in `Source/PluginEditor.cpp`**

```cpp
// Logarithmic sliders: map 0–1 range to 20–20000 Hz
// Use setSkewFactorFromMidPoint so 1000Hz is near the middle
lowCutSlider.setRange(20.0, 20000.0, 1.0);
lowCutSlider.setSkewFactorFromMidPoint(1000.0);
lowCutSlider.setSliderStyle(juce::Slider::LinearHorizontal);
lowCutSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
lowCutSlider.setValue(processor.filterLowCut, juce::dontSendNotification);

highCutSlider.setRange(20.0, 20000.0, 1.0);
highCutSlider.setSkewFactorFromMidPoint(1000.0);
highCutSlider.setSliderStyle(juce::Slider::LinearHorizontal);
highCutSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
highCutSlider.setValue(processor.filterHighCut, juce::dontSendNotification);

auto makeHzLabel = [](juce::Label& lbl, float hz) {
    lbl.setText(juce::String(static_cast<int>(hz)) + " Hz", juce::dontSendNotification);
    lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    lbl.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    lbl.setEditable(true); // click to type
};
makeHzLabel(lowCutLabel,  processor.filterLowCut);
makeHzLabel(highCutLabel, processor.filterHighCut);

lowCutTitle.setText("Low Cut",  juce::dontSendNotification);
highCutTitle.setText("High Cut", juce::dontSendNotification);
lowCutTitle.setColour(juce::Label::textColourId, juce::Colours::grey);
highCutTitle.setColour(juce::Label::textColourId, juce::Colours::grey);

lowCutSlider.onValueChange = [this] {
    float val = static_cast<float>(lowCutSlider.getValue());
    // Enforce: lowCut <= highCut
    if (val > processor.filterHighCut) val = processor.filterHighCut;
    processor.filterLowCut = val;
    lowCutLabel.setText(juce::String(static_cast<int>(val)) + " Hz", juce::dontSendNotification);
};
highCutSlider.onValueChange = [this] {
    float val = static_cast<float>(highCutSlider.getValue());
    if (val < processor.filterLowCut) val = processor.filterLowCut;
    processor.filterHighCut = val;
    highCutLabel.setText(juce::String(static_cast<int>(val)) + " Hz", juce::dontSendNotification);
};
// Handle typed-in values
lowCutLabel.onEditorHide = [this] {
    float val = juce::jlimit(20.0f, 20000.0f, lowCutLabel.getText().getFloatValue());
    lowCutSlider.setValue(val, juce::sendNotification);
};
highCutLabel.onEditorHide = [this] {
    float val = juce::jlimit(20.0f, 20000.0f, highCutLabel.getText().getFloatValue());
    highCutSlider.setValue(val, juce::sendNotification);
};

addAndMakeVisible(lowCutSlider);  addAndMakeVisible(lowCutLabel);  addAndMakeVisible(lowCutTitle);
addAndMakeVisible(highCutSlider); addAndMakeVisible(highCutLabel); addAndMakeVisible(highCutTitle);
```

- [ ] **Step 3: Layout in `resized()`**

In `PenguinLFOEditor::resized()`, replace the existing layout with:
```cpp
void PenguinLFOEditor::resized() {
    auto area = getLocalBounds().reduced(8);
    auto header = area.removeFromTop(30);
    presetBox.setBounds(header.removeFromLeft(260));
    header.removeFromLeft(8);
    saveButton.setBounds(header.removeFromLeft(60));

    visualizer.setBounds(area.removeFromBottom(60));
    area.removeFromBottom(4);

    // Filter section at bottom (above visualizer)
    auto filterArea = area.removeFromBottom(46);
    filterArea.removeFromTop(4);
    g.setColour(juce::Colours::grey); // drawn in paint()
    int halfW = filterArea.getWidth() / 2 - 4;
    auto lcArea = filterArea.removeFromLeft(halfW);
    lowCutTitle.setBounds(lcArea.removeFromLeft(60));
    lowCutSlider.setBounds(lcArea.removeFromLeft(100));
    lowCutLabel.setBounds(lcArea.removeFromLeft(60));
    filterArea.removeFromLeft(8);
    highCutTitle.setBounds(filterArea.removeFromLeft(60));
    highCutSlider.setBounds(filterArea.removeFromLeft(100));
    highCutLabel.setBounds(filterArea.removeFromLeft(60));

    // LFO rows — variable height
    updateLFORowHeights();  // handles its own layout from remaining area
}
```

Note: `resized()` cannot call `g.setColour` (that's in `paint`). Draw the filter section border in `paint` instead:
```cpp
void PenguinLFOEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a2e));
    // title
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("Bold")));
    g.drawText("PenguinLFO", getLocalBounds().removeFromTop(30).reduced(8, 4),
               juce::Justification::centredLeft);
    // filter section border
    auto filterRect = /* compute same area as in resized */;
    g.setColour(juce::Colour(0xff333355));
    g.drawRoundedRectangle(filterRect.toFloat(), 3.0f, 1.0f);
    g.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    g.setColour(juce::Colours::grey);
    g.drawText("Filter", filterRect.removeFromLeft(36).toFloat(), juce::Justification::centredLeft);
}
```

To avoid duplicating the filter rect calculation, store it as a member `juce::Rectangle<int> filterSectionBounds` updated in `resized()`.

- [ ] **Step 4: Build and verify the filter sliders appear and update `processor.filterLowCut/High`**

- [ ] **Step 5: Commit**

```bash
git add Source/PluginEditor.h Source/PluginEditor.cpp
git commit -m "feat: filter section UI with low-cut/high-cut sliders and Hz readouts"
```

---

## Task 10: WaveformVisualizer — custom waveform + pitch center line

**Files:**
- Modify: `Source/WaveformVisualizer.h`
- Modify: `Source/WaveformVisualizer.cpp`

The visualizer needs `filterLowCut`/`filterHighCut` for context (so it can pass them to the simulated processor) and needs to handle the `Custom` shape.

- [ ] **Step 1: Add pitch center and filter params to `updateLFOs` signature**

In `Source/WaveformVisualizer.h`:
```cpp
void updateLFOs(const std::array<LFOInstance, 4>& lfos, float bpm, float sampleRate,
                float filterLowCut = 20.0f, float filterHighCut = 20000.0f);
```

Add members:
```cpp
float filterLowCut  = 20.0f;
float filterHighCut = 20000.0f;
```

- [ ] **Step 2: Update `updateLFOs` implementation**

```cpp
void WaveformVisualizer::updateLFOs(const std::array<LFOInstance, 4>& lfos,
                                     float bpm_, float sampleRate_,
                                     float lowCut, float highCut) {
    lfoCopy     = lfos;
    bpm         = bpm_;
    sampleRate  = sampleRate_;
    filterLowCut  = lowCut;
    filterHighCut = highCut;
}
```

- [ ] **Step 3: Update `timerCallback` in `PenguinLFOEditor` to pass filter params**

```cpp
void PenguinLFOEditor::timerCallback() {
    visualizer.updateLFOs(processor.lfos, processor.currentBPM,
                          static_cast<float>(processor.currentSampleRate),
                          processor.filterLowCut, processor.filterHighCut);
}
```

- [ ] **Step 4: Update `paint` in `WaveformVisualizer.cpp`**

The current paint simulates only Volume LFOs. Extend to also draw a dashed horizontal line for each Pitch LFO at its pitch center position, and handle the Custom shape via the existing `lfoValueAtPhase`:

```cpp
void WaveformVisualizer::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff111122));
    g.setColour(juce::Colour(0xff333355));
    g.drawRect(getLocalBounds());

    int   w = getWidth(), h = getHeight();
    float totalBeats      = 4.0f;
    float samplesPerPoint = (totalBeats * 60.0f / bpm * sampleRate) / w;

    std::array<LFOInstance, 4> sim = lfoCopy;
    for (auto& lfo : sim) lfo.phase = 0.0f;

    // Volume path (existing logic, now passes sampleRate to lfoAdvance)
    juce::Path path;
    for (int x = 0; x < w; ++x) {
        float gain = 1.0f;
        for (int i = 0; i < 4; ++i) {
            if (!sim[i].enabled || sim[i].target != LFOTarget::Volume) continue;
            float inc = lfoPhaseIncrement(sim[i].rateIndex, bpm, sampleRate);
            for (int s = 0; s < static_cast<int>(samplesPerPoint); ++s)
                lfoAdvance(sim[i], inc, sampleRate);
            float val = lfoValueAtPhase(sim[i], sim[i].phase);
            gain *= 1.0f - sim[i].depth * (1.0f - (val * 0.5f + 0.5f));
        }
        float y = (1.0f - gain) * (h - 4) + 2;
        x == 0 ? path.startNewSubPath(x, y) : path.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xff7ec8e3));
    g.strokePath(path, juce::PathStrokeType(1.5f));

    // Pitch center lines — one dashed line per enabled Pitch LFO
    for (int i = 0; i < 4; ++i) {
        if (!lfoCopy[i].enabled || lfoCopy[i].target != LFOTarget::Pitch) continue;
        // Map pitchCenter [-1,1] to screen Y: center=0 → midY, center=1 → top, center=-1 → bottom
        float centerNorm = (lfoCopy[i].pitchCenter + 1.0f) * 0.5f; // [0,1]
        float lineY = (1.0f - centerNorm) * (h - 4) + 2;
        g.setColour(juce::Colour(0x66ffaa44)); // semi-transparent orange
        float dashLen = 6.0f, gapLen = 4.0f;
        for (float x = 0; x < w; x += dashLen + gapLen)
            g.drawHorizontalLine(static_cast<int>(lineY), x, x + dashLen);
    }
}
```

- [ ] **Step 5: Build and verify pitch center dashed line appears when a Pitch LFO is active**

- [ ] **Step 6: Commit**

```bash
git add Source/WaveformVisualizer.h Source/WaveformVisualizer.cpp Source/PluginEditor.cpp
git commit -m "feat: visualizer renders custom waveform and pitch center indicator line"
```

---

## Task 11: Factory presets (15+ total, showcasing new features)

**Files:**
- Modify: `Presets/presets.json`
- Modify: `tests/test_preset_manager.cpp`

The 7 existing presets stay. Add 15 more = 22 total. Each preset includes `filterLowCut`, `filterHighCut`, and per-LFO `smoothing`/`pitchCenter` fields. Custom waveform presets require `customWave` on that LFO.

- [ ] **Step 1: Update preset count test in `tests/test_preset_manager.cpp`**

```cpp
// Change:
REQUIRE(presets.size() >= 7);
// To:
REQUIRE(presets.size() >= 22);
```

- [ ] **Step 2: Verify test fails** (only 7 presets exist)

- [ ] **Step 3: Write 15 new presets into `Presets/presets.json`** (append after existing 7):

```json
,
{
  "name": "Soft Saw Up",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {"shape":"SawUp","rate":"1/4","target":"Volume","depth":0.8,"enabled":true,"smoothing":0.15,"pitchCenter":0.0},
    {"shape":"Sine", "rate":"1/2","target":"Filter","depth":0.3,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine", "rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine", "rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Soft Saw Down",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {"shape":"SawDown","rate":"1/4","target":"Volume","depth":0.8,"enabled":true,"smoothing":0.15,"pitchCenter":0.0},
    {"shape":"Sine",   "rate":"1/2","target":"Pan",  "depth":0.3,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Soft Gate",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {"shape":"Square","rate":"1/8","target":"Volume","depth":0.9,"enabled":true,"smoothing":0.2,"pitchCenter":0.0},
    {"shape":"Sine",  "rate":"1/4","target":"Filter","depth":0.3,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Filter Sweep Narrow",
  "filterLowCut": 500.0, "filterHighCut": 4000.0,
  "lfos": [
    {"shape":"Sine","rate":"1/2","target":"Filter","depth":0.8,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Filter Sweep Wide",
  "filterLowCut": 80.0, "filterHighCut": 16000.0,
  "lfos": [
    {"shape":"SawUp","rate":"2/1","target":"Filter","depth":1.0,"enabled":true,"smoothing":0.1,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Pitch Up Only",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {"shape":"Sine","rate":"1/4","target":"Pitch","depth":0.4,"enabled":true,"smoothing":0.0,"pitchCenter":1.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Pitch Down Only",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {"shape":"Sine","rate":"1/4","target":"Pitch","depth":0.4,"enabled":true,"smoothing":0.0,"pitchCenter":-1.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Pitch Vibrato Centered",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {"shape":"Sine","rate":"1/4t","target":"Pitch","depth":0.25,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.2,"enabled":true,"smoothing":0.05,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Filter + Tremolo",
  "filterLowCut": 200.0, "filterHighCut": 8000.0,
  "lfos": [
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.6,"enabled":true,"smoothing":0.05,"pitchCenter":0.0},
    {"shape":"Triangle","rate":"1/2","target":"Filter","depth":0.7,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Triplet Filter Sweep",
  "filterLowCut": 300.0, "filterHighCut": 12000.0,
  "lfos": [
    {"shape":"SawUp","rate":"1/4t","target":"Filter","depth":0.9,"enabled":true,"smoothing":0.08,"pitchCenter":0.0},
    {"shape":"Square","rate":"1/8","target":"Volume","depth":0.5,"enabled":true,"smoothing":0.15,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Deep Pan + Pitch",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {"shape":"Sine","rate":"2/1","target":"Pan",  "depth":1.0,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/2","target":"Pitch","depth":0.3,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Odd Groove",
  "filterLowCut": 100.0, "filterHighCut": 10000.0,
  "lfos": [
    {"shape":"Square",  "rate":"7/16","target":"Volume","depth":0.85,"enabled":true,"smoothing":0.12,"pitchCenter":0.0},
    {"shape":"Triangle","rate":"5/16","target":"Filter","depth":0.6, "enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine",    "rate":"3/16","target":"Pan",   "depth":0.4, "enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Gentle Combo",
  "filterLowCut": 60.0, "filterHighCut": 14000.0,
  "lfos": [
    {"shape":"Sine","rate":"1/1",  "target":"Volume","depth":0.4, "enabled":true,"smoothing":0.05,"pitchCenter":0.0},
    {"shape":"Sine","rate":"2/1",  "target":"Filter","depth":0.3, "enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/2",  "target":"Pan",   "depth":0.25,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4t","target":"Pitch",  "depth":0.15,"enabled":true,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Custom Heartbeat",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {
      "shape":"Custom","rate":"1/2","target":"Volume","depth":0.9,"enabled":true,"smoothing":0.05,"pitchCenter":0.0,
      "customWave":{
        "isStepMode":false,"stepCount":16,"steps":[],
        "nodes":[
          {"x":0.0,"y":0.0},{"x":0.1,"y":1.0},{"x":0.2,"y":-0.3},
          {"x":0.3,"y":0.8},{"x":0.45,"y":-1.0},{"x":0.5,"y":-1.0},
          {"x":0.7,"y":-0.1},{"x":1.0,"y":0.0}
        ]
      }
    },
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
},
{
  "name": "Step Stutter",
  "filterLowCut": 20.0, "filterHighCut": 20000.0,
  "lfos": [
    {
      "shape":"Custom","rate":"1/4","target":"Volume","depth":1.0,"enabled":true,"smoothing":0.0,"pitchCenter":0.0,
      "customWave":{
        "isStepMode":true,"stepCount":16,
        "steps":[1,0,1,1,-1,0,1,0,0,1,-1,1,0,0,1,-1],
        "nodes":[]
      }
    },
    {"shape":"SampleAndHold","rate":"1/16","target":"Pan","depth":0.5,"enabled":true,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0},
    {"shape":"Sine","rate":"1/4","target":"Volume","depth":0.0,"enabled":false,"smoothing":0.0,"pitchCenter":0.0}
  ]
}
```

- [ ] **Step 4: Run tests — expect PASS (22 presets, new fields round-trip)**

- [ ] **Step 5: Final manual test checklist**

In Visual Studio: build → copy DLL/VST3 to DAW plugin folder → rescan in MPC 2.

- [ ] Saw wave with smoothing=0 clicks; smoothing=0.2+ removes click
- [ ] Custom shape row expands; breakpoint mode: click adds nodes, drag moves them, double-click removes
- [ ] Custom shape row: switch to Step mode shows step bars, drag changes height
- [ ] Only one custom editor open at a time
- [ ] Filter Low Cut / High Cut sliders update sound; LFO Filter target sweeps within that range
- [ ] Pitch LFO with pitchCenter=+1.0 only bends up; -1.0 only bends down; 0.0 symmetric
- [ ] Dashed pitch center line appears in visualizer for Pitch LFOs
- [ ] Load old preset (any of the 7 originals) — no crash, all new fields default correctly
- [ ] Save/load user preset — custom waveform survives round-trip

- [ ] **Step 6: Commit**

```bash
git add Presets/presets.json tests/test_preset_manager.cpp
git commit -m "feat: 15 new factory presets covering smoothing, custom waveforms, filter, pitch center"
```

---

## Resuming Next Session

If you run out of tokens, paste this into the next session:

> "Resume PenguinLFO implementation. Plan is at `docs/superpowers/plans/2026-05-20-penguin-lfo-features.md`. Check the Progress Log section at the top to see which tasks are done. Pick up from the first unchecked task. Use the superpowers:executing-plans skill."
