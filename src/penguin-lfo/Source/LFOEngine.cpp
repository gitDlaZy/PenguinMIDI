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
