#include "CustomWaveformEditor.h"
#include <algorithm>

static const juce::Colour BG   { 0xff0d1117 };
static const juce::Colour GRID { 0xff2a2a3a };
static const juce::Colour LINE { 0xff7ec8e3 };
static const juce::Colour NODE { 0xffffffff };
static const juce::Colour STEP { 0xff4a9eff };

CustomWaveformEditor::CustomWaveformEditor() {
    stepCountBox.addItem("8 steps",  8);
    stepCountBox.addItem("16 steps", 16);
    stepCountBox.addItem("32 steps", 32);
    stepCountBox.setSelectedId(16, juce::dontSendNotification);

    breakpointBtn.setClickingTogglesState(true);
    stepBtn.setClickingTogglesState(true);
    breakpointBtn.setToggleState(true, juce::dontSendNotification);

    breakpointBtn.onClick = [this] {
        if (!breakpointBtn.getToggleState()) {
            breakpointBtn.setToggleState(true, juce::dontSendNotification);
            return;
        }
        stepBtn.setToggleState(false, juce::dontSendNotification);
        waveform.isStepMode = false;
        repaint();
        if (onChange) onChange();
    };
    stepBtn.onClick = [this] {
        if (!stepBtn.getToggleState()) {
            stepBtn.setToggleState(true, juce::dontSendNotification);
            return;
        }
        breakpointBtn.setToggleState(false, juce::dontSendNotification);
        waveform.isStepMode = true;
        repaint();
        if (onChange) onChange();
    };
    stepCountBox.onChange = [this] {
        waveform.stepCount = stepCountBox.getSelectedId();
        repaint();
        if (onChange) onChange();
    };
    clearBtn.onClick = [this] {
        if (waveform.isStepMode) {
            for (int i = 0; i < 32; ++i) waveform.steps[i] = 0.0f;
        } else {
            waveform.nodeCount = 0;
        }
        repaint();
        if (onChange) onChange();
    };

    addAndMakeVisible(breakpointBtn);
    addAndMakeVisible(stepBtn);
    addAndMakeVisible(stepCountBox);
    addAndMakeVisible(clearBtn);
}

void CustomWaveformEditor::setWaveform(const CustomWaveform& wf) {
    waveform = wf;
    breakpointBtn.setToggleState(!wf.isStepMode, juce::dontSendNotification);
    stepBtn.setToggleState(wf.isStepMode, juce::dontSendNotification);
    stepCountBox.setSelectedId(wf.stepCount, juce::dontSendNotification);
    repaint();
}

void CustomWaveformEditor::resized() {
    auto r = getLocalBounds();
    auto toolbar = r.removeFromTop(24);
    breakpointBtn.setBounds(toolbar.removeFromLeft(80));
    toolbar.removeFromLeft(4);
    stepBtn.setBounds(toolbar.removeFromLeft(50));
    toolbar.removeFromLeft(4);
    stepCountBox.setBounds(toolbar.removeFromLeft(80));
    toolbar.removeFromLeft(4);
    clearBtn.setBounds(toolbar.removeFromLeft(50));
}

juce::Rectangle<float> CustomWaveformEditor::editorArea() const {
    return getLocalBounds().toFloat().withTrimmedTop(28.0f).reduced(4.0f, 4.0f);
}

juce::Point<float> CustomWaveformEditor::nodeToScreen(float x, float y) const {
    auto a = editorArea();
    return { a.getX() + x * a.getWidth(),
             a.getY() + (1.0f - (y + 1.0f) * 0.5f) * a.getHeight() };
}

juce::Point<float> CustomWaveformEditor::screenToNorm(float sx, float sy) const {
    auto a = editorArea();
    float x = juce::jlimit(0.0f, 1.0f, (sx - a.getX()) / a.getWidth());
    float y = juce::jlimit(-1.0f, 1.0f, 1.0f - (sy - a.getY()) / a.getHeight() * 2.0f);
    return { x, y };
}

int CustomWaveformEditor::hitTestNode(juce::Point<float> pt) const {
    for (int i = 0; i < waveform.nodeCount; ++i) {
        auto sp = nodeToScreen(waveform.nodes[i].x, waveform.nodes[i].y);
        if (sp.getDistanceFrom(pt) < 8.0f) return i;
    }
    return -1;
}

int CustomWaveformEditor::hitTestStep(juce::Point<float> pt) const {
    auto a = editorArea();
    if (!a.contains(pt)) return -1;
    float stepW = a.getWidth() / waveform.stepCount;
    int idx = static_cast<int>((pt.x - a.getX()) / stepW);
    return juce::jlimit(0, waveform.stepCount - 1, idx);
}

void CustomWaveformEditor::sortNodes() {
    std::sort(waveform.nodes, waveform.nodes + waveform.nodeCount,
              [](const WaveNode& a, const WaveNode& b) { return a.x < b.x; });
}

void CustomWaveformEditor::mouseDown(const juce::MouseEvent& e) {
    auto pt = e.position;
    if (waveform.isStepMode) {
        dragStepIndex = hitTestStep(pt);
        if (dragStepIndex >= 0) {
            auto norm = screenToNorm(pt.x, pt.y);
            waveform.steps[dragStepIndex] = norm.y;
            repaint();
            if (onChange) onChange();
        }
    } else {
        dragNodeIndex = hitTestNode(pt);
        if (dragNodeIndex < 0 && waveform.nodeCount < 32) {
            auto norm = screenToNorm(pt.x, pt.y);
            waveform.nodes[waveform.nodeCount++] = { norm.x, norm.y };
            sortNodes();
            for (int i = 0; i < waveform.nodeCount; ++i)
                if (std::abs(waveform.nodes[i].x - norm.x) < 0.001f) { dragNodeIndex = i; break; }
            repaint();
            if (onChange) onChange();
        }
    }
}

void CustomWaveformEditor::mouseDrag(const juce::MouseEvent& e) {
    auto norm = screenToNorm(e.position.x, e.position.y);
    if (waveform.isStepMode) {
        int idx = hitTestStep(e.position);
        if (idx >= 0) {
            waveform.steps[idx] = norm.y;
            repaint();
            if (onChange) onChange();
        }
    } else if (dragNodeIndex >= 0) {
        waveform.nodes[dragNodeIndex] = { norm.x, norm.y };
        sortNodes();
        repaint();
        if (onChange) onChange();
    }
}

void CustomWaveformEditor::mouseDoubleClick(const juce::MouseEvent& e) {
    if (waveform.isStepMode) return;
    int idx = hitTestNode(e.position);
    if (idx >= 0 && waveform.nodeCount > 1) {
        for (int i = idx; i < waveform.nodeCount - 1; ++i)
            waveform.nodes[i] = waveform.nodes[i + 1];
        --waveform.nodeCount;
        repaint();
        if (onChange) onChange();
    }
}

void CustomWaveformEditor::paintBreakpointEditor(juce::Graphics& g, juce::Rectangle<float> area) {
    float midY = area.getY() + area.getHeight() * 0.5f;
    g.setColour(GRID);
    g.drawHorizontalLine(static_cast<int>(midY), area.getX(), area.getRight());

    if (waveform.nodeCount < 1) return;

    juce::Path path;
    for (int i = 0; i < waveform.nodeCount; ++i) {
        auto sp = nodeToScreen(waveform.nodes[i].x, waveform.nodes[i].y);
        i == 0 ? path.startNewSubPath(sp) : path.lineTo(sp);
    }
    g.setColour(LINE);
    g.strokePath(path, juce::PathStrokeType(1.5f));

    for (int i = 0; i < waveform.nodeCount; ++i) {
        auto sp = nodeToScreen(waveform.nodes[i].x, waveform.nodes[i].y);
        g.setColour(NODE);
        g.fillEllipse(sp.x - 5.0f, sp.y - 5.0f, 10.0f, 10.0f);
    }
}

void CustomWaveformEditor::paintStepEditor(juce::Graphics& g, juce::Rectangle<float> area) {
    float stepW = area.getWidth() / waveform.stepCount;
    float midY  = area.getY() + area.getHeight() * 0.5f;
    g.setColour(GRID);
    g.drawHorizontalLine(static_cast<int>(midY), area.getX(), area.getRight());

    for (int i = 0; i < waveform.stepCount; ++i) {
        float x   = area.getX() + i * stepW + 1.0f;
        float w   = stepW - 2.0f;
        float val = waveform.steps[i];
        float barY, barH;
        if (val >= 0.0f) {
            barH = val * area.getHeight() * 0.5f;
            barY = midY - barH;
        } else {
            barH = -val * area.getHeight() * 0.5f;
            barY = midY;
        }
        g.setColour(STEP);
        if (barH > 0.0f) g.fillRect(x, barY, w, barH);
    }
}

void CustomWaveformEditor::paint(juce::Graphics& g) {
    auto area = editorArea();
    g.setColour(BG);
    g.fillRect(area);
    g.setColour(GRID);
    g.drawRect(area);

    if (waveform.isStepMode)
        paintStepEditor(g, area);
    else
        paintBreakpointEditor(g, area);
}
