#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLUGIN="${1:-lfo}"

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
