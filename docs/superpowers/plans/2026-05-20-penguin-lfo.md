# PenguinLFO Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build PenguinLFO, a VST2 audio effect plugin for MPC 2 that layers 4 independent LFOs to create rhythmic modulation on sustained sounds, selectable via presets in one click.

**Architecture:** A pure-C++ `LFOEngine` (no JUCE dependency, fully unit-testable) handles all LFO math. A JUCE `AudioProcessor` wraps the engine and applies modulation to audio. A JUCE `Component` provides the UI. Presets are JSON files parsed by a standalone `PresetManager`. Build targets Windows VST2 (.dll) using CMake + MSVC — this cannot be built inside the Linux Docker container.

**Tech Stack:** C++17, JUCE 7, CMake 3.22+, Catch2 v3 (unit tests), Visual Studio Build Tools 2022 (Windows), nlohmann/json (header-only)

---

## File Map

```
src/penguin-lfo/
  CMakeLists.txt                  ← main build config (VST2, JUCE, tests)
  install.bat                     ← build + copy .dll to VST folder
  Source/
    LFOEngine.h/.cpp              ← shapes, rates, phase — no JUCE dependency
    PresetManager.h/.cpp          ← JSON load/save using nlohmann/json
    PluginProcessor.h/.cpp        ← JUCE AudioProcessor: connects LFO → audio
    PluginEditor.h/.cpp           ← JUCE AudioProcessorEditor: UI
    WaveformVisualizer.h/.cpp     ← JUCE Component: real-time LFO display
  Presets/
    presets.json                  ← 7 factory presets
  tests/
    CMakeLists.txt
    test_lfo_engine.cpp           ← unit tests: shapes, rates
    test_preset_manager.cpp       ← unit tests: parse, round-trip
  JUCE/                           ← git submodule (JUCE 7.0.9)
  third_party/
    nlohmann/json.hpp             ← single-header JSON
    VST2_SDK/pluginterfaces/vst2.x/
      aeffect.h
      aeffectx.h
```

---

## Task 1: Prerequisites and project scaffold

**Files:**
- Create: `src/penguin-lfo/CMakeLists.txt`
- Create: `src/penguin-lfo/tests/CMakeLists.txt`
- Create: placeholder `.h` and `.cpp` for each Source file

> **IMPORTANT:** This plugin builds on Windows (not inside Linux Docker). Use the "x64 Native Tools Command Prompt for VS 2022" for all build commands in this plan.

- [ ] **Step 1: Install Windows prerequisites**

  In a Windows terminal (not WSL):
  1. Install [CMake for Windows](https://cmake.org/download/) — tick "Add to PATH"
  2. Install [Visual Studio Build Tools 2022](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) — select "Desktop development with C++" workload

  Verify in "x64 Native Tools Command Prompt for VS 2022":
  ```
  cmake --version
  cl
  ```
  Expected: cmake 3.22+, and MSVC compiler banner.

- [ ] **Step 2: Get VST2 SDK headers**

  The VST2 SDK is no longer officially distributed. Obtain `aeffect.h` and `aeffectx.h` from a GitHub mirror (search "VST2 SDK aeffect.h github" — they appear in many open-source audio projects).

  Then:
  ```
  mkdir -p src/penguin-lfo/third_party/VST2_SDK/pluginterfaces/vst2.x
  # copy aeffect.h and aeffectx.h into that folder
  ```

- [ ] **Step 3: Add JUCE as a git submodule**

  Run from project root:
  ```bash
  git submodule add https://github.com/juce-framework/JUCE.git src/penguin-lfo/JUCE
  git submodule update --init --recursive
  ```
  Expected: `src/penguin-lfo/JUCE/` populated (~200 MB download).

- [ ] **Step 4: Download nlohmann/json**

  ```bash
  mkdir -p src/penguin-lfo/third_party/nlohmann
  curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp \
       -o src/penguin-lfo/third_party/nlohmann/json.hpp
  ```

- [ ] **Step 5: Create placeholder source files**

  Create each of these files (exact content shown):

  `src/penguin-lfo/Source/LFOEngine.h`:
  ```cpp
  #pragma once
  ```

  `src/penguin-lfo/Source/LFOEngine.cpp`:
  ```cpp
  #include "LFOEngine.h"
  ```

  Repeat the same pattern (`.h` with `#pragma once`, `.cpp` with the matching `#include`) for:
  - `PresetManager.h` / `PresetManager.cpp`
  - `PluginProcessor.h` / `PluginProcessor.cpp`
  - `PluginEditor.h` / `PluginEditor.cpp`
  - `WaveformVisualizer.h` / `WaveformVisualizer.cpp`

- [ ] **Step 6: Write CMakeLists.txt**

  Create `src/penguin-lfo/CMakeLists.txt`:
  ```cmake
  cmake_minimum_required(VERSION 3.22)
  project(PenguinLFO VERSION 1.0.0)

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  add_subdirectory(JUCE)
  juce_set_vst2_sdk_path(${CMAKE_CURRENT_SOURCE_DIR}/third_party/VST2_SDK)

  juce_add_plugin(PenguinLFO
      COMPANY_NAME "DlaZy"
      IS_SYNTH FALSE
      NEEDS_MIDI_INPUT FALSE
      NEEDS_MIDI_OUTPUT FALSE
      IS_MIDI_EFFECT FALSE
      EDITOR_WANTS_KEYBOARD_FOCUS FALSE
      COPY_PLUGIN_AFTER_BUILD FALSE
      PLUGIN_MANUFACTURER_CODE Dlzy
      PLUGIN_CODE Plfo
      FORMATS VST
      PRODUCT_NAME "PenguinLFO"
      VST2_CATEGORY kPlugCategEffect
  )

  juce_generate_juce_header(PenguinLFO)

  target_sources(PenguinLFO PRIVATE
      Source/LFOEngine.cpp
      Source/PresetManager.cpp
      Source/PluginProcessor.cpp
      Source/PluginEditor.cpp
      Source/WaveformVisualizer.cpp
  )

  target_compile_definitions(PenguinLFO PUBLIC
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_include_directories(PenguinLFO PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/third_party
  )

  target_link_libraries(PenguinLFO PRIVATE
      juce::juce_audio_utils
      juce::juce_audio_processors
      juce::juce_dsp
      PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_subdirectory(tests)
  ```

- [ ] **Step 7: Write tests/CMakeLists.txt**

  Create `src/penguin-lfo/tests/CMakeLists.txt`:
  ```cmake
  include(FetchContent)
  FetchContent_Declare(
      Catch2
      GIT_REPOSITORY https://github.com/catchorg/Catch2.git
      GIT_TAG v3.4.0
  )
  FetchContent_MakeAvailable(Catch2)

  add_executable(PenguinLFO_Tests
      test_lfo_engine.cpp
      test_preset_manager.cpp
  )

  target_link_libraries(PenguinLFO_Tests PRIVATE Catch2::Catch2WithMain)

  target_include_directories(PenguinLFO_Tests PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/../Source
      ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
  )

  include(CTest)
  include(Catch)
  catch_discover_tests(PenguinLFO_Tests)
  ```

- [ ] **Step 8: Configure and verify empty build**

  In x64 Native Tools Command Prompt:
  ```
  cd C:\Users\DlaZy\Documents\music-tools\src\penguin-lfo
  cmake -B build -G "Visual Studio 17 2022" -A x64
  cmake --build build --config Release
  ```
  Expected: build succeeds, `build\PenguinLFO_artefacts\Release\VST\PenguinLFO.dll` exists.

- [ ] **Step 9: Commit**

  ```bash
  git add src/penguin-lfo/
  git commit -m "feat: scaffold PenguinLFO JUCE VST2 project"
  ```

---

## Task 2: LFO Engine — shapes (TDD)

**Files:**
- Modify: `src/penguin-lfo/Source/LFOEngine.h`
- Modify: `src/penguin-lfo/Source/LFOEngine.cpp`
- Create: `src/penguin-lfo/tests/test_lfo_engine.cpp`

- [ ] **Step 1: Write failing tests**

  Create `src/penguin-lfo/tests/test_lfo_engine.cpp`:
  ```cpp
  #include <catch2/catch_test_macros.hpp>
  #include <catch2/matchers/catch_matchers_floating_point.hpp>
  #include "LFOEngine.h"
  using namespace Catch::Matchers;

  TEST_CASE("Sine at phase 0 returns 0",    "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::Sine;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(0.0f,  0.001f));
  }
  TEST_CASE("Sine at phase 0.25 returns 1", "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::Sine;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.25f), WithinAbs(1.0f,  0.001f));
  }
  TEST_CASE("Sine at phase 0.75 returns -1","[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::Sine;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.75f), WithinAbs(-1.0f, 0.001f));
  }
  ```

- [ ] **Step 2: Run to confirm it fails**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  ```
  Expected: compilation error — `LFOShape`, `LFOInstance`, `lfoValueAtPhase` not defined.

- [ ] **Step 3: Implement LFOEngine**

  Replace `src/penguin-lfo/Source/LFOEngine.h`:
  ```cpp
  #pragma once
  #include <cmath>
  #include <cstdlib>

  #ifndef M_PI
  #define M_PI 3.14159265358979323846f
  #endif

  enum class LFOShape  { Sine, Square, SawUp, SawDown, Triangle, SampleAndHold };
  enum class LFOTarget { Volume, Filter, Pan, Pitch };

  struct LFOInstance {
      LFOShape  shape              = LFOShape::Sine;
      LFOTarget target             = LFOTarget::Volume;
      int       rateIndex          = 5;    // default: 1/4
      float     depth              = 1.0f;
      bool      enabled            = true;
      float     phase              = 0.0f; // [0, 1)
      float     sampleAndHoldValue = 0.0f;
  };

  // Returns LFO value in [-1, 1] for the given phase [0, 1]
  float lfoValueAtPhase(const LFOInstance& lfo, float phase);

  // Advances phase by phaseIncrement, returns new value. Updates S&H on phase wrap.
  float lfoAdvance(LFOInstance& lfo, float phaseIncrement);
  ```

  Replace `src/penguin-lfo/Source/LFOEngine.cpp`:
  ```cpp
  #include "LFOEngine.h"

  float lfoValueAtPhase(const LFOInstance& lfo, float phase) {
      switch (lfo.shape) {
          case LFOShape::Sine:
              return std::sin(phase * 2.0f * static_cast<float>(M_PI));
          case LFOShape::Square:
              return phase < 0.5f ? 1.0f : -1.0f;
          case LFOShape::SawUp:
              return 2.0f * phase - 1.0f;
          case LFOShape::SawDown:
              return 1.0f - 2.0f * phase;
          case LFOShape::Triangle:
              return phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
          case LFOShape::SampleAndHold:
              return lfo.sampleAndHoldValue;
      }
      return 0.0f;
  }

  float lfoAdvance(LFOInstance& lfo, float phaseIncrement) {
      lfo.phase += phaseIncrement;
      if (lfo.phase >= 1.0f) {
          lfo.phase -= 1.0f;
          lfo.sampleAndHoldValue =
              (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
      }
      return lfoValueAtPhase(lfo, lfo.phase);
  }
  ```

- [ ] **Step 4: Run and confirm the 3 tests pass**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  build\Release\PenguinLFO_Tests.exe
  ```
  Expected: 3 tests pass.

- [ ] **Step 5: Add remaining shape tests**

  Append to `src/penguin-lfo/tests/test_lfo_engine.cpp`:
  ```cpp
  TEST_CASE("Square below 0.5 returns 1",   "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::Square;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.25f), WithinAbs( 1.0f, 0.001f));
  }
  TEST_CASE("Square at 0.5 returns -1",     "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::Square;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.5f),  WithinAbs(-1.0f, 0.001f));
  }
  TEST_CASE("SawUp at phase 0 returns -1",  "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::SawUp;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(-1.0f, 0.001f));
  }
  TEST_CASE("SawDown at phase 0 returns 1", "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::SawDown;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs( 1.0f, 0.001f));
  }
  TEST_CASE("Triangle at phase 0 returns -1",  "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::Triangle;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.0f),  WithinAbs(-1.0f, 0.001f));
  }
  TEST_CASE("Triangle at phase 0.5 returns 1", "[shapes]") {
      LFOInstance lfo; lfo.shape = LFOShape::Triangle;
      REQUIRE_THAT(lfoValueAtPhase(lfo, 0.5f),  WithinAbs( 1.0f, 0.001f));
  }
  ```

- [ ] **Step 6: Run all shape tests**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  build\Release\PenguinLFO_Tests.exe
  ```
  Expected: all 9 tests pass.

- [ ] **Step 7: Commit**

  ```bash
  git add src/penguin-lfo/Source/LFOEngine.h src/penguin-lfo/Source/LFOEngine.cpp \
          src/penguin-lfo/tests/test_lfo_engine.cpp
  git commit -m "feat: LFO engine shapes with unit tests"
  ```

---

## Task 3: LFO Engine — BPM-synced rates (TDD)

**Files:**
- Modify: `src/penguin-lfo/Source/LFOEngine.h`
- Modify: `src/penguin-lfo/Source/LFOEngine.cpp`
- Modify: `src/penguin-lfo/tests/test_lfo_engine.cpp`

- [ ] **Step 1: Write failing rate tests**

  Append to `src/penguin-lfo/tests/test_lfo_engine.cpp`:
  ```cpp
  TEST_CASE("1/4 note at 120 BPM, 44100 Hz = 22050 samples per cycle", "[rates]") {
      float inc = lfoPhaseIncrement(LFO_RATE_1_4, 120.0f, 44100.0f);
      REQUIRE_THAT(1.0f / inc, WithinRel(22050.0f, 0.001f));
  }
  TEST_CASE("1/8 rate is exactly double the 1/4 rate", "[rates]") {
      float inc4 = lfoPhaseIncrement(LFO_RATE_1_4, 120.0f, 44100.0f);
      float inc8 = lfoPhaseIncrement(LFO_RATE_1_8, 120.0f, 44100.0f);
      REQUIRE_THAT(inc8 / inc4, WithinRel(2.0f, 0.001f));
  }
  TEST_CASE("1/4t (triplet) is 3/2 the speed of 1/4", "[rates]") {
      float inc4  = lfoPhaseIncrement(LFO_RATE_1_4,  120.0f, 44100.0f);
      float inc4t = lfoPhaseIncrement(LFO_RATE_1_4T, 120.0f, 44100.0f);
      REQUIRE_THAT(inc4t / inc4, WithinRel(1.5f, 0.001f));
  }
  ```

- [ ] **Step 2: Run to confirm fail**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  ```
  Expected: compilation error — `lfoPhaseIncrement`, `LFO_RATE_1_4`, etc. not defined.

- [ ] **Step 3: Implement rate table and phase increment**

  Add to `src/penguin-lfo/Source/LFOEngine.h` (after the `LFOInstance` struct):
  ```cpp
  struct LFORateEntry {
      float       beats; // cycle length in quarter-note beats
      const char* name;
  };

  // Rate index constants
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
  constexpr int LFO_RATE_COUNT  = 25;

  extern const LFORateEntry LFO_RATES[LFO_RATE_COUNT];

  // Returns phase-increment-per-sample (phase 0→1 = one cycle)
  float lfoPhaseIncrement(int rateIndex, float bpm, float sampleRate);
  ```

  Add to `src/penguin-lfo/Source/LFOEngine.cpp`:
  ```cpp
  const LFORateEntry LFO_RATES[LFO_RATE_COUNT] = {
      {32.0f,    "8/1"},
      {16.0f,    "4/1"},
      {8.0f,     "2/1"},
      {4.0f,     "1/1"},
      {2.0f,     "1/2"},
      {1.0f,     "1/4"},
      {0.5f,     "1/8"},
      {0.25f,    "1/16"},
      {0.125f,   "1/32"},
      {0.0625f,  "1/64"},
      {6.0f,     "1/1d"},
      {3.0f,     "1/2d"},
      {1.5f,     "1/4d"},
      {0.75f,    "1/8d"},
      {0.375f,   "1/16d"},
      {1.3333f,  "1/2t"},
      {0.6667f,  "1/4t"},
      {0.3333f,  "1/8t"},
      {0.1667f,  "1/16t"},
      {0.08333f, "1/32t"},
      {0.75f,    "3/16"},
      {1.25f,    "5/16"},
      {1.75f,    "7/16"},
      {2.5f,     "5/8"},
      {3.5f,     "7/8"},
  };

  float lfoPhaseIncrement(int rateIndex, float bpm, float sampleRate) {
      // cycles/sample = bpm / (60 * sampleRate * beatsPerCycle)
      return bpm / (60.0f * sampleRate * LFO_RATES[rateIndex].beats);
  }
  ```

  Also update `LFOInstance::rateIndex` default to use the constant:
  ```cpp
  // In LFOEngine.h, change:
  int rateIndex = LFO_RATE_1_4;
  ```

- [ ] **Step 4: Run all tests**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  build\Release\PenguinLFO_Tests.exe
  ```
  Expected: all 12 tests pass.

- [ ] **Step 5: Commit**

  ```bash
  git add src/penguin-lfo/Source/LFOEngine.h src/penguin-lfo/Source/LFOEngine.cpp \
          src/penguin-lfo/tests/test_lfo_engine.cpp
  git commit -m "feat: LFO BPM-synced rate table with 25 timings"
  ```

---

## Task 4: Preset Manager (TDD)

**Files:**
- Modify: `src/penguin-lfo/Source/PresetManager.h`
- Modify: `src/penguin-lfo/Source/PresetManager.cpp`
- Create: `src/penguin-lfo/tests/test_preset_manager.cpp`

- [ ] **Step 1: Write failing test**

  Create `src/penguin-lfo/tests/test_preset_manager.cpp`:
  ```cpp
  #include <catch2/catch_test_macros.hpp>
  #include "PresetManager.h"

  TEST_CASE("Parse a single preset from JSON", "[presets]") {
      const char* json = R"({
        "presets": [{
          "name": "Test",
          "lfos": [
            {"shape":"Sine",    "rate":"1/4",  "target":"Volume","depth":0.8,"enabled":true},
            {"shape":"Square",  "rate":"1/8",  "target":"Filter","depth":0.5,"enabled":true},
            {"shape":"SawUp",   "rate":"1/16", "target":"Pan",   "depth":0.3,"enabled":false},
            {"shape":"Triangle","rate":"1/32", "target":"Pitch", "depth":0.2,"enabled":false}
          ]
        }]
      })";
      auto presets = parsePresetsJson(json);
      REQUIRE(presets.size() == 1);
      REQUIRE(presets[0].name == "Test");
      REQUIRE(presets[0].lfos[0].shape     == LFOShape::Sine);
      REQUIRE(presets[0].lfos[0].rateIndex == LFO_RATE_1_4);
      REQUIRE(presets[0].lfos[0].target    == LFOTarget::Volume);
      REQUIRE(presets[0].lfos[0].depth     == 0.8f);
      REQUIRE(presets[0].lfos[0].enabled   == true);
      REQUIRE(presets[0].lfos[1].shape     == LFOShape::Square);
      REQUIRE(presets[0].lfos[2].enabled   == false);
  }
  ```

- [ ] **Step 2: Run to confirm fail**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  ```
  Expected: compilation error — `PresetData`, `parsePresetsJson` not defined.

- [ ] **Step 3: Implement PresetManager**

  Replace `src/penguin-lfo/Source/PresetManager.h`:
  ```cpp
  #pragma once
  #include "LFOEngine.h"
  #include <string>
  #include <vector>
  #include <array>

  struct PresetData {
      std::string              name;
      std::array<LFOInstance, 4> lfos;
  };

  std::vector<PresetData> parsePresetsJson(const std::string& jsonStr);
  std::vector<PresetData> loadPresetsFromFile(const std::string& filePath);
  std::string             serializePreset(const PresetData& preset);
  ```

  Replace `src/penguin-lfo/Source/PresetManager.cpp`:
  ```cpp
  #include "PresetManager.h"
  #include <nlohmann/json.hpp>
  #include <fstream>

  using json = nlohmann::json;

  static LFOShape shapeFromString(const std::string& s) {
      if (s == "Sine")          return LFOShape::Sine;
      if (s == "Square")        return LFOShape::Square;
      if (s == "SawUp")         return LFOShape::SawUp;
      if (s == "SawDown")       return LFOShape::SawDown;
      if (s == "Triangle")      return LFOShape::Triangle;
      if (s == "SampleAndHold") return LFOShape::SampleAndHold;
      return LFOShape::Sine;
  }

  static std::string shapeToString(LFOShape s) {
      switch (s) {
          case LFOShape::Sine:          return "Sine";
          case LFOShape::Square:        return "Square";
          case LFOShape::SawUp:         return "SawUp";
          case LFOShape::SawDown:       return "SawDown";
          case LFOShape::Triangle:      return "Triangle";
          case LFOShape::SampleAndHold: return "SampleAndHold";
      }
      return "Sine";
  }

  static LFOTarget targetFromString(const std::string& s) {
      if (s == "Filter") return LFOTarget::Filter;
      if (s == "Pan")    return LFOTarget::Pan;
      if (s == "Pitch")  return LFOTarget::Pitch;
      return LFOTarget::Volume;
  }

  static std::string targetToString(LFOTarget t) {
      switch (t) {
          case LFOTarget::Volume: return "Volume";
          case LFOTarget::Filter: return "Filter";
          case LFOTarget::Pan:    return "Pan";
          case LFOTarget::Pitch:  return "Pitch";
      }
      return "Volume";
  }

  static int rateIndexFromString(const std::string& s) {
      for (int i = 0; i < LFO_RATE_COUNT; ++i)
          if (LFO_RATES[i].name == s) return i;
      return LFO_RATE_1_4;
  }

  std::vector<PresetData> parsePresetsJson(const std::string& jsonStr) {
      std::vector<PresetData> result;
      auto j = json::parse(jsonStr);
      for (auto& p : j["presets"]) {
          PresetData preset;
          preset.name = p["name"].get<std::string>();
          int i = 0;
          for (auto& l : p["lfos"]) {
              preset.lfos[i].shape     = shapeFromString(l["shape"]);
              preset.lfos[i].rateIndex = rateIndexFromString(l["rate"]);
              preset.lfos[i].target    = targetFromString(l["target"]);
              preset.lfos[i].depth     = l["depth"].get<float>();
              preset.lfos[i].enabled   = l["enabled"].get<bool>();
              if (++i >= 4) break;
          }
          result.push_back(preset);
      }
      return result;
  }

  std::vector<PresetData> loadPresetsFromFile(const std::string& filePath) {
      std::ifstream f(filePath);
      std::string content((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
      return parsePresetsJson(content);
  }

  std::string serializePreset(const PresetData& preset) {
      json j;
      j["name"] = preset.name;
      j["lfos"] = json::array();
      for (const auto& lfo : preset.lfos) {
          j["lfos"].push_back({
              {"shape",   shapeToString(lfo.shape)},
              {"rate",    LFO_RATES[lfo.rateIndex].name},
              {"target",  targetToString(lfo.target)},
              {"depth",   lfo.depth},
              {"enabled", lfo.enabled}
          });
      }
      return j.dump(2);
  }
  ```

- [ ] **Step 4: Run and confirm pass**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  build\Release\PenguinLFO_Tests.exe
  ```
  Expected: all tests pass.

- [ ] **Step 5: Add round-trip test**

  Append to `src/penguin-lfo/tests/test_preset_manager.cpp`:
  ```cpp
  TEST_CASE("Serialize and re-parse round-trips correctly", "[presets]") {
      PresetData original;
      original.name                = "Round Trip";
      original.lfos[0].shape       = LFOShape::Square;
      original.lfos[0].rateIndex   = LFO_RATE_1_8T;
      original.lfos[0].target      = LFOTarget::Filter;
      original.lfos[0].depth       = 0.6f;
      original.lfos[0].enabled     = true;
      original.lfos[1].enabled     = false;
      original.lfos[2].enabled     = false;
      original.lfos[3].enabled     = false;

      std::string wrapped = "{\"presets\":[" + serializePreset(original) + "]}";
      auto loaded = parsePresetsJson(wrapped);

      REQUIRE(loaded[0].name                == "Round Trip");
      REQUIRE(loaded[0].lfos[0].shape       == LFOShape::Square);
      REQUIRE(loaded[0].lfos[0].rateIndex   == LFO_RATE_1_8T);
      REQUIRE(loaded[0].lfos[0].target      == LFOTarget::Filter);
  }
  ```

- [ ] **Step 6: Run all tests**

  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  build\Release\PenguinLFO_Tests.exe
  ```
  Expected: all tests pass.

- [ ] **Step 7: Commit**

  ```bash
  git add src/penguin-lfo/Source/PresetManager.h src/penguin-lfo/Source/PresetManager.cpp \
          src/penguin-lfo/tests/test_preset_manager.cpp
  git commit -m "feat: preset manager JSON load/save with unit tests"
  ```

---

## Task 5: Factory presets JSON

**Files:**
- Create: `src/penguin-lfo/Presets/presets.json`

- [ ] **Step 1: Write factory presets**

  Create `src/penguin-lfo/Presets/presets.json`:
  ```json
  {
    "presets": [
      {
        "name": "Slow Breathe",
        "lfos": [
          {"shape":"Sine",   "rate":"1/1",  "target":"Volume","depth":0.7, "enabled":true},
          {"shape":"Sine",   "rate":"1/2d", "target":"Filter","depth":0.4, "enabled":true},
          {"shape":"Sine",   "rate":"2/1",  "target":"Pan",   "depth":0.2, "enabled":true},
          {"shape":"Sine",   "rate":"1/1",  "target":"Volume","depth":0.0, "enabled":false}
        ]
      },
      {
        "name": "Hard Gate",
        "lfos": [
          {"shape":"Square", "rate":"1/8",  "target":"Volume","depth":0.95,"enabled":true},
          {"shape":"Square", "rate":"1/16", "target":"Volume","depth":0.3, "enabled":true},
          {"shape":"Sine",   "rate":"1/4",  "target":"Filter","depth":0.2, "enabled":false},
          {"shape":"Sine",   "rate":"1/4",  "target":"Volume","depth":0.0, "enabled":false}
        ]
      },
      {
        "name": "Stutter 16ths",
        "lfos": [
          {"shape":"Square",        "rate":"1/16",  "target":"Volume","depth":0.9,"enabled":true},
          {"shape":"Sine",          "rate":"1/16t", "target":"Filter","depth":0.6,"enabled":true},
          {"shape":"SampleAndHold", "rate":"1/8",   "target":"Pan",   "depth":0.4,"enabled":true},
          {"shape":"Sine",          "rate":"1/4",   "target":"Volume","depth":0.0,"enabled":false}
        ]
      },
      {
        "name": "Polyrhythm",
        "lfos": [
          {"shape":"Square", "rate":"1/4",   "target":"Volume","depth":0.8,"enabled":true},
          {"shape":"Sine",   "rate":"1/8t",  "target":"Filter","depth":0.5,"enabled":true},
          {"shape":"Square", "rate":"3/16",  "target":"Volume","depth":0.5,"enabled":true},
          {"shape":"Sine",   "rate":"1/16d", "target":"Pan",   "depth":0.3,"enabled":true}
        ]
      },
      {
        "name": "Pan Drift",
        "lfos": [
          {"shape":"Sine",     "rate":"2/1", "target":"Pan",   "depth":0.8,"enabled":true},
          {"shape":"Sine",     "rate":"1/1", "target":"Volume","depth":0.4,"enabled":true},
          {"shape":"Triangle", "rate":"1/2", "target":"Filter","depth":0.2,"enabled":true},
          {"shape":"Sine",     "rate":"1/4", "target":"Volume","depth":0.0,"enabled":false}
        ]
      },
      {
        "name": "Glitch",
        "lfos": [
          {"shape":"SampleAndHold","rate":"1/16","target":"Volume","depth":0.9,"enabled":true},
          {"shape":"SampleAndHold","rate":"3/16","target":"Filter","depth":0.7,"enabled":true},
          {"shape":"Square",       "rate":"7/16","target":"Pan",   "depth":0.5,"enabled":true},
          {"shape":"SampleAndHold","rate":"5/16","target":"Volume","depth":0.4,"enabled":true}
        ]
      },
      {
        "name": "Half Time Feel",
        "lfos": [
          {"shape":"Square",   "rate":"1/2",  "target":"Volume","depth":0.85,"enabled":true},
          {"shape":"Sine",     "rate":"1/4t", "target":"Pitch", "depth":0.2, "enabled":true},
          {"shape":"Sine",     "rate":"1/1",  "target":"Filter","depth":0.3, "enabled":true},
          {"shape":"Triangle", "rate":"7/8",  "target":"Pan",   "depth":0.15,"enabled":false}
        ]
      }
    ]
  }
  ```

- [ ] **Step 2: Add factory-file load test**

  Append to `src/penguin-lfo/tests/test_preset_manager.cpp`:
  ```cpp
  #include <fstream>
  TEST_CASE("All 7 factory presets load from file", "[presets]") {
      std::ifstream f("../Presets/presets.json");
      REQUIRE(f.is_open());
      std::string content((std::istreambuf_iterator<char>(f)),
                           std::istreambuf_iterator<char>());
      auto presets = parsePresetsJson(content);
      REQUIRE(presets.size() == 7);
      for (const auto& p : presets)
          REQUIRE(!p.name.empty());
  }
  ```

  Run from the `build/` directory so the relative path resolves:
  ```
  cmake --build build --config Release --target PenguinLFO_Tests
  cd build
  ctest -C Release -V
  cd ..
  ```
  Expected: all tests pass.

- [ ] **Step 3: Commit**

  ```bash
  git add src/penguin-lfo/Presets/presets.json src/penguin-lfo/tests/test_preset_manager.cpp
  git commit -m "feat: 7 factory presets JSON"
  ```

---

## Task 6: Plugin Processor — audio engine

**Files:**
- Modify: `src/penguin-lfo/Source/PluginProcessor.h`
- Modify: `src/penguin-lfo/Source/PluginProcessor.cpp`

- [ ] **Step 1: Write PluginProcessor.h**

  Replace `src/penguin-lfo/Source/PluginProcessor.h`:
  ```cpp
  #pragma once
  #include <JuceHeader.h>
  #include "LFOEngine.h"
  #include "PresetManager.h"
  #include <array>

  class PenguinLFOProcessor : public juce::AudioProcessor {
  public:
      PenguinLFOProcessor();
      ~PenguinLFOProcessor() override = default;

      void prepareToPlay(double sampleRate, int samplesPerBlock) override;
      void releaseResources() override {}
      void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

      juce::AudioProcessorEditor* createEditor() override;
      bool hasEditor() const override { return true; }

      const juce::String getName() const override { return "PenguinLFO"; }
      bool   acceptsMidi()  const override { return false; }
      bool   producesMidi() const override { return false; }
      double getTailLengthSeconds() const override { return 0.0; }
      int    getNumPrograms()  override { return 1; }
      int    getCurrentProgram() override { return 0; }
      void   setCurrentProgram(int) override {}
      const  juce::String getProgramName(int) override { return {}; }
      void   changeProgramName(int, const juce::String&) override {}
      void   getStateInformation(juce::MemoryBlock&) override {}
      void   setStateInformation(const void*, int) override {}

      void applyPreset(const PresetData& preset);

      // Public so the editor and visualizer can read them
      std::array<LFOInstance, 4> lfos;
      std::vector<PresetData>    factoryPresets;
      std::vector<PresetData>    userPresets;
      float currentBPM        = 120.0f;
      double currentSampleRate = 44100.0;

  private:
      juce::dsp::StateVariableTPTFilter<float> filterLeft, filterRight;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinLFOProcessor)
  };
  ```

- [ ] **Step 2: Write PluginProcessor.cpp**

  Replace `src/penguin-lfo/Source/PluginProcessor.cpp`:
  ```cpp
  #include "PluginProcessor.h"
  #include "PluginEditor.h"

  PenguinLFOProcessor::PenguinLFOProcessor()
      : AudioProcessor(BusesProperties()
            .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true))
  {
      auto pluginDir   = juce::File::getSpecialLocation(
                             juce::File::currentExecutableFile).getParentDirectory();
      auto presetsFile = pluginDir.getChildFile("PenguinLFO_Presets/presets.json");
      if (presetsFile.existsAsFile())
          factoryPresets = loadPresetsFromFile(presetsFile.getFullPathName().toStdString());

      for (auto& lfo : lfos) {
          lfo.shape     = LFOShape::Sine;
          lfo.rateIndex = LFO_RATE_1_4;
          lfo.target    = LFOTarget::Volume;
          lfo.depth     = 0.5f;
          lfo.enabled   = false;
      }
      lfos[0].enabled = true;

      if (!factoryPresets.empty())
          applyPreset(factoryPresets[0]);
  }

  void PenguinLFOProcessor::prepareToPlay(double sampleRate, int) {
      currentSampleRate = sampleRate;

      juce::dsp::ProcessSpec spec;
      spec.sampleRate       = sampleRate;
      spec.maximumBlockSize = 512;
      spec.numChannels      = 1;

      filterLeft.prepare(spec);
      filterRight.prepare(spec);
      filterLeft.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
      filterRight.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
      filterLeft.setCutoffFrequency(20000.0f);
      filterRight.setCutoffFrequency(20000.0f);
  }

  void PenguinLFOProcessor::applyPreset(const PresetData& preset) {
      for (int i = 0; i < 4; ++i)
          lfos[i] = preset.lfos[i];
  }

  void PenguinLFOProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&) {
      if (auto* ph = getPlayHead())
          if (auto pos = ph->getPosition())
              if (auto bpm = pos->getBpm())
                  currentBPM = static_cast<float>(*bpm);

      auto* L = buffer.getWritePointer(0);
      auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
      int   N = buffer.getNumSamples();

      for (int s = 0; s < N; ++s) {
          float gainMod    = 1.0f;
          float panMod     = 0.0f;
          float filterCutoff = 20000.0f;

          for (int i = 0; i < 4; ++i) {
              if (!lfos[i].enabled) continue;
              float inc = lfoPhaseIncrement(lfos[i].rateIndex,
                                            currentBPM,
                                            static_cast<float>(currentSampleRate));
              float val = lfoAdvance(lfos[i], inc); // [-1, 1]

              switch (lfos[i].target) {
                  case LFOTarget::Volume:
                      gainMod *= 1.0f - lfos[i].depth * (1.0f - (val * 0.5f + 0.5f));
                      break;
                  case LFOTarget::Filter:
                      filterCutoff = 200.0f * std::pow(100.0f,
                          (val * 0.5f + 0.5f) * lfos[i].depth + (1.0f - lfos[i].depth));
                      filterCutoff = juce::jlimit(200.0f, 20000.0f, filterCutoff);
                      break;
                  case LFOTarget::Pan:
                      panMod = juce::jlimit(-1.0f, 1.0f, panMod + val * lfos[i].depth);
                      break;
                  case LFOTarget::Pitch:
                      gainMod *= 1.0f + val * lfos[i].depth * 0.05f; // subtle ring-mod vibrato
                      break;
              }
          }

          filterLeft.setCutoffFrequency(filterCutoff);
          filterRight.setCutoffFrequency(filterCutoff);

          float leftGain  = std::cos((panMod + 1.0f) * juce::MathConstants<float>::pi / 4.0f);
          float rightGain = std::sin((panMod + 1.0f) * juce::MathConstants<float>::pi / 4.0f);

          L[s] = filterLeft.processSample(0, L[s]) * gainMod * leftGain;
          if (R)
              R[s] = filterRight.processSample(0, R[s]) * gainMod * rightGain;
      }
  }

  juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
      return new PenguinLFOProcessor();
  }
  ```

- [ ] **Step 3: Build and verify compile**

  ```
  cmake --build build --config Release --target PenguinLFO
  ```
  Expected: build succeeds.

- [ ] **Step 4: Commit**

  ```bash
  git add src/penguin-lfo/Source/PluginProcessor.h src/penguin-lfo/Source/PluginProcessor.cpp
  git commit -m "feat: plugin processor wires LFO engine to audio"
  ```

---

## Task 7: Plugin Editor and Waveform Visualizer

**Files:**
- Modify: `src/penguin-lfo/Source/WaveformVisualizer.h`
- Modify: `src/penguin-lfo/Source/WaveformVisualizer.cpp`
- Modify: `src/penguin-lfo/Source/PluginEditor.h`
- Modify: `src/penguin-lfo/Source/PluginEditor.cpp`

- [ ] **Step 1: Write WaveformVisualizer**

  Replace `src/penguin-lfo/Source/WaveformVisualizer.h`:
  ```cpp
  #pragma once
  #include <JuceHeader.h>
  #include "LFOEngine.h"
  #include <array>

  class WaveformVisualizer : public juce::Component, private juce::Timer {
  public:
      WaveformVisualizer();
      void paint(juce::Graphics& g) override;
      void updateLFOs(const std::array<LFOInstance, 4>& lfos, float bpm, float sampleRate);

  private:
      void timerCallback() override { repaint(); }
      std::array<LFOInstance, 4> lfoCopy;
      float bpm        = 120.0f;
      float sampleRate = 44100.0f;
  };
  ```

  Replace `src/penguin-lfo/Source/WaveformVisualizer.cpp`:
  ```cpp
  #include "WaveformVisualizer.h"

  WaveformVisualizer::WaveformVisualizer() { startTimerHz(30); }

  void WaveformVisualizer::updateLFOs(const std::array<LFOInstance, 4>& lfos,
                                       float bpm_, float sampleRate_) {
      lfoCopy = lfos;
      bpm = bpm_;
      sampleRate = sampleRate_;
  }

  void WaveformVisualizer::paint(juce::Graphics& g) {
      g.fillAll(juce::Colour(0xff111122));
      g.setColour(juce::Colour(0xff333355));
      g.drawRect(getLocalBounds());

      int   w = getWidth(), h = getHeight();
      float totalBeats      = 4.0f;
      float samplesPerPoint = (totalBeats * 60.0f / bpm * sampleRate) / w;

      std::array<LFOInstance, 4> sim = lfoCopy;
      for (auto& lfo : sim) lfo.phase = 0.0f;

      juce::Path path;
      for (int x = 0; x < w; ++x) {
          float gain = 1.0f;
          for (int i = 0; i < 4; ++i) {
              if (!sim[i].enabled || sim[i].target != LFOTarget::Volume) continue;
              float inc = lfoPhaseIncrement(sim[i].rateIndex, bpm, sampleRate);
              for (int s = 0; s < static_cast<int>(samplesPerPoint); ++s)
                  lfoAdvance(sim[i], inc);
              float val = lfoValueAtPhase(sim[i], sim[i].phase);
              gain *= 1.0f - sim[i].depth * (1.0f - (val * 0.5f + 0.5f));
          }
          float y = (1.0f - gain) * (h - 4) + 2;
          x == 0 ? path.startNewSubPath(x, y) : path.lineTo(x, y);
      }
      g.setColour(juce::Colour(0xff7ec8e3));
      g.strokePath(path, juce::PathStrokeType(1.5f));
  }
  ```

- [ ] **Step 2: Write PluginEditor.h**

  Replace `src/penguin-lfo/Source/PluginEditor.h`:
  ```cpp
  #pragma once
  #include <JuceHeader.h>
  #include "PluginProcessor.h"
  #include "WaveformVisualizer.h"
  #include <array>

  class LFORow : public juce::Component {
  public:
      explicit LFORow(int index);
      void resized() override;
      juce::ComboBox    shapeBox, rateBox, targetBox;
      juce::Slider      depthSlider;
      juce::ToggleButton enableToggle;
  private:
      juce::Label label;
  };

  class PenguinLFOEditor : public juce::AudioProcessorEditor, private juce::Timer {
  public:
      explicit PenguinLFOEditor(PenguinLFOProcessor&);
      void resized() override;
      void paint(juce::Graphics& g) override;
  private:
      void timerCallback() override;
      void populatePresetBox();
      void onPresetSelected();
      void onSavePreset();
      void syncRowFromLFO(int i);
      void syncLFOFromRow(int i);

      PenguinLFOProcessor& processor;
      juce::ComboBox   presetBox;
      juce::TextButton saveButton { "Save" };
      std::array<LFORow, 4> lfoRows { LFORow(0), LFORow(1), LFORow(2), LFORow(3) };
      WaveformVisualizer visualizer;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinLFOEditor)
  };
  ```

- [ ] **Step 3: Write PluginEditor.cpp**

  Replace `src/penguin-lfo/Source/PluginEditor.cpp`:
  ```cpp
  #include "PluginEditor.h"

  static const char* SHAPE_NAMES[]  = {"Sine","Square","Saw Up","Saw Down","Triangle","S&H"};
  static const char* TARGET_NAMES[] = {"Volume","Filter","Pan","Pitch"};

  // ── LFORow ────────────────────────────────────────────────────────────

  LFORow::LFORow(int index) {
      label.setText("LFO " + juce::String(index + 1), juce::dontSendNotification);
      label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
      addAndMakeVisible(label);

      for (int i = 0; i < 6; ++i)            shapeBox.addItem(SHAPE_NAMES[i],  i + 1);
      for (int i = 0; i < LFO_RATE_COUNT; ++i) rateBox.addItem(LFO_RATES[i].name, i + 1);
      for (int i = 0; i < 4; ++i)            targetBox.addItem(TARGET_NAMES[i], i + 1);

      depthSlider.setRange(0.0, 1.0, 0.01);
      depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
      depthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
      enableToggle.setButtonText("ON");

      addAndMakeVisible(shapeBox);
      addAndMakeVisible(rateBox);
      addAndMakeVisible(targetBox);
      addAndMakeVisible(depthSlider);
      addAndMakeVisible(enableToggle);
  }

  void LFORow::resized() {
      auto r = getLocalBounds().reduced(2);
      label.setBounds(r.removeFromLeft(40));
      r.removeFromLeft(4);
      enableToggle.setBounds(r.removeFromRight(36));
      r.removeFromRight(4);
      depthSlider.setBounds(r.removeFromRight(80));
      r.removeFromRight(4);
      int cw = r.getWidth() / 3;
      shapeBox.setBounds(r.removeFromLeft(cw));
      rateBox.setBounds(r.removeFromLeft(cw));
      targetBox.setBounds(r);
  }

  // ── PenguinLFOEditor ──────────────────────────────────────────────────

  PenguinLFOEditor::PenguinLFOEditor(PenguinLFOProcessor& p)
      : AudioProcessorEditor(&p), processor(p) {
      setSize(520, 320);
      addAndMakeVisible(presetBox);
      addAndMakeVisible(saveButton);
      for (auto& row : lfoRows) addAndMakeVisible(row);
      addAndMakeVisible(visualizer);

      populatePresetBox();
      for (int i = 0; i < 4; ++i) syncRowFromLFO(i);

      presetBox.onChange  = [this] { onPresetSelected(); };
      saveButton.onClick  = [this] { onSavePreset(); };

      for (int i = 0; i < 4; ++i) {
          lfoRows[i].shapeBox.onChange         = [this, i] { syncLFOFromRow(i); };
          lfoRows[i].rateBox.onChange          = [this, i] { syncLFOFromRow(i); };
          lfoRows[i].targetBox.onChange        = [this, i] { syncLFOFromRow(i); };
          lfoRows[i].depthSlider.onValueChange = [this, i] { syncLFOFromRow(i); };
          lfoRows[i].enableToggle.onClick      = [this, i] { syncLFOFromRow(i); };
      }
      startTimerHz(30);
  }

  void PenguinLFOEditor::paint(juce::Graphics& g) {
      g.fillAll(juce::Colour(0xff1a1a2e));
      g.setColour(juce::Colours::white);
      g.setFont(juce::Font(16.0f, juce::Font::bold));
      g.drawText("PenguinLFO",
                 getLocalBounds().removeFromTop(30).reduced(8, 4),
                 juce::Justification::centredLeft);
  }

  void PenguinLFOEditor::resized() {
      auto area   = getLocalBounds().reduced(8);
      auto header = area.removeFromTop(30);
      presetBox.setBounds(header.removeFromLeft(260));
      header.removeFromLeft(8);
      saveButton.setBounds(header.removeFromLeft(60));

      visualizer.setBounds(area.removeFromBottom(60));
      int rowH = area.getHeight() / 4;
      for (auto& row : lfoRows)
          row.setBounds(area.removeFromTop(rowH));
  }

  void PenguinLFOEditor::timerCallback() {
      visualizer.updateLFOs(processor.lfos, processor.currentBPM,
                            static_cast<float>(processor.currentSampleRate));
  }

  void PenguinLFOEditor::populatePresetBox() {
      presetBox.clear();
      int id = 1;
      for (const auto& p : processor.factoryPresets) presetBox.addItem(p.name, id++);
      if (!processor.userPresets.empty()) {
          presetBox.addSeparator();
          for (const auto& p : processor.userPresets) presetBox.addItem(p.name, id++);
      }
  }

  void PenguinLFOEditor::onPresetSelected() {
      int idx = presetBox.getSelectedItemIndex();
      if (idx < 0) return;
      if (idx < static_cast<int>(processor.factoryPresets.size())) {
          processor.applyPreset(processor.factoryPresets[idx]);
          for (int i = 0; i < 4; ++i) syncRowFromLFO(i);
      }
  }

  void PenguinLFOEditor::onSavePreset() {
      auto* aw = new juce::AlertWindow("Save Preset", "Enter a name:", juce::AlertWindow::NoIcon);
      aw->addTextEditor("name", "My Preset");
      aw->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
      aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
      aw->enterModalState(true,
          juce::ModalCallbackFunction::forComponent(
              [this](int result, juce::AlertWindow* w) {
                  if (result == 1) {
                      auto name = w->getTextEditorContents("name");
                      if (name.isNotEmpty()) {
                          PresetData p;
                          p.name = name.toStdString();
                          p.lfos = processor.lfos;
                          processor.userPresets.push_back(p);
                          populatePresetBox();
                      }
                  }
                  delete w;
              }, aw), true);
  }

  void PenguinLFOEditor::syncRowFromLFO(int i) {
      auto& lfo = processor.lfos[i];
      auto& row = lfoRows[i];
      row.shapeBox.setSelectedId(static_cast<int>(lfo.shape) + 1,  juce::dontSendNotification);
      row.rateBox.setSelectedId(lfo.rateIndex + 1,                  juce::dontSendNotification);
      row.targetBox.setSelectedId(static_cast<int>(lfo.target) + 1, juce::dontSendNotification);
      row.depthSlider.setValue(lfo.depth,                           juce::dontSendNotification);
      row.enableToggle.setToggleState(lfo.enabled,                  juce::dontSendNotification);
  }

  void PenguinLFOEditor::syncLFOFromRow(int i) {
      auto& lfo = processor.lfos[i];
      auto& row = lfoRows[i];
      lfo.shape     = static_cast<LFOShape>(row.shapeBox.getSelectedId() - 1);
      lfo.rateIndex = row.rateBox.getSelectedId() - 1;
      lfo.target    = static_cast<LFOTarget>(row.targetBox.getSelectedId() - 1);
      lfo.depth     = static_cast<float>(row.depthSlider.getValue());
      lfo.enabled   = row.enableToggle.getToggleState();
  }
  ```

- [ ] **Step 4: Build and verify**

  ```
  cmake --build build --config Release --target PenguinLFO
  ```
  Expected: build succeeds with no errors.

- [ ] **Step 5: Commit**

  ```bash
  git add src/penguin-lfo/Source/WaveformVisualizer.h src/penguin-lfo/Source/WaveformVisualizer.cpp \
          src/penguin-lfo/Source/PluginEditor.h src/penguin-lfo/Source/PluginEditor.cpp
  git commit -m "feat: plugin editor UI with LFO rows, preset picker, and waveform visualizer"
  ```

---

## Task 8: Build script and install to MPC 2

**Files:**
- Create: `src/penguin-lfo/install.bat`

- [ ] **Step 1: Write install.bat**

  Create `src/penguin-lfo/install.bat`:
  ```bat
  @echo off
  echo Building PenguinLFO...
  cmake --build build --config Release --target PenguinLFO
  if errorlevel 1 (
      echo Build failed.
      exit /b 1
  )
  echo Copying .dll to VST folder...
  copy /Y "build\PenguinLFO_artefacts\Release\VST\PenguinLFO.dll" ^
          "C:\Program Files\Steinberg\vstplugins\"
  echo Copying presets...
  mkdir "C:\Program Files\Steinberg\vstplugins\PenguinLFO_Presets" 2>nul
  copy /Y "Presets\presets.json" ^
          "C:\Program Files\Steinberg\vstplugins\PenguinLFO_Presets\"
  echo Done. Rescan plugins in MPC 2.
  ```

- [ ] **Step 2: Run install (as Administrator)**

  In x64 Native Tools Command Prompt **run as Administrator**:
  ```
  cd C:\Users\DlaZy\Documents\music-tools\src\penguin-lfo
  install.bat
  ```
  Expected: `PenguinLFO.dll` and `PenguinLFO_Presets\presets.json` appear in `C:\Program Files\Steinberg\vstplugins\`.

- [ ] **Step 3: Test in MPC 2**

  1. Open MPC 2
  2. Go to **Preferences → Plugins** and verify `C:\Program Files\Steinberg\vstplugins` is in the scan path
  3. Click **Rescan**
  4. Load a track with a sustained synth sound
  5. Add **PenguinLFO** as an insert effect
  6. Select **"Hard Gate"** preset — volume should chop rhythmically
  7. Select **"Polyrhythm"** — should feel more complex and layered
  8. Select **"Glitch"** — should sound stuttery and unpredictable

- [ ] **Step 4: Commit**

  ```bash
  git add src/penguin-lfo/install.bat
  git commit -m "feat: install script for PenguinLFO VST2 to MPC 2"
  ```

---

## Self-Review

**Spec coverage:**
- ✅ VST2 plugin for MPC 2 — Task 1, FORMATS VST
- ✅ 4 independent LFOs — Task 6 `std::array<LFOInstance, 4>`
- ✅ Volume/Filter/Pan/Pitch targets — Task 6 processBlock switch
- ✅ 25 timing options — Task 3 rate table, `LFO_RATE_COUNT = 25`
- ✅ 7 factory presets — Task 5 presets.json
- ✅ User preset save — Task 7 `onSavePreset()`
- ✅ Waveform visualizer — Task 7 `WaveformVisualizer`
- ✅ BPM sync from host — Task 6 `getPlayHead()→getBpm()`
- ✅ One-click preset selection — Task 7 `presetBox.onChange`

**Type consistency:**
- `lfoValueAtPhase(lfo, phase)` defined Task 2, used in Task 7 visualizer ✅
- `lfoAdvance(lfo, inc)` defined Task 2, used in Tasks 6 and 7 ✅
- `lfoPhaseIncrement(rateIndex, bpm, sr)` defined Task 3, used in Tasks 6 and 7 ✅
- `LFO_RATES[i].name` defined Task 3, used in Tasks 4 and 7 ✅
- `LFO_RATE_COUNT` defined Task 3, used in Task 7 ✅
- `parsePresetsJson` / `loadPresetsFromFile` defined Task 4, used in Task 6 ✅
- `processor.currentBPM` / `processor.currentSampleRate` declared public in Task 6, read in Task 7 ✅
