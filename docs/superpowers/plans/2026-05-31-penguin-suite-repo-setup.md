# Penguin Suite Repo Setup Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Restructure the repo from the original `music-tools` Python layout to the Penguin Suite monorepo layout, then rename the root folder.

**Architecture:** All changes are filesystem and git operations — no code is written. The active plugin (`penguin-lfo`) is moved via `git mv` to preserve history. Placeholder packages are created with `.gitkeep` files. The folder rename is the final step, after which Claude Code must be closed and reopened.

**Tech Stack:** git, bash, WSL2

---

## Files Modified / Created / Deleted

| Action | Path |
|--------|------|
| Delete | `Dockerfile` |
| Delete | `docker-compose.yml` |
| Delete | `requirements.txt` |
| Delete | `src/__main__.py` |
| git mv | `src/penguin-lfo/` → `packages/penguin-lfo/` |
| Modify | `build-plugin.sh` — update `PLUGIN_DIR` path |
| Rewrite | `CLAUDE.md` |
| Create | `CONTEXT.md` |
| Create | `packages/penguin-midi/web-prototype/.gitkeep` |
| Create | `packages/penguin-fx/.gitkeep` |
| Create | `packages/penguin-voice/.gitkeep` |
| Create | `packages/penguin-serum/.gitkeep` |
| Create | `packages/penguin-dj/.gitkeep` |
| Create | `packages/penguin-split/.gitkeep` |
| Create | `packages/penguin-find/.gitkeep` |
| Create | `packages/penguin-synthesia/.gitkeep` |
| Create | `packages/penguin-chop/.gitkeep` |
| Create | `packages/penguin-master/.gitkeep` |
| Create | `packages/penguin-stereo/.gitkeep` |
| Create | `packages/penguin-kick/.gitkeep` |
| Create | `shared/dsp/.gitkeep` |
| Create | `shared/scale-utils/.gitkeep` |
| Create | `shared/branding/.gitkeep` |
| Modify | `docs/superpowers/specs/2026-05-31-penguin-suite-setup-design.md` — note rename is done by Claude |
| Rename | `/mnt/c/Users/DlaZy/Documents/music-tools` → `/mnt/c/Users/DlaZy/Documents/PenguinSuite` |

---

## Task 1: Delete root-level Python/Docker files

**Files:**
- Delete: `Dockerfile`
- Delete: `docker-compose.yml`
- Delete: `requirements.txt`
- Delete: `src/__main__.py`

- [ ] **Step 1: Remove the files from git and disk**

```bash
git rm Dockerfile docker-compose.yml requirements.txt src/__main__.py
```

Expected output: four `rm` lines, one per file.

- [ ] **Step 2: Commit**

```bash
git commit -m "chore: remove Python/Docker root files — not part of Penguin Suite"
```

---

## Task 2: Move penguin-lfo into packages/

**Files:**
- git mv: `src/penguin-lfo/` → `packages/penguin-lfo/`

- [ ] **Step 1: Create the packages/ directory and move the plugin**

```bash
mkdir -p packages
git mv src/penguin-lfo packages/penguin-lfo
```

- [ ] **Step 2: Remove the now-empty src/ directory**

```bash
rmdir src
```

- [ ] **Step 3: Verify the move**

```bash
ls packages/penguin-lfo/Source/
```

Expected: `CustomWaveformEditor.cpp  CustomWaveformEditor.h  LFOEngine.cpp  LFOEngine.h  PluginEditor.cpp  PluginEditor.h  PluginProcessor.cpp  PluginProcessor.h  PresetManager.cpp  PresetManager.h  WaveformVisualizer.cpp  WaveformVisualizer.h`

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "refactor: move penguin-lfo from src/ to packages/"
```

---

## Task 3: Create placeholder packages

**Files:**
- Create: `.gitkeep` in each of the 11 new package folders

- [ ] **Step 1: Create all package placeholders in one command**

```bash
mkdir -p \
  packages/penguin-midi/web-prototype \
  packages/penguin-fx \
  packages/penguin-voice \
  packages/penguin-serum \
  packages/penguin-dj \
  packages/penguin-split \
  packages/penguin-find \
  packages/penguin-synthesia \
  packages/penguin-chop \
  packages/penguin-master \
  packages/penguin-stereo \
  packages/penguin-kick \
  shared/dsp \
  shared/scale-utils \
  shared/branding

touch \
  packages/penguin-midi/web-prototype/.gitkeep \
  packages/penguin-fx/.gitkeep \
  packages/penguin-voice/.gitkeep \
  packages/penguin-serum/.gitkeep \
  packages/penguin-dj/.gitkeep \
  packages/penguin-split/.gitkeep \
  packages/penguin-find/.gitkeep \
  packages/penguin-synthesia/.gitkeep \
  packages/penguin-chop/.gitkeep \
  packages/penguin-master/.gitkeep \
  packages/penguin-stereo/.gitkeep \
  packages/penguin-kick/.gitkeep \
  shared/dsp/.gitkeep \
  shared/scale-utils/.gitkeep \
  shared/branding/.gitkeep
```

- [ ] **Step 2: Verify structure**

```bash
find packages shared -name ".gitkeep" | sort
```

Expected: 15 lines — one `.gitkeep` per package/subfolder.

- [ ] **Step 3: Commit**

```bash
git add packages/ shared/
git commit -m "chore: scaffold packages/ and shared/ structure for Penguin Suite"
```

---

## Task 4: Update build-plugin.sh

**Files:**
- Modify: `build-plugin.sh` line 5 — `src/penguin-lfo` → `packages/penguin-lfo`

- [ ] **Step 1: Update the PLUGIN_DIR path**

In `build-plugin.sh`, change line 5 from:
```bash
PLUGIN_DIR="$SCRIPT_DIR/src/penguin-lfo"
```
to:
```bash
PLUGIN_DIR="$SCRIPT_DIR/packages/penguin-lfo"
```

- [ ] **Step 2: Verify the change**

```bash
grep "PLUGIN_DIR" build-plugin.sh
```

Expected: `PLUGIN_DIR="$SCRIPT_DIR/packages/penguin-lfo"`

- [ ] **Step 3: Commit**

```bash
git add build-plugin.sh
git commit -m "fix: update build-plugin.sh path to packages/penguin-lfo"
```

---

## Task 5: Rewrite CLAUDE.md

**Files:**
- Rewrite: `CLAUDE.md`

- [ ] **Step 1: Replace CLAUDE.md content**

Write the following to `CLAUDE.md`:

```markdown
# Penguin Suite

## What This Is

A suite of 11 VST plugins + 2 desktop apps (13 tools total) for music production. Built with JUCE (C++), targeting VST2, VST3, and standalone. See CONTEXT.md for a quick overview. See MASTER CONTEXT.txt for full detail on every plugin.

## Stack

- JUCE (C++) — all VST plugins
- VST2 for MPC2 compatibility (personal use only — Steinberg licensing)
- Electron or Tauri — PenguinSerum and PenguinSynthesia desktop apps
- Ollama (local LLM, 4b/8b model) — all AI features, no cloud cost
- WSL2 — development environment
- CMake + MSBuild — build toolchain (Windows)

## Project Structure

```
packages/
  penguin-lfo/      ← ACTIVE
  penguin-midi/
    web-prototype/  ← working HTML prototype (complete)
  penguin-fx/
  penguin-voice/
  penguin-serum/
  penguin-dj/
  penguin-split/
  penguin-find/
  penguin-synthesia/
  penguin-chop/
  penguin-master/
  penguin-stereo/
  penguin-kick/
shared/
  dsp/              ← shared DSP utilities (scale logic, BPM sync, MIDI helpers)
  scale-utils/
  branding/
```

## Building

Run `./build-plugin.sh` from repo root to build the active plugin (currently PenguinLFO).

## DAW Context

- DAW: MPC2 (Akai) — VST2 required for MPC2 compatibility
- Synth: Serum
- OS: Windows 11 + WSL2
- GPU: Nvidia RTX 3060 (used by PenguinSplit via CUDA)

## Engineering Priorities

1. Simplicity first
2. One plugin per package
3. Each plugin gets its own Dockerfile when development starts — not upfront
4. All AI features via local Ollama — no cloud APIs
5. Techno presets required on every plugin (Drumcode / Afterlife / Berghain references)

## Starting a Session

Say: `Read CONTEXT.md then let's work on [plugin name]`
For full detail: `Read MASTER CONTEXT.txt`
```

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "docs: rewrite CLAUDE.md for Penguin Suite"
```

---

## Task 6: Create CONTEXT.md

**Files:**
- Create: `CONTEXT.md`

- [ ] **Step 1: Create CONTEXT.md**

Write the following to `CONTEXT.md`:

```markdown
# Penguin Suite — Session Context

Read this at the start of every Claude Code session. For full detail on any plugin, see MASTER CONTEXT.txt.

## What This Is

13 VST plugins + 1 desktop app for music production. Built with JUCE (C++), VST2 + VST3.
Primary DAW: MPC2 (Akai). Primary producer: techno. Windows PC + WSL2 + Nvidia RTX 3060.
All AI features run locally via Ollama — no cloud cost.

## Tech Stack

- JUCE (C++) — all VST plugins (VST2 + VST3 + standalone from one codebase)
- VST2 included for MPC2 compatibility (personal use only)
- Electron or Tauri — PenguinSerum and PenguinSynthesia
- Ollama (local LLM, 4b or 8b — check with `ollama list`)
- CMake + MSBuild (Windows), WSL2 for development

## The Plugins

| # | Name | Description | Status |
|---|------|-------------|--------|
| 1 | PenguinLFO | BPM-synced modulation — volume, filter, pan, stutter | **Active** |
| 2 | PenguinMIDI | Step sequencer, scale-aware, hum-to-MIDI, drum machine | Web prototype done |
| 3 | PenguinFX | Modular effects chain — 30+ effect types, drag/drop | Not started |
| 4 | PenguinVoice | Vocal transformation, vocoder, talkbox, AI pitch correct | Not started |
| 5 | PenguinSerum | AI text-to-Serum-patch (desktop app, local LLM) | Not started |
| 6 | PenguinDJ | Fully automatic DJ VST — harmonic mixing, auto-transitions | Not started |
| 7 | PenguinSplit | AI stem splitter — Demucs + CUDA, bundled in DLL | Not started |
| 8 | PenguinFind | Sample search VST — Freesound + local library | Not started |
| 9 | PenguinSynthesia | AI piano teaching desktop app, tier-locked song library | Not started |
| 10 | PenguinChop | Sample chopper and pad instrument, MPC2 export | Not started |
| 11 | PenguinMaster | AI mastering chain — genre-aware, one-click | Not started |
| 12 | PenguinStereo | Pro stereo processor — mid/side EQ, vectorscope | Not started |
| 13 | PenguinKick | Kick drum designer — synth + sample hybrid, techno-focused | Not started |

## Build Order

1. PenguinLFO ← **active now**
2. PenguinMIDI (web prototype complete, VST next)
3. PenguinSplit (Demucs + CUDA bundling)
4. PenguinFX (most complex DSP)
5. PenguinVoice
6–13. Remaining plugins after core is solid

## Shared Suite Settings

All plugins read from a shared local config file:
- Key (C–B), Scale, BPM, Genre preset (Boom Bap / House / Pop / D&B / Techno)

## Techno Focus

Every plugin includes Techno presets. BPM range 130–145. References: Drumcode, Afterlife, Berghain.
```

- [ ] **Step 2: Commit**

```bash
git add CONTEXT.md
git commit -m "docs: add CONTEXT.md — quick session loader for Penguin Suite"
```

---

## Task 7: Update design spec — rename note

**Files:**
- Modify: `docs/superpowers/specs/2026-05-31-penguin-suite-setup-design.md`

- [ ] **Step 1: Update the rename section**

In the spec file, find the "Folder Rename" section and change:

```markdown
**How:** User does this manually in Windows Explorer (or PowerShell) after all internal changes are committed.
```

to:

```markdown
**How:** Claude Code runs `mv /mnt/c/Users/DlaZy/Documents/music-tools /mnt/c/Users/DlaZy/Documents/PenguinSuite` as the final step after all internal changes are committed. User then closes Claude Code and reopens from the new path.
```

- [ ] **Step 2: Commit**

```bash
git add docs/superpowers/specs/2026-05-31-penguin-suite-setup-design.md
git commit -m "docs: update rename step — Claude runs mv, not manual"
```

---

## Task 8: Rename the root folder (FINAL STEP)

**This is the last thing that runs. After this, Claude Code's working directory is broken. Close and reopen.**

- [ ] **Step 1: Run the rename**

```bash
mv /mnt/c/Users/DlaZy/Documents/music-tools /mnt/c/Users/DlaZy/Documents/PenguinSuite
```

No output expected. If you get "Permission denied", close any Windows Explorer windows showing that folder and retry.

- [ ] **Step 2: Notify the user**

Tell the user:

> Done. The folder has been renamed to `PenguinSuite`. Please close Claude Code now and reopen it from `C:\Users\DlaZy\Documents\PenguinSuite`. The next session will be fully set up.
