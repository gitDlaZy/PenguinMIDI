# Penguin Suite — Session Context

Read this at the start of every Claude Code session. For full detail on any plugin, see MASTER CONTEXT.txt.

## What This Is

11 VST plugins + 2 desktop apps (13 tools total) for music production. Built with JUCE (C++), VST2 + VST3.
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
