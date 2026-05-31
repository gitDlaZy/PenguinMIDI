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
