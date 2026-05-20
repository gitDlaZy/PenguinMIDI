#pragma once
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

enum class LFOShape  { Sine, Square, SawUp, SawDown, Triangle, SampleAndHold };
enum class LFOTarget { Volume, Filter, Pan, Pitch };

struct LFOInstance {
    LFOShape  shape              = LFOShape::Sine;
    LFOTarget target             = LFOTarget::Volume;
    int       rateIndex          = 5;    // default: 1/4
    float     depth              = 1.0f;
    bool      enabled            = true;
    float     phase              = 0.0f; // [0, 1)
    float     sampleAndHoldValue = 0.0f;
};

// Returns LFO value in [-1, 1] for the given phase [0, 1]
float lfoValueAtPhase(const LFOInstance& lfo, float phase);

// Advances phase by phaseIncrement, returns new value. Updates S&H on phase wrap.
float lfoAdvance(LFOInstance& lfo, float phaseIncrement);
