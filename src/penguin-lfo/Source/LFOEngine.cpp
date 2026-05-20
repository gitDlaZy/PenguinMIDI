#include "LFOEngine.h"
#include <algorithm>

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
        case LFOShape::Custom: {
            const auto& cw = lfo.customWave;
            if (cw.isStepMode) {
                int idx = static_cast<int>(phase * cw.stepCount);
                idx = std::max(0, std::min(idx, cw.stepCount - 1));
                return cw.steps[idx];
            }
            if (cw.nodeCount == 0) return 0.0f;
            if (phase <= cw.nodes[0].x) return cw.nodes[0].y;
            if (phase >= cw.nodes[cw.nodeCount - 1].x) return cw.nodes[cw.nodeCount - 1].y;
            for (int i = 0; i < cw.nodeCount - 1; ++i) {
                if (phase >= cw.nodes[i].x && phase <= cw.nodes[i + 1].x) {
                    float span = cw.nodes[i + 1].x - cw.nodes[i].x;
                    if (span < 0.0001f) return cw.nodes[i].y;
                    float t = (phase - cw.nodes[i].x) / span;
                    return cw.nodes[i].y * (1.0f - t) + cw.nodes[i + 1].y * t;
                }
            }
            return 0.0f;
        }
    }
    return 0.0f;
}

float lfoAdvance(LFOInstance& lfo, float phaseIncrement, float sampleRate) {
    lfo.phase += phaseIncrement;
    if (lfo.phase >= 1.0f) {
        lfo.phase -= 1.0f;
        lfo.sampleAndHoldValue =
            (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
    }
    float raw = lfoValueAtPhase(lfo, lfo.phase);

    if (lfo.smoothing > 0.0f && phaseIncrement > 0.0f) {
        float fadeSamples = lfo.smoothing * 20.0f / 1000.0f * sampleRate;
        float fadePhase   = fadeSamples * phaseIncrement;
        fadePhase = std::min(fadePhase, 0.49f);
        if (lfo.phase < fadePhase)
            raw *= lfo.phase / fadePhase;
        else if (lfo.phase > (1.0f - fadePhase))
            raw *= (1.0f - lfo.phase) / fadePhase;
    }
    return raw;
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
    {4.0f/3.0f, "1/2t"},
    {2.0f/3.0f, "1/4t"},
    {1.0f/3.0f, "1/8t"},
    {1.0f/6.0f, "1/16t"},
    {1.0f/12.0f,"1/32t"},
    {0.75f,    "3/16"},
    {1.25f,    "5/16"},
    {1.75f,    "7/16"},
    {2.5f,     "5/8"},
    {3.5f,     "7/8"},
};

float lfoPhaseIncrement(int rateIndex, float bpm, float sampleRate) {
    if (rateIndex < 0 || rateIndex >= LFO_RATE_COUNT) rateIndex = LFO_RATE_1_4;
    return bpm / (60.0f * sampleRate * LFO_RATES[rateIndex].beats);
}
