#include "LFOEngine.h"

float lfoValueAtPhase(const LFOInstance& lfo, float phase) {
    switch (lfo.shape) {
        case LFOShape::Sine:
            return std::sin(phase * 2.0f * static_cast<float>(M_PI));
        case LFOShape::Square:
            return phase < 0.5f ? 1.0f : -1.0f;
        case LFOShape::SawUp:
            return 2.0f * phase - 1.0f;
        case LFOShape::SawDown:
            return 1.0f - 2.0f * phase;
        case LFOShape::Triangle:
            return phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
        case LFOShape::SampleAndHold:
            return lfo.sampleAndHoldValue;
    }
    return 0.0f;
}

float lfoAdvance(LFOInstance& lfo, float phaseIncrement) {
    lfo.phase += phaseIncrement;
    if (lfo.phase >= 1.0f) {
        lfo.phase -= 1.0f;
        lfo.sampleAndHoldValue =
            (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
    }
    return lfoValueAtPhase(lfo, lfo.phase);
}

const LFORateEntry LFO_RATES[LFO_RATE_COUNT] = {
    {32.0f,    "8/1"},
    {16.0f,    "4/1"},
    {8.0f,     "2/1"},
    {4.0f,     "1/1"},
    {2.0f,     "1/2"},
    {1.0f,     "1/4"},
    {0.5f,     "1/8"},
    {0.25f,    "1/16"},
    {0.125f,   "1/32"},
    {0.0625f,  "1/64"},
    {6.0f,     "1/1d"},
    {3.0f,     "1/2d"},
    {1.5f,     "1/4d"},
    {0.75f,    "1/8d"},
    {0.375f,   "1/16d"},
    {1.3333f,  "1/2t"},
    {0.6667f,  "1/4t"},
    {0.3333f,  "1/8t"},
    {0.1667f,  "1/16t"},
    {0.08333f, "1/32t"},
    {0.75f,    "3/16"},
    {1.25f,    "5/16"},
    {1.75f,    "7/16"},
    {2.5f,     "5/8"},
    {3.5f,     "7/8"},
};

float lfoPhaseIncrement(int rateIndex, float bpm, float sampleRate) {
    // cycles/sample = bpm / (60 * sampleRate * beatsPerCycle)
    return bpm / (60.0f * sampleRate * LFO_RATES[rateIndex].beats);
}
