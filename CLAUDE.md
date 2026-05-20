# Music Tools

## Goal

A collection of tools for music production workflows. Modular — each tool lives in its own folder under `src/`.

## Stack

- Python 3.12
- Docker + docker-compose for consistent environment
- WSL2 (Linux) development

## Local-First Rule

- No paid APIs unless explicitly chosen per tool
- Prefer lightweight dependencies
- GPU optional (RTX 3060 available if needed)

## Engineering Priorities

1. Simplicity first
2. One tool per module
3. Easy to run with `docker-compose up`

## Project Structure

```
src/
  <tool-name>/      ← one folder per tool
    main.py
    requirements.txt (if extra deps needed)
```

## DAW Context

- DAW: MPC 2
- Synth: Serum
- OS: Windows 11 + WSL2
