#include "WaveformVisualizer.h"

WaveformVisualizer::WaveformVisualizer() { startTimerHz(30); }

void WaveformVisualizer::updateLFOs(const std::array<LFOInstance, 4>& lfos,
                                     float bpm_, float sampleRate_) {
    lfoCopy = lfos;
    bpm = bpm_;
    sampleRate = sampleRate_;
}

void WaveformVisualizer::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff111122));
    g.setColour(juce::Colour(0xff333355));
    g.drawRect(getLocalBounds());

    int   w = getWidth(), h = getHeight();
    float totalBeats      = 4.0f;
    float samplesPerPoint = (totalBeats * 60.0f / bpm * sampleRate) / w;

    std::array<LFOInstance, 4> sim = lfoCopy;
    for (auto& lfo : sim) lfo.phase = 0.0f;

    juce::Path path;
    for (int x = 0; x < w; ++x) {
        float gain = 1.0f;
        for (int i = 0; i < 4; ++i) {
            if (!sim[i].enabled || sim[i].target != LFOTarget::Volume) continue;
            float inc = lfoPhaseIncrement(sim[i].rateIndex, bpm, sampleRate);
            for (int s = 0; s < static_cast<int>(samplesPerPoint); ++s)
                lfoAdvance(sim[i], inc);
            float val = lfoValueAtPhase(sim[i], sim[i].phase);
            gain *= 1.0f - sim[i].depth * (1.0f - (val * 0.5f + 0.5f));
        }
        float y = (1.0f - gain) * (h - 4) + 2;
        x == 0 ? path.startNewSubPath(x, y) : path.lineTo(x, y);
    }
    g.setColour(juce::Colour(0xff7ec8e3));
    g.strokePath(path, juce::PathStrokeType(1.5f));
}
