#pragma once
#include <JuceHeader.h>
#include "LFOEngine.h"
#include "PresetManager.h"
#include <array>
#include <atomic>

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
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void applyPreset(const PresetData& preset);
    void updateLFOParam(int index, LFOShape shape, int rateIndex,
                        LFOTarget target, float depth, bool enabled,
                        float smoothing, float pitchCenter);
    void updateCustomWaveform(int index, const CustomWaveform& wf);

    // Public — read by editor and visualizer (message thread only)
    std::array<LFOInstance, 4> lfos;
    std::vector<PresetData>    factoryPresets;
    std::vector<PresetData>    userPresets;
    float currentBPM         = 120.0f;
    double currentSampleRate = 44100.0;
    juce::String userPresetsFilePath;

    float filterLowCut  = 20.0f;
    float filterHighCut = 20000.0f;

    void saveUserPresets() const;

private:
    std::array<LFOInstance, 4> pendingLfos;
    std::atomic<bool>          presetPending { false };

    std::array<LFOInstance, 4> pendingParamUpdates {};
    std::atomic<uint32_t>      paramUpdateMask { 0 };

    float pendingFilterLowCut  = 20.0f;
    float pendingFilterHighCut = 20000.0f;

    // Vibrato delay line for Pitch target (stereo, ~50ms max)
    std::vector<float> vibratoL, vibratoR;
    int   vibratoWritePos  = 0;
    int   vibratoBaseDelay = 0;

    juce::dsp::StateVariableTPTFilter<float> hpLeft, hpRight;
    juce::dsp::StateVariableTPTFilter<float> lpLeft, lpRight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinLFOProcessor)
};
