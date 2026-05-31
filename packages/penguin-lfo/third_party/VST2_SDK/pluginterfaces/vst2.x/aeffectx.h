#pragma once
#ifndef __aeffectx__
#define __aeffectx__

#include "aeffect.h"

typedef short VstInt16;

enum {
    kVstMaxNameLen       = 64,
    kVstMaxLabelLen      = 64,
    kVstMaxShortLabelLen = 8,
    kVstMaxCategLabelLen = 24,
    kVstMaxFileNameLen   = 100,
    kVstMaxProgNameLen   = 24
};

enum VstSmpteFrameRate {
    kVstSmpte24fps    = 0,
    kVstSmpte25fps    = 1,
    kVstSmpte2997fps  = 2,
    kVstSmpte30fps    = 3,
    kVstSmpte2997dfps = 4,
    kVstSmpte30dfps   = 5,
    kVstSmpteFilm16mm = 6,
    kVstSmpteFilm35mm = 7,
    kVstSmpte239fps   = 10,
    kVstSmpte249fps   = 11,
    kVstSmpte599fps   = 12,
    kVstSmpte60fps    = 13
};

enum AEffectXOpcodes {
    effProcessEvents = effNumOpcodes,
    effCanBeAutomated,
    effString2Parameter,
    effGetNumProgramCategories,
    effGetProgramNameIndexed,
    effCopyProgram,
    effConnectInput,
    effConnectOutput,
    effGetInputProperties,
    effGetOutputProperties,
    effGetPlugCategory,
    effGetCurrentPosition,
    effGetDestinationBuffer,
    effOfflineNotify,
    effOfflinePrepare,
    effOfflineRun,
    effProcessVarIo,
    effSetSpeakerArrangement,
    effSetBlockSizeAndSampleRate,
    effSetBypass,
    effGetEffectName,
    effGetErrorText,
    effGetVendorString,
    effGetProductString,
    effGetVendorVersion,
    effVendorSpecific,
    effCanDo,
    effGetTailSize,
    effIdle,
    effGetIcon,
    effSetViewPosition,
    effGetParameterProperties,
    effKeysRequired,
    effGetVstVersion,
    effEditKeyDown,
    effEditKeyUp,
    effSetEditKnobMode,
    effGetMidiProgramName,
    effGetCurrentMidiProgram,
    effGetMidiProgramCategory,
    effHasMidiProgramsChanged,
    effGetMidiKeyName,
    effBeginSetProgram,
    effEndSetProgram,
    effGetSpeakerArrangement,
    effShellGetNextPlugin,
    effStartProcess,
    effStopProcess,
    effSetTotalSampleToProcess,
    effSetPanLaw,
    effBeginLoadBank,
    effBeginLoadProgram,
    effSetProcessPrecision,
    effGetNumMidiInputChannels,
    effGetNumMidiOutputChannels
};

enum AudioMasterOpcodesX {
    audioMasterWantMidi = audioMasterPinConnected + 2,
    audioMasterGetTime,
    audioMasterProcessEvents,
    audioMasterSetTime,
    audioMasterTempoAt,
    audioMasterGetNumAutomatableParameters,
    audioMasterGetParameterQuantization,
    audioMasterIOChanged,
    audioMasterNeedIdle,
    audioMasterSizeWindow,
    audioMasterGetSampleRate,
    audioMasterGetBlockSize,
    audioMasterGetInputLatency,
    audioMasterGetOutputLatency,
    audioMasterGetPreviousPlug,
    audioMasterGetNextPlug,
    audioMasterWillReplaceOrAccumulate,
    audioMasterGetCurrentProcessLevel,
    audioMasterGetAutomationState,
    audioMasterOfflineStart,
    audioMasterOfflineRead,
    audioMasterOfflineWrite,
    audioMasterOfflineGetCurrentPass,
    audioMasterOfflineGetCurrentMetaPass,
    audioMasterSetOutputSampleRate,
    audioMasterGetOutputSpeakerArrangement,
    audioMasterGetVendorString,
    audioMasterGetProductString,
    audioMasterGetVendorVersion,
    audioMasterVendorSpecific,
    audioMasterSetIcon,
    audioMasterCanDo,
    audioMasterGetLanguage,
    audioMasterOpenWindow,
    audioMasterCloseWindow,
    audioMasterGetDirectory,
    audioMasterUpdateDisplay,
    audioMasterBeginEdit,
    audioMasterEndEdit,
    audioMasterOpenFileSelector,
    audioMasterCloseFileSelector,
    audioMasterEditFile,
    audioMasterGetChunkFile,
    audioMasterGetInputSpeakerArrangement
};

enum VstPlugCategory {
    kPlugCategUnknown = 0,
    kPlugCategEffect,
    kPlugCategSynth,
    kPlugCategAnalysis,
    kPlugCategMastering,
    kPlugCategSpacializer,
    kPlugCategRoomFx,
    kPlugSurroundFx,
    kPlugCategRestoration,
    kPlugCategOfflineProcess,
    kPlugCategShell,
    kPlugCategGenerator,
    kPlugCategMaxCount
};

enum VstProcessPrecision {
    kVstProcessPrecision32 = 0,
    kVstProcessPrecision64
};

enum VstEventTypes {
    kVstMidiType  = 1,
    kVstSysExType = 6
};

struct VstEvent {
    VstInt32 type;
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    char     data[16];
};

struct VstMidiEvent {
    VstInt32 type;
    VstInt32 byteSize;
    VstInt32 deltaFrames;
    VstInt32 flags;
    VstInt32 noteLength;
    VstInt32 noteOffset;
    char     midiData[4];
    char     detune;
    char     noteOffVelocity;
    char     reserved1;
    char     reserved2;
};

enum VstMidiEventFlags {
    kVstMidiEventIsRealtime = 1 << 0
};

struct VstMidiSysexEvent {
    VstInt32  type;
    VstInt32  byteSize;
    VstInt32  deltaFrames;
    VstInt32  flags;
    VstInt32  dumpBytes;
    VstIntPtr resvd1;
    char*     sysexDump;
    VstIntPtr resvd2;
};

struct VstEvents {
    VstInt32   numEvents;
    VstIntPtr  reserved;
    VstEvent*  events[2];
};

struct VstTimeInfo {
    double   samplePos;
    double   sampleRate;
    double   nanoSeconds;
    double   ppqPos;
    double   tempo;
    double   barStartPos;
    double   cycleStartPos;
    double   cycleEndPos;
    VstInt32 timeSigNumerator;
    VstInt32 timeSigDenominator;
    VstInt32 smpteOffset;
    VstInt32 smpteFrameRate;
    VstInt32 samplesToNextClock;
    VstInt32 flags;
};

enum VstTimeInfoFlags {
    kVstTransportChanged     = 1 << 0,
    kVstTransportPlaying     = 1 << 1,
    kVstTransportCycleActive = 1 << 2,
    kVstTransportRecording   = 1 << 3,
    kVstAutomationWriting    = 1 << 6,
    kVstAutomationReading    = 1 << 7,
    kVstNanosValid           = 1 << 8,
    kVstPpqPosValid          = 1 << 9,
    kVstTempoValid           = 1 << 10,
    kVstBarsValid            = 1 << 11,
    kVstCyclePosValid        = 1 << 12,
    kVstTimeSigValid         = 1 << 13,
    kVstSmpteValid           = 1 << 14,
    kVstClockValid           = 1 << 15
};

struct ERect {
    short top;
    short left;
    short bottom;
    short right;
};

struct VstPatchChunkInfo {
    VstInt32 version;
    VstInt32 pluginUniqueID;
    VstInt32 pluginVersion;
    VstInt32 numElements;
    char     reserved[48];
};

struct VstPinProperties {
    char     label[kVstMaxLabelLen];
    VstInt32 flags;
    VstInt32 arrangementType;
    char     shortLabel[kVstMaxShortLabelLen];
    char     future[48];
};

enum VstPinPropertiesFlags {
    kVstPinIsActive   = 1 << 0,
    kVstPinIsStereo   = 1 << 1,
    kVstPinUseSpeaker = 1 << 2
};

struct VstParameterProperties {
    float    stepFloat;
    float    smallStepFloat;
    float    largeStepFloat;
    char     label[kVstMaxLabelLen];
    VstInt32 flags;
    VstInt32 minInteger;
    VstInt32 maxInteger;
    VstInt32 stepInteger;
    VstInt32 largeStepInteger;
    char     shortLabel[kVstMaxShortLabelLen];
    VstInt16 displayIndex;
    VstInt16 category;
    VstInt16 numParametersInCategory;
    VstInt16 reserved;
    char     categoryLabel[kVstMaxCategLabelLen];
    char     future[16];
};

enum VstParameterFlags {
    kVstParameterIsSwitch                  = 1 << 0,
    kVstParameterUsesIntegerMinMax         = 1 << 1,
    kVstParameterUsesFloatStep             = 1 << 2,
    kVstParameterUsesIntStep               = 1 << 3,
    kVstParameterSupportsDisplayIndex      = 1 << 4,
    kVstParameterSupportsDisplayCategory   = 1 << 5,
    kVstParameterCanRamp                   = 1 << 6
};

struct MidiKeyName {
    VstInt32 thisProgramIndex;
    VstInt32 thisKeyNumber;
    char     keyName[kVstMaxNameLen];
    VstInt32 reserved;
    VstInt32 flags;
};

struct MidiProgramName {
    VstInt32 thisProgramIndex;
    char     name[kVstMaxNameLen];
    char     midiProgram;
    char     midiBankMsb;
    char     midiBankLsb;
    char     reserved;
    VstInt32 parentCategoryIndex;
    VstInt32 flags;
};

struct MidiProgramCategory {
    VstInt32 thisCategoryIndex;
    char     name[kVstMaxNameLen];
    VstInt32 parentCategoryIndex;
    VstInt32 flags;
};

struct VstSpeakerProperties {
    float    azimuth;
    float    elevation;
    float    radius;
    float    reserved;
    char     name[kVstMaxNameLen];
    VstInt32 type;
    char     future[28];
};

struct VstSpeakerArrangement {
    VstInt32             type;
    VstInt32             numChannels;
    VstSpeakerProperties speakers[8];
};

struct VstIndividualSpeakerInfo {
    VstSpeakerProperties speaker;
};

enum VstSpeakerType {
    kSpeakerUndefined = 0x7fffffff,
    kSpeakerM  = 0,
    kSpeakerL,
    kSpeakerR,
    kSpeakerC,
    kSpeakerLfe,
    kSpeakerLs,
    kSpeakerRs,
    kSpeakerLc,
    kSpeakerRc,
    kSpeakerS,
    kSpeakerSl,
    kSpeakerSr,
    kSpeakerTm,
    kSpeakerTfl,
    kSpeakerTfc,
    kSpeakerTfr,
    kSpeakerTrl,
    kSpeakerTrc,
    kSpeakerTrr,
    kSpeakerLfe2
};

enum VstSpeakerArrangementType {
    kSpeakerArrUserDefined = -2,
    kSpeakerArrEmpty       = -1,
    kSpeakerArrMono        =  0,
    kSpeakerArrStereo,
    kSpeakerArrStereoSurround,
    kSpeakerArrStereoCenter,
    kSpeakerArrStereoSide,
    kSpeakerArrStereoCLfe,
    kSpeakerArr30Cine,
    kSpeakerArr30Music,
    kSpeakerArr31Cine,
    kSpeakerArr31Music,
    kSpeakerArr40Cine,
    kSpeakerArr40Music,
    kSpeakerArr41Cine,
    kSpeakerArr41Music,
    kSpeakerArr50,
    kSpeakerArr51,
    kSpeakerArr60Cine,
    kSpeakerArr60Music,
    kSpeakerArr61Cine,
    kSpeakerArr61Music,
    kSpeakerArr70Cine,
    kSpeakerArr70Music,
    kSpeakerArr71Cine,
    kSpeakerArr71Music,
    kSpeakerArr80Cine,
    kSpeakerArr80Music,
    kSpeakerArr81Cine,
    kSpeakerArr81Music,
    kSpeakerArr102,
    kNumSpeakerArr
};

struct VstVariableIo {
    float**  inputs;
    float**  outputs;
    VstInt32 numSamplesInput;
    VstInt32 numSamplesOutput;
    VstInt32* numSamplesInputProcessed;
    VstInt32* numSamplesOutputProcessed;
};

enum VstPanLawType {
    kLinearPanLaw = 0,
    kEqualPowerPanLaw
};

#endif // __aeffectx__
