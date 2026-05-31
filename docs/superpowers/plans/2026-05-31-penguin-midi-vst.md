# PenguinMIDI VST Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship PenguinMIDI as a VST2 + VST3 plugin for Windows — the existing web prototype UI runs inside a JUCE 8 WebView2 component, outputting MIDI to the DAW track.

**Architecture:** The plugin is a JUCE 8 Instrument plugin (IS_SYNTH=true, NEEDS_MIDI_OUTPUT=true). The plugin editor contains a `juce::WebBrowserComponent` (WebView2 backend) that loads a lightly modified copy of `index.html` via a ResourceProvider. Three native function bridge calls replace Web Audio playback, localStorage, and manual BPM with DAW-backed equivalents.

**Tech Stack:** JUCE 8.0.13 (already in repo at `packages/penguin-lfo/JUCE`), WebView2 (bundled with Windows 11/Edge), VST2 SDK (at `packages/penguin-lfo/third_party/VST2_SDK`), C++17, CMake + MSBuild.

---

## File Map

| File | Action | Responsibility |
|---|---|---|
| `packages/penguin-midi/CMakeLists.txt` | Create | Build config — WebView2-enabled instrument plugin |
| `packages/penguin-midi/Source/PluginProcessor.h` | Create | MIDI collector, BPM read, file I/O API |
| `packages/penguin-midi/Source/PluginProcessor.cpp` | Create | processBlock, MidiMessageCollector, AppData storage |
| `packages/penguin-midi/Source/PluginEditor.h` | Create | WebBrowserComponent declaration |
| `packages/penguin-midi/Source/PluginEditor.cpp` | Create | WebView2 setup, ResourceProvider, 4 bridge handlers |
| `packages/penguin-midi/web/index.html` | Create | Copy of prototype with 5 targeted modifications |
| `build-plugin.sh` | Modify | Add `--midi` flag to build PenguinMIDI instead of LFO |

---

## Task 1: Directory scaffold + CMakeLists.txt

**Files:**
- Create: `packages/penguin-midi/CMakeLists.txt`
- Create: `packages/penguin-midi/Source/PluginProcessor.h` (stub)
- Create: `packages/penguin-midi/Source/PluginProcessor.cpp` (stub)
- Create: `packages/penguin-midi/Source/PluginEditor.h` (stub)
- Create: `packages/penguin-midi/Source/PluginEditor.cpp` (stub)
- Create: `packages/penguin-midi/web/` (directory — index.html added in Task 4)

- [ ] **Step 1: Create directory structure**

```bash
mkdir -p packages/penguin-midi/Source
mkdir -p packages/penguin-midi/web
```

- [ ] **Step 2: Create CMakeLists.txt**

Create `packages/penguin-midi/CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.22)
project(PenguinMIDI VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Reuse JUCE and VST2 SDK from penguin-lfo — no duplication needed
add_subdirectory(../penguin-lfo/JUCE juce_build)
juce_set_vst2_sdk_path(${CMAKE_CURRENT_SOURCE_DIR}/../penguin-lfo/third_party/VST2_SDK)

juce_add_plugin(PenguinMIDI
    COMPANY_NAME            "DlaZy"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Dlzy
    PLUGIN_CODE             Pmid
    FORMATS                 VST VST3
    PRODUCT_NAME            "PenguinMIDI"
    VST2_CATEGORY           kPlugCategSynth
    NEEDS_WEBVIEW2          TRUE
)

juce_generate_juce_header(PenguinMIDI)

juce_add_binary_data(PenguinMIDIData SOURCES
    web/index.html
)

target_sources(PenguinMIDI PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(PenguinMIDI PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(PenguinMIDI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../penguin-lfo/third_party
)

target_link_libraries(PenguinMIDI
    PRIVATE
        PenguinMIDIData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

- [ ] **Step 3: Create stub source files so CMake can configure**

Create `packages/penguin-midi/Source/PluginProcessor.h`:
```cpp
#pragma once
#include <JuceHeader.h>

class PenguinMIDIProcessor : public juce::AudioProcessor {
public:
    PenguinMIDIProcessor();
    ~PenguinMIDIProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PenguinMIDI"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int    getNumPrograms()  override { return 1; }
    int    getCurrentProgram() override { return 0; }
    void   setCurrentProgram(int) override {}
    const  juce::String getProgramName(int) override { return {}; }
    void   changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    // Called from bridge (message thread) — thread-safe via MidiMessageCollector
    void scheduleMidiNote(int channel, int note, int velocity, int durationMs);

    // Called from bridge — returns current host BPM (or 120 if unavailable)
    double getCurrentBpm() const { return lastBpm.load(); }

    // Called from bridge — file-backed pattern storage
    void savePatterns(const juce::String& jsonString);
    juce::String loadPatternsJson();

private:
    juce::MidiMessageCollector midiCollector;
    std::atomic<double> lastBpm { 120.0 };

    juce::File getPatternsFile() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinMIDIProcessor)
};
```

Create `packages/penguin-midi/Source/PluginProcessor.cpp`:
```cpp
#include "PluginProcessor.h"
#include "PluginEditor.h"

PenguinMIDIProcessor::PenguinMIDIProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

void PenguinMIDIProcessor::prepareToPlay(double sampleRate, int)
{
    midiCollector.reset(sampleRate);
}

void PenguinMIDIProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    audio.clear();
    midiCollector.removeNextBlockOfMessages(midi, audio.getNumSamples());

    // Update cached BPM from host transport
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                lastBpm.store(*bpm);
}

juce::AudioProcessorEditor* PenguinMIDIProcessor::createEditor()
{
    return new PenguinMIDIEditor(*this);
}

void PenguinMIDIProcessor::scheduleMidiNote(int channel, int note, int velocity, int durationMs)
{
    auto t = juce::Time::getMillisecondCounterHiRes() * 0.001;
    midiCollector.addMessageToQueue(
        juce::MidiMessage::noteOn(channel, note, (juce::uint8)velocity).withTimeStamp(t));

    juce::Timer::callAfterDelay(durationMs, [this, channel, note, t] {
        midiCollector.addMessageToQueue(
            juce::MidiMessage::noteOff(channel, note).withTimeStamp(
                juce::Time::getMillisecondCounterHiRes() * 0.001));
    });
}

juce::File PenguinMIDIProcessor::getPatternsFile() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("PenguinSuite");
    dir.createDirectory();
    return dir.getChildFile("midi-patterns.json");
}

void PenguinMIDIProcessor::savePatterns(const juce::String& jsonString)
{
    getPatternsFile().replaceWithText(jsonString);
}

juce::String PenguinMIDIProcessor::loadPatternsJson()
{
    auto f = getPatternsFile();
    if (f.existsAsFile())
        return f.loadFileAsString();
    return "[]";
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PenguinMIDIProcessor();
}
```

Create `packages/penguin-midi/Source/PluginEditor.h`:
```cpp
#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PenguinMIDIEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit PenguinMIDIEditor(PenguinMIDIProcessor&);
    ~PenguinMIDIEditor() override;

    void resized() override;
    void paint(juce::Graphics&) override {}

private:
    void timerCallback() override;

    PenguinMIDIProcessor& processor;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    double lastPushedBpm = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinMIDIEditor)
};
```

Create `packages/penguin-midi/Source/PluginEditor.cpp`:
```cpp
#include "PluginEditor.h"

PenguinMIDIEditor::PenguinMIDIEditor(PenguinMIDIProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    auto resourceProvider = [](const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
    {
        if (url == "/" || url == "/index.html")
        {
            auto* data = reinterpret_cast<const std::byte*>(BinaryData::index_html);
            return juce::WebBrowserComponent::Resource {
                std::vector<std::byte>(data, data + BinaryData::index_htmlSize),
                "text/html"
            };
        }
        return std::nullopt;
    };

    auto options = juce::WebBrowserComponent::Options{}
        .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
        .withWinWebView2Options(
            juce::WebBrowserComponent::Options::WinWebView2{}
                .withUserDataFolder(juce::File::getSpecialLocation(juce::File::tempDirectory)
                                        .getChildFile("PenguinMIDI_WebView"))
                .withStatusBarDisabled()
                .withBuiltInErrorPageDisabled())
        .withNativeIntegrationEnabled()
        .withResourceProvider(resourceProvider)
        .withNativeFunction("midiNoteOn",
            [this](const juce::Array<juce::var>& args, auto complete)
            {
                if (args.size() >= 4)
                    processor.scheduleMidiNote(
                        (int)args[0], (int)args[1], (int)args[2], (int)args[3]);
                complete(juce::var{});
            })
        .withNativeFunction("getBpm",
            [this](const juce::Array<juce::var>&, auto complete)
            {
                complete(juce::var(processor.getCurrentBpm()));
            })
        .withNativeFunction("savePatterns",
            [this](const juce::Array<juce::var>& args, auto complete)
            {
                if (!args.isEmpty())
                    processor.savePatterns(args[0].toString());
                complete(juce::var{});
            })
        .withNativeFunction("loadPatterns",
            [this](const juce::Array<juce::var>&, auto complete)
            {
                complete(juce::var(processor.loadPatternsJson()));
            });

    webView = std::make_unique<juce::WebBrowserComponent>(options);
    addAndMakeVisible(*webView);
    webView->goToURL(juce::WebBrowserComponent::getResourceProviderRoot());

    setResizable(true, false);
    setSize(1000, 780);

    startTimerHz(2); // poll BPM twice per second
}

PenguinMIDIEditor::~PenguinMIDIEditor()
{
    stopTimer();
}

void PenguinMIDIEditor::resized()
{
    webView->setBounds(getLocalBounds());
}

void PenguinMIDIEditor::timerCallback()
{
    auto bpm = processor.getCurrentBpm();
    if (std::abs(bpm - lastPushedBpm) > 0.5)
    {
        lastPushedBpm = bpm;
        juce::DynamicObject::Ptr obj = new juce::DynamicObject();
        obj->setProperty("bpm", bpm);
        webView->emitEventIfBrowserIsVisible("bpmChanged", juce::var(obj.get()));
    }
}
```

- [ ] **Step 4: Commit scaffold**

```bash
git add packages/penguin-midi/
git commit -m "feat: scaffold PenguinMIDI VST package structure"
```

---

## Task 2: Create the modified index.html for the VST

**Files:**
- Create: `packages/penguin-midi/web/index.html` (copy of prototype + 5 modifications)

The modifications are surgical — copy `packages/penguin-midi/web-prototype/index.html` to `packages/penguin-midi/web/index.html` then apply these 5 changes:

- [ ] **Step 1: Copy the prototype**

```bash
cp packages/penguin-midi/web-prototype/index.html packages/penguin-midi/web/index.html
```

- [ ] **Step 2: Add JUCE bridge helpers at the top of the `<script>` block**

Find the line `<script>` (the opening tag of the big script block, around line 971) and insert this immediately after it:

```javascript
// ─── JUCE BRIDGE ─────────────────────────────────────────────────────────────
// Inline promise handler — mirrors JUCE's juce/index.js without ES module syntax
const _jucePromises = new Map();
let _jucePromiseId = 0;

if (window.__JUCE__) {
  window.__JUCE__.backend.addEventListener('__juce__complete', ({ promiseId, result }) => {
    const resolve = _jucePromises.get(promiseId);
    if (resolve) { resolve(result); _jucePromises.delete(promiseId); }
  });
}

function _juceCall(name, ...args) {
  return new Promise(resolve => {
    const id = _jucePromiseId++;
    _jucePromises.set(id, resolve);
    window.__JUCE__.backend.emitEvent('__juce__invoke', { name, params: args, resultId: id });
  });
}

const jucePlayMidi     = (ch, note, vel, durMs) => _juceCall('midiNoteOn', ch, note, vel, durMs);
const juceGetBpm       = ()                     => _juceCall('getBpm');
const juceSavePatterns = (json)                 => _juceCall('savePatterns', json);
const juceLoadPatterns = ()                     => _juceCall('loadPatterns');
```

- [ ] **Step 3: Add Techno drum preset**

Find `const DRUM_PATTERNS = {` and add `techno` as a fifth entry (after `dnb`):

```javascript
  techno: {
    kick:  [1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0],
    snare: [0,0,0,0, 1,0,0,0, 0,0,0,0, 1,0,0,0],
    hat:   [1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1],
    open:  [0,0,0,0, 0,0,0,1, 0,0,0,0, 0,0,0,1],
    clap:  [0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0],
  },
```

Find `const STYLE_BPM = { boombap: 96, house: 128, pop: 118, dnb: 174 };` and add techno:
```javascript
const STYLE_BPM = { boombap: 96, house: 128, pop: 118, dnb: 174, techno: 138 };
```

Find the drum style pills HTML section and add the Techno pill after D&B:
```html
<button class="drum-pill" id="dpill-techno" data-style="techno">Techno</button>
```

- [ ] **Step 4: Replace playNote() with bridge call**

Find and replace the entire `function playNote(midi, time, duration) {` function (lines ~1733–1785 in the original, the multi-oscillator Web Audio piano synth). Replace with:

```javascript
function playNote(midi, time, duration) {
  if (window.__JUCE__) {
    jucePlayMidi(1, midi, 80, Math.round(duration * 1000));
    return;
  }
  // Web Audio fallback (standalone browser use only)
  const ctx = getAudioCtx();
  const freq = 440 * Math.pow(2, (midi - 69) / 12);
  const master = ctx.createGain();
  master.connect(ctx.destination);
  const osc = ctx.createOscillator();
  const g = ctx.createGain();
  osc.connect(g); g.connect(master);
  osc.frequency.value = freq;
  osc.type = 'sine';
  g.gain.value = 0.3;
  g.gain.exponentialRampToValueAtTime(0.0001, ctx.currentTime + duration + 0.5);
  osc.start(ctx.currentTime); osc.stop(ctx.currentTime + duration + 0.6);
}
```

- [ ] **Step 5: Replace playDrum() with bridge call**

Find and replace the entire `function playDrum(type, time, vol) {` function. Replace with:

```javascript
const DRUM_MIDI_NOTES = { kick: 36, snare: 38, hat: 42, open: 46, clap: 39 };

function playDrum(type, time, vol) {
  if (window.__JUCE__) {
    jucePlayMidi(10, DRUM_MIDI_NOTES[type], Math.round(vol * 100), 50);
    return;
  }
  // Web Audio fallback (standalone browser use only — original implementation)
  const ctx = getAudioCtx();
  const v = vol * drumState.volume;
  if (type === 'kick') {
    const osc = ctx.createOscillator(); const g = ctx.createGain();
    osc.connect(g); g.connect(ctx.destination);
    osc.frequency.setValueAtTime(160, time);
    osc.frequency.exponentialRampToValueAtTime(40, time + 0.12);
    g.gain.setValueAtTime(v * 1.4, time);
    g.gain.exponentialRampToValueAtTime(0.001, time + 0.35);
    osc.start(time); osc.stop(time + 0.4);
  } else if (type === 'snare' || type === 'hat' || type === 'open' || type === 'clap') {
    const buf = ctx.createBuffer(1, ctx.sampleRate * 0.15, ctx.sampleRate);
    const data = buf.getChannelData(0);
    for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
    const src = ctx.createBufferSource(); src.buffer = buf;
    const g = ctx.createGain(); const filt = ctx.createBiquadFilter();
    filt.type = 'highpass'; filt.frequency.value = type === 'hat' ? 7000 : 1500;
    src.connect(filt); filt.connect(g); g.connect(ctx.destination);
    g.gain.setValueAtTime(v * 0.6, time);
    g.gain.exponentialRampToValueAtTime(0.001, time + 0.12);
    src.start(time); src.stop(time + 0.15);
  }
}
```

- [ ] **Step 6: Replace localStorage pattern storage with bridge-backed storage**

Find and replace `function loadPatterns()`:
```javascript
function loadPatterns() {
  return _patternCache;
}
```

Find and replace `function savePatterns(patterns)`:
```javascript
function savePatterns(patterns) {
  _patternCache = patterns;
  if (window.__JUCE__)
    juceSavePatterns(JSON.stringify(patterns));
  else
    localStorage.setItem('penguinPatterns', JSON.stringify(patterns));
}
```

Add a `_patternCache` variable and async init near the top of the script block (just after the JUCE bridge section added in Step 2):
```javascript
let _patternCache = [];

async function _initBridge() {
  if (window.__JUCE__) {
    try {
      const json = await juceLoadPatterns();
      _patternCache = JSON.parse(json) || [];
    } catch(e) { _patternCache = []; }
    // Sync BPM from host
    try {
      const bpm = await juceGetBpm();
      document.getElementById('bpm').value = Math.round(bpm);
    } catch(e) {}
    // Listen for BPM changes while plugin is open
    window.__JUCE__.backend.addEventListener('bpmChanged', (data) => {
      document.getElementById('bpm').value = Math.round(data.bpm);
    });
  } else {
    try { _patternCache = JSON.parse(localStorage.getItem('penguinPatterns') || '[]'); }
    catch(e) { _patternCache = []; }
  }
}
```

Find the `// ─── INIT ─────────────────────────────────────────────────────────────────` section at the very end of the script and add `_initBridge();` as the first call:

```javascript
// ─── INIT ─────────────────────────────────────────────────────────────────
_initBridge();
buildKeyScalePanel();
renderDrums();
render();
```

- [ ] **Step 7: Commit the modified index.html**

```bash
git add packages/penguin-midi/web/index.html
git commit -m "feat: add PenguinMIDI VST web UI with JUCE bridge calls"
```

---

## Task 3: Build the plugin

**Files:**
- Modify: `build-plugin.sh`

- [ ] **Step 1: Add `--midi` flag to build-plugin.sh**

Open `build-plugin.sh` and replace the current hardcoded LFO content with a plugin selector:

```bash
#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN="${1:-lfo}"   # default to lfo; pass --midi to build PenguinMIDI

case "$PLUGIN" in
  --midi|midi)
    PLUGIN_NAME="PenguinMIDI"
    PLUGIN_DIR="$SCRIPT_DIR/packages/penguin-midi"
    ;;
  *)
    PLUGIN_NAME="PenguinLFO"
    PLUGIN_DIR="$SCRIPT_DIR/packages/penguin-lfo"
    ;;
esac

BUILD_DIR="$PLUGIN_DIR/build"

CMAKE_EXE="/mnt/c/Program Files/CMake/bin/cmake.exe"
MSBUILD_EXE="/mnt/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe"

VST2_OUT="$BUILD_DIR/${PLUGIN_NAME}_artefacts/Release/VST/${PLUGIN_NAME}.dll"
VST3_OUT="$BUILD_DIR/${PLUGIN_NAME}_artefacts/Release/VST3/${PLUGIN_NAME}.vst3"

echo "==> Building $PLUGIN_NAME..."

WIN_PLUGIN_DIR="$(wslpath -w "$PLUGIN_DIR")"
WIN_BUILD_DIR="$(wslpath -w "$BUILD_DIR")"
"$CMAKE_EXE" -S "$WIN_PLUGIN_DIR" -B "$WIN_BUILD_DIR" -G "Visual Studio 17 2022" -A x64

WIN_SLN="$(wslpath -w "$BUILD_DIR/${PLUGIN_NAME}.sln")"
"$MSBUILD_EXE" "$WIN_SLN" /t:Rebuild -p:Configuration=Release -p:Platform=x64 -m -nologo -v:minimal

echo ""
echo "Build complete. Copy the plugin manually:"
echo ""
echo "  VST2: $(wslpath -w "$VST2_OUT")"
echo "   ->   C:\\Program Files\\Steinberg\\vstplugins\\${PLUGIN_NAME}.dll"
echo ""
echo "  VST3: $(wslpath -w "$VST3_OUT")"
echo "   ->   C:\\Program Files\\Common Files\\VST3\\${PLUGIN_NAME}.vst3"
echo ""
echo "Then rescan plugins in MPC 2."
```

- [ ] **Step 2: Build PenguinMIDI**

```bash
./build-plugin.sh --midi
```

Expected: Build succeeds. Artefacts at:
- `packages/penguin-midi/build/PenguinMIDI_artefacts/Release/VST/PenguinMIDI.dll`
- `packages/penguin-midi/build/PenguinMIDI_artefacts/Release/VST3/PenguinMIDI.vst3`

If the build fails due to WebView2 not being found, install the WebView2 runtime from Microsoft or check that `NEEDS_WEBVIEW2 TRUE` is in CMakeLists.txt.

- [ ] **Step 3: Commit build script update**

```bash
git add build-plugin.sh
git commit -m "feat: update build-plugin.sh to support --midi flag for PenguinMIDI"
```

---

## Task 4: Install and smoke test

- [ ] **Step 1: Copy DLL to VST folder (run in Windows PowerShell or File Explorer)**

```
Copy packages\penguin-midi\build\PenguinMIDI_artefacts\Release\VST\PenguinMIDI.dll
  → C:\Program Files\Steinberg\vstplugins\PenguinMIDI.dll

Copy packages\penguin-midi\build\PenguinMIDI_artefacts\Release\VST3\PenguinMIDI.vst3
  → C:\Program Files\Common Files\VST3\PenguinMIDI.vst3
```

- [ ] **Step 2: Rescan plugins in MPC2**

Open MPC2 → Preferences → Plugins → Rescan. PenguinMIDI should appear as an Instrument plugin.

- [ ] **Step 3: Load plugin and verify UI**

Load PenguinMIDI on an instrument track with Serum (or any synth) also on the track. Open the plugin window.

Expected: The dark UI from the web prototype appears pixel-for-pixel. The BPM field shows the current DAW BPM.

- [ ] **Step 4: Test MIDI output**

Click Generate All, then Play. Serum should produce sound.

Expected: Notes play through Serum at the correct BPM. The playhead dot moves across the grid.

- [ ] **Step 5: Test drum machine**

Enable the drum machine, click Play.

Expected: Drum notes on MIDI channel 10 trigger drum sounds in Serum's drum patch (if loaded), or are visible in the MIDI monitor.

- [ ] **Step 6: Test pattern save/load**

Save a pattern via the 💾 Patterns button. Close and reopen the plugin.

Expected: The saved pattern appears in the list and loads correctly. Pattern file exists at `%APPDATA%\PenguinSuite\midi-patterns.json`.

- [ ] **Step 7: Test Export MIDI**

Click ↓ Export MIDI.

Expected: A `.mid` file downloads. Drag it into MPC2 and verify notes are correct.

- [ ] **Step 8: Commit final state**

```bash
git add -A
git commit -m "feat: PenguinMIDI VST — full implementation complete"
```

---

## Known Issues / Later

- **Hum / Listen** — deferred to v1.1. Mic access via `getUserMedia()` inside WebView2 inside a DAW host is unreliable. Will require explicit Windows `mediaCapture` permissions granted to the host process.
- **BPM field in standalone mode** — when running in JUCE standalone (outside DAW), `getPlayHead()` returns nullptr; `getCurrentBpm()` falls back to 120.0. This is acceptable for v1.
- **Google Fonts** — the UI loads DM Mono and Playfair Display from fonts.googleapis.com. Falls back to system monospace/serif if internet is unavailable. Embed fonts in v1.1 if needed.
