#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PenguinMIDIEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit PenguinMIDIEditor(PenguinMIDIProcessor&);
    ~PenguinMIDIEditor() override;

    void resized() override;
    void paint(juce::Graphics&) override {}

private:
    void timerCallback() override;

    PenguinMIDIProcessor& processor;
    std::unique_ptr<juce::WebBrowserComponent> webView;
    double lastPushedBpm = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinMIDIEditor)
};
