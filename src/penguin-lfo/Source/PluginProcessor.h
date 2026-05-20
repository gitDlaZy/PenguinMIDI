#pragma once
#include <JuceHeader.h>
#include "LFOEngine.h"
#include "PresetManager.h"
#include <array>

class PenguinLFOProcessor : public juce::AudioProcessor {
public:
    PenguinLFOProcessor();
    ~PenguinLFOProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PenguinLFO"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int    getNumPrograms()  override { return 1; }
    int    getCurrentProgram() override { return 0; }
    void   setCurrentProgram(int) override {}
    const  juce::String getProgramName(int) override { return {}; }
    void   changeProgramName(int, const juce::String&) override {}
    void   getStateInformation(juce::MemoryBlock&) override {}
    void   setStateInformation(const void*, int) override {}

    void applyPreset(const PresetData& preset);

    // Public so the editor and visualizer can read them
    std::array<LFOInstance, 4> lfos;
    std::vector<PresetData>    factoryPresets;
    std::vector<PresetData>    userPresets;
    float currentBPM        = 120.0f;
    double currentSampleRate = 44100.0;

private:
    juce::dsp::StateVariableTPTFilter<float> filterLeft, filterRight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinLFOProcessor)
};
