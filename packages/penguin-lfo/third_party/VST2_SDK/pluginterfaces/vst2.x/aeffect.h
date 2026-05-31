#pragma once
#ifndef __aeffect__
#define __aeffect__

#include <stdint.h>

#if defined(_WIN32) || defined(_WIN64)
  #define VSTCALLBACK __cdecl
#else
  #define VSTCALLBACK
#endif

typedef int32_t  VstInt32;
typedef int64_t  VstInt64;
typedef intptr_t VstIntPtr;

struct AEffect;

typedef VstIntPtr (VSTCALLBACK* audioMasterCallback)(AEffect*, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
typedef VstIntPtr (VSTCALLBACK* AEffectDispatcherProc)(AEffect*, VstInt32, VstInt32, VstIntPtr, void*, float);
typedef void      (VSTCALLBACK* AEffectProcessProc)(AEffect*, float** inputs, float** outputs, VstInt32 frames);
typedef void      (VSTCALLBACK* AEffectProcessDoubleProc)(AEffect*, double** inputs, double** outputs, VstInt32 frames);
typedef void      (VSTCALLBACK* AEffectSetParameterProc)(AEffect*, VstInt32 index, float value);
typedef float     (VSTCALLBACK* AEffectGetParameterProc)(AEffect*, VstInt32 index);

enum VstAEffectFlags {
    effFlagsHasEditor        = 1 << 0,
    effFlagsCanReplacing     = 1 << 4,
    effFlagsProgramChunks    = 1 << 5,
    effFlagsIsSynth          = 1 << 8,
    effFlagsNoSoundInStop    = 1 << 9,
    effFlagsCanDoubleReplacing = 1 << 12
};

enum AEffectOpcodes {
    effOpen = 0,
    effClose,
    effSetProgram,
    effGetProgram,
    effSetProgramName,
    effGetProgramName,
    effGetParamLabel,
    effGetParamDisplay,
    effGetParamName,
    effGetVu,
    effSetSampleRate,
    effSetBlockSize,
    effMainsChanged,
    effEditGetRect,
    effEditOpen,
    effEditClose,
    effEditDraw,
    effEditMouse,
    effEditKey,
    effEditIdle,
    effEditTop,
    effEditSleep,
    effIdentify,
    effGetChunk,
    effSetChunk,
    effNumOpcodes
};

enum AudioMasterOpcodes {
    audioMasterAutomate = 0,
    audioMasterVersion,
    audioMasterCurrentId,
    audioMasterIdle,
    audioMasterPinConnected
};

struct AEffect {
    VstInt32 magic;                         // 0x56737450 ('VstP')
    AEffectDispatcherProc    dispatcher;
    AEffectProcessProc       process;
    AEffectSetParameterProc  setParameter;
    AEffectGetParameterProc  getParameter;
    VstInt32 numPrograms;
    VstInt32 numParams;
    VstInt32 numInputs;
    VstInt32 numOutputs;
    VstInt32 flags;
    VstIntPtr resvd1;
    VstIntPtr resvd2;
    VstInt32 initialDelay;
    VstInt32 realQualities;
    VstInt32 offQualities;
    float    ioRatio;
    void*    object;
    void*    user;
    VstInt32 uniqueID;
    VstInt32 version;
    AEffectProcessProc       processReplacing;
    AEffectProcessDoubleProc processDoubleReplacing;
    char     future[56];
};

#define kEffectMagic 0x56737450
#define kVstVersion  2400

#endif // __aeffect__
