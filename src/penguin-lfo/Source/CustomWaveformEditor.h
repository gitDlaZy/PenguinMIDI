#pragma once
#include <JuceHeader.h>
#include "LFOEngine.h"
#include <functional>

class CustomWaveformEditor : public juce::Component {
public:
    CustomWaveformEditor();

    void setWaveform(const CustomWaveform& wf);
    const CustomWaveform& getWaveform() const { return waveform; }

    std::function<void()> onChange;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

private:
    CustomWaveform waveform;

    juce::TextButton breakpointBtn { "Breakpoint" };
    juce::TextButton stepBtn       { "Step" };
    juce::ComboBox   stepCountBox;
    juce::TextButton clearBtn      { "Clear" };

    int dragNodeIndex = -1;
    int dragStepIndex = -1;

    juce::Rectangle<float> editorArea() const;
    juce::Point<float> nodeToScreen(float x, float y) const;
    juce::Point<float> screenToNorm(float sx, float sy) const;

    int hitTestNode(juce::Point<float> screenPt) const;
    int hitTestStep(juce::Point<float> screenPt) const;

    void sortNodes();
    void paintBreakpointEditor(juce::Graphics& g, juce::Rectangle<float> area);
    void paintStepEditor(juce::Graphics& g, juce::Rectangle<float> area);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomWaveformEditor)
};
