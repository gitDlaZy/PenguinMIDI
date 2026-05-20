#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "WaveformVisualizer.h"
#include <array>

class LFORow : public juce::Component {
public:
    explicit LFORow(int index);
    void resized() override;
    juce::ComboBox    shapeBox, rateBox, targetBox;
    juce::Slider      depthSlider;
    juce::ToggleButton enableToggle;
private:
    juce::Label label;
};

class PenguinLFOEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit PenguinLFOEditor(PenguinLFOProcessor&);
    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    void timerCallback() override;
    void populatePresetBox();
    void onPresetSelected();
    void onSavePreset();
    void syncRowFromLFO(int i);
    void syncLFOFromRow(int i);

    PenguinLFOProcessor& processor;
    juce::ComboBox   presetBox;
    juce::TextButton saveButton { "Save" };
    std::array<LFORow, 4> lfoRows { LFORow(0), LFORow(1), LFORow(2), LFORow(3) };
    WaveformVisualizer visualizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PenguinLFOEditor)
};
