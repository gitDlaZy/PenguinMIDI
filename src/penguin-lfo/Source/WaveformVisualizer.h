#pragma once
#include <JuceHeader.h>
#include "LFOEngine.h"
#include <array>

class WaveformVisualizer : public juce::Component, private juce::Timer {
public:
    WaveformVisualizer();
    void paint(juce::Graphics& g) override;
    void updateLFOs(const std::array<LFOInstance, 4>& lfos, float bpm, float sampleRate,
                    float filterLowCut = 20.0f, float filterHighCut = 20000.0f);

private:
    void timerCallback() override { repaint(); }
    std::array<LFOInstance, 4> lfoCopy;
    float bpm           = 120.0f;
    float sampleRate    = 44100.0f;
    float filterLowCut  = 20.0f;
    float filterHighCut = 20000.0f;
};
