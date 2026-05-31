#pragma once
#include <JuceHeader.h>

class PenguinMIDIProcessor : public juce::AudioProcessor {
public:
    PenguinMIDIProcessor();
    ~PenguinMIDIProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PenguinMIDI"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.0; }
    int    getNumPrograms()  override { return 1; }
    int    getCurrentProgram() override { return 0; }
    void   setCurrentProgram(int) override {}
    const  juce::String getProgramName(int) override { return {}; }
    void   changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    void scheduleMidiNote(int channel, int note, int velocity, int durationMs);
    double getCurrentBpm() const { return lastBpm.load(); }
    void savePatterns(const juce::String& jsonString);
    juce::String loadPatternsJson();

private:
    juce::MidiMessageCollector midiCollector;
    std::atomic<double> lastBpm { 120.0 };

    juce::File getPatternsFile() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinMIDIProcessor)
};
