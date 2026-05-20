#pragma once
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

enum class LFOShape  { Sine, Square, SawUp, SawDown, Triangle, SampleAndHold, Custom };
enum class LFOTarget { Volume, Filter, Pan, Pitch };

struct LFORateEntry {
    float       beats;
    const char* name;
};

constexpr int LFO_RATE_8_1   = 0;
constexpr int LFO_RATE_4_1   = 1;
constexpr int LFO_RATE_2_1   = 2;
constexpr int LFO_RATE_1_1   = 3;
constexpr int LFO_RATE_1_2   = 4;
constexpr int LFO_RATE_1_4   = 5;
constexpr int LFO_RATE_1_8   = 6;
constexpr int LFO_RATE_1_16  = 7;
constexpr int LFO_RATE_1_32  = 8;
constexpr int LFO_RATE_1_64  = 9;
constexpr int LFO_RATE_1_1D  = 10;
constexpr int LFO_RATE_1_2D  = 11;
constexpr int LFO_RATE_1_4D  = 12;
constexpr int LFO_RATE_1_8D  = 13;
constexpr int LFO_RATE_1_16D = 14;
constexpr int LFO_RATE_1_2T  = 15;
constexpr int LFO_RATE_1_4T  = 16;
constexpr int LFO_RATE_1_8T  = 17;
constexpr int LFO_RATE_1_16T = 18;
constexpr int LFO_RATE_1_32T = 19;
constexpr int LFO_RATE_3_16  = 20;
constexpr int LFO_RATE_5_16  = 21;
constexpr int LFO_RATE_7_16  = 22;
constexpr int LFO_RATE_5_8   = 23;
constexpr int LFO_RATE_7_8   = 24;
constexpr int LFO_RATE_COUNT = 25;

struct WaveNode { float x; float y; }; // x in [0,1], y in [-1,1]

// Fixed-size (no heap) so LFOInstance stays copyable on the audio thread.
struct CustomWaveform {
    bool     isStepMode = false;
    int      stepCount  = 16;     // 8, 16, or 32
    float    steps[32]  = {};     // y values in [-1,1]
    WaveNode nodes[32]  = {};     // sorted by x
    int      nodeCount  = 0;
};

struct LFOInstance {
    LFOShape       shape              = LFOShape::Sine;
    LFOTarget      target             = LFOTarget::Volume;
    int            rateIndex          = LFO_RATE_1_4;
    float          depth              = 1.0f;
    bool           enabled            = true;
    float          phase              = 0.0f;       // [0,1)
    float          sampleAndHoldValue = 0.0f;
    float          smoothing          = 0.0f;       // 0–1, maps to 0–20 ms at phase wrap
    float          pitchCenter        = 0.0f;       // -1–+1, bias for Pitch target
    CustomWaveform customWave;
};

// Returns LFO value in [-1, 1] for the given phase [0, 1]
float lfoValueAtPhase(const LFOInstance& lfo, float phase);

// Advances phase, applies smoothing ramp, returns value in [-1,1].
// sampleRate needed to convert smoothing (0-1) to a fade duration in samples.
float lfoAdvance(LFOInstance& lfo, float phaseIncrement, float sampleRate = 44100.0f);

extern const LFORateEntry LFO_RATES[LFO_RATE_COUNT];

float lfoPhaseIncrement(int rateIndex, float bpm, float sampleRate);
