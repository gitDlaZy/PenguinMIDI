#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN_DIR="$SCRIPT_DIR/src/penguin-lfo"
BUILD_DIR="$PLUGIN_DIR/build"

CMAKE_EXE="/mnt/c/Program Files/CMake/bin/cmake.exe"
MSBUILD_EXE="/mnt/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/MSBuild/Current/Bin/MSBuild.exe"

VST2_OUT="$BUILD_DIR/PenguinLFO_artefacts/Release/VST/PenguinLFO.dll"
VST3_OUT="$BUILD_DIR/PenguinLFO_artefacts/Release/VST3/PenguinLFO.vst3"
VST2_DEST="/mnt/c/Program Files/Steinberg/vstplugins/PenguinLFO.dll"
VST3_DEST="/mnt/c/Program Files/Common Files/VST3/PenguinLFO.vst3"

echo "==> Configuring (cmake)..."
WIN_PLUGIN_DIR="$(wslpath -w "$PLUGIN_DIR")"
WIN_BUILD_DIR="$(wslpath -w "$BUILD_DIR")"
"$CMAKE_EXE" -S "$WIN_PLUGIN_DIR" -B "$WIN_BUILD_DIR" -G "Visual Studio 17 2022" -A x64

echo "==> Building (MSBuild Release)..."
WIN_SLN="$(wslpath -w "$BUILD_DIR/PenguinLFO.sln")"
"$MSBUILD_EXE" "$WIN_SLN" /t:Rebuild -p:Configuration=Release -p:Platform=x64 -m -nologo -v:minimal

echo ""
echo "Build complete. Copy the plugin manually:"
echo ""
echo "  VST2: $(wslpath -w "$VST2_OUT")"
echo "   ->   C:\\Program Files\\Steinberg\\vstplugins\\PenguinLFO.dll"
echo ""
echo "  VST3: $(wslpath -w "$VST3_OUT")"
echo "   ->   C:\\Program Files\\Common Files\\VST3\\PenguinLFO.vst3"
echo ""
echo "Then rescan plugins in MPC 2."
