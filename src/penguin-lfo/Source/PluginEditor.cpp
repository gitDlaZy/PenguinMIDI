#include "PluginEditor.h"

static const char* SHAPE_NAMES[]  = {"Sine","Square","Saw Up","Saw Down","Triangle","S&H"};
static const char* TARGET_NAMES[] = {"Volume","Filter","Pan","Pitch"};

// ── LFORow ────────────────────────────────────────────────────────────

LFORow::LFORow(int index) {
    label.setText("LFO " + juce::String(index + 1), juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);

    for (int i = 0; i < 6; ++i)              shapeBox.addItem(SHAPE_NAMES[i],  i + 1);
    for (int i = 0; i < LFO_RATE_COUNT; ++i) rateBox.addItem(LFO_RATES[i].name, i + 1);
    for (int i = 0; i < 4; ++i)              targetBox.addItem(TARGET_NAMES[i], i + 1);

    depthSlider.setRange(0.0, 1.0, 0.01);
    depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    depthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    enableToggle.setButtonText("ON");

    addAndMakeVisible(shapeBox);
    addAndMakeVisible(rateBox);
    addAndMakeVisible(targetBox);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(enableToggle);
}

void LFORow::resized() {
    auto r = getLocalBounds().reduced(2);
    label.setBounds(r.removeFromLeft(40));
    r.removeFromLeft(4);
    enableToggle.setBounds(r.removeFromRight(36));
    r.removeFromRight(4);
    depthSlider.setBounds(r.removeFromRight(80));
    r.removeFromRight(4);
    int cw = r.getWidth() / 3;
    shapeBox.setBounds(r.removeFromLeft(cw));
    rateBox.setBounds(r.removeFromLeft(cw));
    targetBox.setBounds(r);
}

// ── PenguinLFOEditor ──────────────────────────────────────────────────

PenguinLFOEditor::PenguinLFOEditor(PenguinLFOProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
    setSize(520, 320);
    addAndMakeVisible(presetBox);
    addAndMakeVisible(saveButton);
    for (auto& row : lfoRows) addAndMakeVisible(row);
    addAndMakeVisible(visualizer);

    populatePresetBox();
    for (int i = 0; i < 4; ++i) syncRowFromLFO(i);

    presetBox.onChange  = [this] { onPresetSelected(); };
    saveButton.onClick  = [this] { onSavePreset(); };

    for (int i = 0; i < 4; ++i) {
        lfoRows[i].shapeBox.onChange         = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].rateBox.onChange          = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].targetBox.onChange        = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].depthSlider.onValueChange = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].enableToggle.onClick      = [this, i] { syncLFOFromRow(i); };
    }
    startTimerHz(30);
}

void PenguinLFOEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("PenguinLFO",
               getLocalBounds().removeFromTop(30).reduced(8, 4),
               juce::Justification::centredLeft);
}

void PenguinLFOEditor::resized() {
    auto area   = getLocalBounds().reduced(8);
    auto header = area.removeFromTop(30);
    presetBox.setBounds(header.removeFromLeft(260));
    header.removeFromLeft(8);
    saveButton.setBounds(header.removeFromLeft(60));

    visualizer.setBounds(area.removeFromBottom(60));
    int rowH = area.getHeight() / 4;
    for (auto& row : lfoRows)
        row.setBounds(area.removeFromTop(rowH));
}

void PenguinLFOEditor::timerCallback() {
    visualizer.updateLFOs(processor.lfos, processor.currentBPM,
                          static_cast<float>(processor.currentSampleRate));
}

void PenguinLFOEditor::populatePresetBox() {
    presetBox.clear();
    int id = 1;
    for (const auto& p : processor.factoryPresets) presetBox.addItem(p.name, id++);
    if (!processor.userPresets.empty()) {
        presetBox.addSeparator();
        for (const auto& p : processor.userPresets) presetBox.addItem(p.name, id++);
    }
}

void PenguinLFOEditor::onPresetSelected() {
    int idx = presetBox.getSelectedItemIndex();
    if (idx < 0) return;
    if (idx < static_cast<int>(processor.factoryPresets.size())) {
        processor.applyPreset(processor.factoryPresets[idx]);
        for (int i = 0; i < 4; ++i) syncRowFromLFO(i);
    }
}

void PenguinLFOEditor::onSavePreset() {
    auto* aw = new juce::AlertWindow("Save Preset", "Enter a name:", juce::AlertWindow::NoIcon);
    aw->addTextEditor("name", "My Preset");
    aw->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true,
        juce::ModalCallbackFunction::forComponent(
            [this](int result, juce::AlertWindow* w) {
                if (result == 1) {
                    auto name = w->getTextEditorContents("name");
                    if (name.isNotEmpty()) {
                        PresetData p;
                        p.name = name.toStdString();
                        p.lfos = processor.lfos;
                        processor.userPresets.push_back(p);
                        populatePresetBox();
                    }
                }
                delete w;
            }, aw), true);
}

void PenguinLFOEditor::syncRowFromLFO(int i) {
    auto& lfo = processor.lfos[i];
    auto& row = lfoRows[i];
    row.shapeBox.setSelectedId(static_cast<int>(lfo.shape) + 1,  juce::dontSendNotification);
    row.rateBox.setSelectedId(lfo.rateIndex + 1,                  juce::dontSendNotification);
    row.targetBox.setSelectedId(static_cast<int>(lfo.target) + 1, juce::dontSendNotification);
    row.depthSlider.setValue(lfo.depth,                           juce::dontSendNotification);
    row.enableToggle.setToggleState(lfo.enabled,                  juce::dontSendNotification);
}

void PenguinLFOEditor::syncLFOFromRow(int i) {
    auto& lfo = processor.lfos[i];
    auto& row = lfoRows[i];
    lfo.shape     = static_cast<LFOShape>(row.shapeBox.getSelectedId() - 1);
    lfo.rateIndex = row.rateBox.getSelectedId() - 1;
    lfo.target    = static_cast<LFOTarget>(row.targetBox.getSelectedId() - 1);
    lfo.depth     = static_cast<float>(row.depthSlider.getValue());
    lfo.enabled   = row.enableToggle.getToggleState();
}
