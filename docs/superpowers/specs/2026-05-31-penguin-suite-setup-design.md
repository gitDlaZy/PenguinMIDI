# Penguin Suite — Repo Setup Design
**Date:** 2026-05-31
**Scope:** Rename root folder, restructure internal layout to match MASTER CONTEXT spec

---

## Goal

Establish the Penguin Suite monorepo structure from day one so every future plugin lands in the right place and the repo matches the MASTER CONTEXT spec.

---

## Folder Rename

The Windows folder `C:\Users\DlaZy\Documents\music-tools` is renamed to `C:\Users\DlaZy\Documents\PenguinSuite`.

**How:** User does this manually in Windows Explorer (or PowerShell) after all internal changes are committed. Claude Code must be closed before the rename and reopened from the new path.

There is no git remote, so no remote URLs need updating.

---

## Internal Restructure

### Files Deleted (root level)
- `Dockerfile` — Python/Docker leftover, not part of the suite
- `docker-compose.yml` — same
- `requirements.txt` — same
- `src/__main__.py` — Python entry point, not needed

### Files Moved
- `src/penguin-lfo/` → `packages/penguin-lfo/`
- `src/` folder removed once empty

### Files Updated
- `build-plugin.sh` — path references updated from `src/penguin-lfo` to `packages/penguin-lfo`
- `CLAUDE.md` — rewritten to reflect Penguin Suite context and new structure

### Files Created
- `CONTEXT.md` — short summary of the suite for quick Claude Code session loading
- `packages/penguin-midi/web-prototype/.gitkeep` — placeholder for working web prototype
- `.gitkeep` in each new package folder and each shared subfolder

---

## Final Repo Layout

```
PenguinSuite/
  CLAUDE.md                    ← updated
  CONTEXT.md                   ← new
  MASTER CONTEXT.txt           ← unchanged
  build-plugin.sh              ← paths updated
  docs/                        ← unchanged
  packages/
    penguin-lfo/               ← moved from src/penguin-lfo/ (full existing code)
    penguin-midi/
      web-prototype/           ← placeholder for working HTML prototype
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
    dsp/
    scale-utils/
    branding/
```

All new/empty packages contain a `.gitkeep`. Dockerfiles are created per-package only when development on that package starts — not upfront.

---

## CONTEXT.md Content

A short file at repo root summarising:
- Suite name and purpose
- Tech stack (JUCE, VST2/VST3, Electron/Tauri, Ollama)
- All 12 plugin names with one-line descriptions
- Build order
- Active plugin and its current status
- Pointer to MASTER CONTEXT.txt for full detail

Updated at the start of each new plugin's development.

---

## What Stays the Same

- All `packages/penguin-lfo/` source files — no code changes, only moved
- `docs/` folder and its contents
- `MASTER CONTEXT.txt`
- Git history (move via git mv to preserve history)

---

## After the Rename

1. Close Claude Code
2. In Windows Explorer: rename `music-tools` → `PenguinSuite`
3. Reopen Claude Code from `C:\Users\DlaZy\Documents\PenguinSuite`
4. Update the Claude Code project path if prompted
