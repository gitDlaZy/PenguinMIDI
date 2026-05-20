#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$SCRIPT_DIR/src/penguin-lfo"
BUILD_DIR="$PLUGIN_DIR/build"

CMAKE_EXE="/mnt/c/Program Files/CMake/bin/cmake.exe"
MSBUILD_EXE="/mnt/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe"

VST2_OUT="$BUILD_DIR/PenguinLFO_artefacts/Release/VST/PenguinLFO.dll"
VST3_OUT="$BUILD_DIR/PenguinLFO_artefacts/Release/VST3/PenguinLFO.vst3"
VST2_DEST="/mnt/c/Users/DlaZy/Documents/VST Plugins/PenguinLFO.dll"
VST3_DEST="/mnt/c/Users/DlaZy/Documents/VST3 Plugins/PenguinLFO.vst3"

echo "==> Configuring (cmake)..."
WIN_PLUGIN_DIR="$(wslpath -w "$PLUGIN_DIR")"
WIN_BUILD_DIR="$(wslpath -w "$BUILD_DIR")"
"$CMAKE_EXE" -S "$WIN_PLUGIN_DIR" -B "$WIN_BUILD_DIR" -G "Visual Studio 17 2022" -A x64

echo "==> Building (MSBuild Release)..."
WIN_SLN="$(wslpath -w "$BUILD_DIR/PenguinLFO.sln")"
"$MSBUILD_EXE" "$WIN_SLN" -p:Configuration=Release -p:Platform=x64 -m -nologo -v:minimal

echo "==> Copying VST2..."
cp "$VST2_OUT" "$VST2_DEST"
echo "    -> $VST2_DEST"

echo "==> Copying VST3..."
cp -r "$VST3_OUT" "$VST3_DEST"
echo "    -> $VST3_DEST"

echo ""
echo "Done. Rescan plugins in MPC 2."
