#include "PluginEditor.h"

static const char* SHAPE_NAMES[]  = {"Sine","Square","Saw Up","Saw Down","Triangle","S&H","Custom"};
static const char* TARGET_NAMES[] = {"Volume","Filter","Pan","Pitch"};

// ── LFORow ────────────────────────────────────────────────────────────

LFORow::LFORow(int index) {
    label.setText("LFO " + juce::String(index + 1), juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);

    for (int i = 0; i < 7; ++i)              shapeBox.addItem(SHAPE_NAMES[i],  i + 1);
    for (int i = 0; i < LFO_RATE_COUNT; ++i) rateBox.addItem(LFO_RATES[i].name, i + 1);
    for (int i = 0; i < 4; ++i)              targetBox.addItem(TARGET_NAMES[i], i + 1);

    depthSlider.setRange(0.0, 1.0, 0.01);
    depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    depthSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    enableToggle.setButtonText("ON");

    smoothingSlider.setRange(0.0, 1.0, 0.001);
    smoothingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    smoothingSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    smoothingLabel.setText("0.0 ms", juce::dontSendNotification);
    smoothingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    smoothingLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    smoothingLabel.setJustificationType(juce::Justification::centredLeft);

    pitchCenterSlider.setRange(-1.0, 1.0, 0.01);
    pitchCenterSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    pitchCenterSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    pitchCenterLabel.setText("+0.00", juce::dontSendNotification);
    pitchCenterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    pitchCenterLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    pitchCenterLabel.setJustificationType(juce::Justification::centredLeft);

    smoothingSlider.onValueChange = [this] {
        float ms = static_cast<float>(smoothingSlider.getValue()) * 20.0f;
        smoothingLabel.setText(juce::String(ms, 1) + " ms", juce::dontSendNotification);
    };
    pitchCenterSlider.onValueChange = [this] {
        float v = static_cast<float>(pitchCenterSlider.getValue());
        pitchCenterLabel.setText((v >= 0 ? "+" : "") + juce::String(v, 2), juce::dontSendNotification);
    };

    addAndMakeVisible(shapeBox);
    addAndMakeVisible(rateBox);
    addAndMakeVisible(targetBox);
    addAndMakeVisible(depthSlider);
    addAndMakeVisible(enableToggle);
    addAndMakeVisible(smoothingSlider);
    addAndMakeVisible(smoothingLabel);
    addAndMakeVisible(pitchCenterSlider);
    addAndMakeVisible(pitchCenterLabel);
    addChildComponent(waveEditor); // starts hidden

    updateControlVisibility();
}

void LFORow::updateControlVisibility() {
    bool isPitch  = (static_cast<LFOTarget>(targetBox.getSelectedId() - 1) == LFOTarget::Pitch);
    pitchCenterSlider.setVisible(isPitch);
    pitchCenterLabel.setVisible(isPitch);
}

void LFORow::resized() {
    auto r = getLocalBounds();
    auto topRow = r.removeFromTop(28).reduced(2, 0);

    topRow.removeFromLeft(0);
    label.setBounds(topRow.removeFromLeft(40));
    topRow.removeFromLeft(4);
    enableToggle.setBounds(topRow.removeFromRight(36));
    topRow.removeFromRight(4);
    smoothingLabel.setBounds(topRow.removeFromRight(40));
    smoothingSlider.setBounds(topRow.removeFromRight(60));
    topRow.removeFromRight(4);
    if (pitchCenterSlider.isVisible()) {
        pitchCenterLabel.setBounds(topRow.removeFromRight(40));
        pitchCenterSlider.setBounds(topRow.removeFromRight(60));
        topRow.removeFromRight(4);
    }
    depthSlider.setBounds(topRow.removeFromRight(80));
    topRow.removeFromRight(4);
    int cw = topRow.getWidth() / 3;
    shapeBox.setBounds(topRow.removeFromLeft(cw));
    rateBox.setBounds(topRow.removeFromLeft(cw));
    targetBox.setBounds(topRow);

    if (waveEditor.isVisible())
        waveEditor.setBounds(r.reduced(2));
}

// ── PenguinLFOEditor ──────────────────────────────────────────────────

PenguinLFOEditor::PenguinLFOEditor(PenguinLFOProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {

    addAndMakeVisible(presetBox);
    addAndMakeVisible(saveButton);
    for (auto& row : lfoRows) addAndMakeVisible(row);
    addAndMakeVisible(visualizer);

    // Filter section setup
    lowCutSlider.setRange(20.0, 20000.0, 1.0);
    lowCutSlider.setSkewFactorFromMidPoint(1000.0);
    lowCutSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    lowCutSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    lowCutSlider.setValue(processor.filterLowCut, juce::dontSendNotification);

    highCutSlider.setRange(20.0, 20000.0, 1.0);
    highCutSlider.setSkewFactorFromMidPoint(1000.0);
    highCutSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    highCutSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    highCutSlider.setValue(processor.filterHighCut, juce::dontSendNotification);

    auto setupHzLabel = [](juce::Label& lbl, float hz) {
        lbl.setText(juce::String(static_cast<int>(hz)) + " Hz", juce::dontSendNotification);
        lbl.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        lbl.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
        lbl.setEditable(true);
    };
    setupHzLabel(lowCutLabel,  processor.filterLowCut);
    setupHzLabel(highCutLabel, processor.filterHighCut);

    lowCutTitle.setText("Low Cut",   juce::dontSendNotification);
    highCutTitle.setText("High Cut", juce::dontSendNotification);
    for (auto* lbl : { &lowCutTitle, &highCutTitle }) {
        lbl->setColour(juce::Label::textColourId, juce::Colours::grey);
        lbl->setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    }

    lowCutSlider.onValueChange = [this] {
        float val = static_cast<float>(lowCutSlider.getValue());
        val = std::min(val, processor.filterHighCut);
        processor.filterLowCut = val;
        lowCutLabel.setText(juce::String(static_cast<int>(val)) + " Hz", juce::dontSendNotification);
    };
    highCutSlider.onValueChange = [this] {
        float val = static_cast<float>(highCutSlider.getValue());
        val = std::max(val, processor.filterLowCut);
        processor.filterHighCut = val;
        highCutLabel.setText(juce::String(static_cast<int>(val)) + " Hz", juce::dontSendNotification);
    };
    lowCutLabel.onEditorHide = [this] {
        float val = juce::jlimit(20.0f, 20000.0f, lowCutLabel.getText().getFloatValue());
        lowCutSlider.setValue(val, juce::sendNotification);
    };
    highCutLabel.onEditorHide = [this] {
        float val = juce::jlimit(20.0f, 20000.0f, highCutLabel.getText().getFloatValue());
        highCutSlider.setValue(val, juce::sendNotification);
    };

    addAndMakeVisible(lowCutSlider);  addAndMakeVisible(lowCutLabel);  addAndMakeVisible(lowCutTitle);
    addAndMakeVisible(highCutSlider); addAndMakeVisible(highCutLabel); addAndMakeVisible(highCutTitle);

    populatePresetBox();
    for (int i = 0; i < 4; ++i) syncRowFromLFO(i);

    presetBox.onChange = [this] { onPresetSelected(); };
    saveButton.onClick = [this] { onSavePreset(); };

    for (int i = 0; i < 4; ++i) {
        lfoRows[i].shapeBox.onChange         = [this, i] { onShapeChanged(i); };
        lfoRows[i].rateBox.onChange          = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].targetBox.onChange        = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].depthSlider.onValueChange = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].enableToggle.onClick      = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].smoothingSlider.onValueChange   = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].pitchCenterSlider.onValueChange = [this, i] { syncLFOFromRow(i); };
        lfoRows[i].waveEditor.onChange             = [this, i] { updateCustomWaveform(i); };
    }

    setSize(520, 400);
    updateLFORowHeights();
    startTimerHz(30);
}

void PenguinLFOEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions().withHeight(16.0f).withStyle("Bold")));
    g.drawText("PenguinLFO",
               getLocalBounds().removeFromTop(30).reduced(8, 4),
               juce::Justification::centredLeft);

    if (!filterSectionBounds.isEmpty()) {
        g.setColour(juce::Colour(0xff333355));
        g.drawRoundedRectangle(filterSectionBounds.toFloat().reduced(1.0f), 3.0f, 1.0f);
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
        g.setColour(juce::Colours::grey);
        g.drawText("Filter", filterSectionBounds.withWidth(36).withTrimmedTop(2).toFloat(),
                   juce::Justification::centredLeft);
    }
}

void PenguinLFOEditor::resized() {
    auto area = getLocalBounds().reduced(8);
    auto header = area.removeFromTop(30);
    presetBox.setBounds(header.removeFromLeft(260));
    header.removeFromLeft(8);
    saveButton.setBounds(header.removeFromLeft(60));

    visualizer.setBounds(area.removeFromBottom(60));
    area.removeFromBottom(4);

    // Filter section
    auto filterArea = area.removeFromBottom(46);
    filterSectionBounds = filterArea;
    filterArea.reduce(4, 4);
    int halfW = filterArea.getWidth() / 2 - 4;
    auto lcArea = filterArea.removeFromLeft(halfW);
    lowCutTitle.setBounds(lcArea.removeFromLeft(54));
    lowCutSlider.setBounds(lcArea.removeFromLeft(100));
    lcArea.removeFromLeft(2);
    lowCutLabel.setBounds(lcArea);
    filterArea.removeFromLeft(8);
    highCutTitle.setBounds(filterArea.removeFromLeft(54));
    highCutSlider.setBounds(filterArea.removeFromLeft(100));
    filterArea.removeFromLeft(2);
    highCutLabel.setBounds(filterArea);

    area.removeFromBottom(4);
    updateLFORowHeights();
}

void PenguinLFOEditor::updateLFORowHeights() {
    auto area = getLocalBounds().reduced(8);
    area.removeFromTop(30);
    area.removeFromBottom(60 + 4 + 46 + 4); // visualizer + filter section + gaps

    for (auto& row : lfoRows) {
        int h = row.getIdealHeight();
        row.setBounds(area.removeFromTop(h));
    }

    int totalH = 30 + 8 + 60 + 4 + 46 + 4 + 8;
    for (auto& row : lfoRows) totalH += row.getIdealHeight();
    setSize(520, totalH);
}

void PenguinLFOEditor::timerCallback() {
    visualizer.updateLFOs(processor.lfos, processor.currentBPM,
                          static_cast<float>(processor.currentSampleRate),
                          processor.filterLowCut, processor.filterHighCut);

    // Sync filter slider positions if changed by preset (without triggering callbacks)
    if (std::abs(static_cast<float>(lowCutSlider.getValue()) - processor.filterLowCut) > 0.5f) {
        lowCutSlider.setValue(processor.filterLowCut, juce::dontSendNotification);
        lowCutLabel.setText(juce::String(static_cast<int>(processor.filterLowCut)) + " Hz",
                            juce::dontSendNotification);
    }
    if (std::abs(static_cast<float>(highCutSlider.getValue()) - processor.filterHighCut) > 0.5f) {
        highCutSlider.setValue(processor.filterHighCut, juce::dontSendNotification);
        highCutLabel.setText(juce::String(static_cast<int>(processor.filterHighCut)) + " Hz",
                             juce::dontSendNotification);
    }
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
    int nFactory = static_cast<int>(processor.factoryPresets.size());
    if (idx < nFactory)
        processor.applyPreset(processor.factoryPresets[idx]);
    else {
        int userIdx = idx - nFactory;
        if (userIdx < static_cast<int>(processor.userPresets.size()))
            processor.applyPreset(processor.userPresets[userIdx]);
    }
    for (int i = 0; i < 4; ++i) syncRowFromLFO(i);
}

void PenguinLFOEditor::onSavePreset() {
    auto* aw = new juce::AlertWindow("Save Preset", "Enter a name:", juce::AlertWindow::NoIcon);
    aw->addTextEditor("name", "My Preset");
    aw->addButton("OK",     1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    aw->enterModalState(true,
        juce::ModalCallbackFunction::create([this, aw](int result) {
            if (result == 1) {
                auto name = aw->getTextEditorContents("name");
                if (name.isNotEmpty()) {
                    PresetData p;
                    p.name          = name.toStdString();
                    p.lfos          = processor.lfos;
                    p.filterLowCut  = processor.filterLowCut;
                    p.filterHighCut = processor.filterHighCut;
                    processor.userPresets.push_back(p);
                    processor.saveUserPresets();
                    populatePresetBox();
                }
            }
            delete aw;
        }), true);
}

void PenguinLFOEditor::syncRowFromLFO(int i) {
    auto& lfo = processor.lfos[i];
    auto& row = lfoRows[i];
    row.shapeBox.setSelectedId(static_cast<int>(lfo.shape) + 1,   juce::dontSendNotification);
    row.rateBox.setSelectedId(lfo.rateIndex + 1,                   juce::dontSendNotification);
    row.targetBox.setSelectedId(static_cast<int>(lfo.target) + 1,  juce::dontSendNotification);
    row.depthSlider.setValue(lfo.depth,                            juce::dontSendNotification);
    row.enableToggle.setToggleState(lfo.enabled,                   juce::dontSendNotification);
    row.smoothingSlider.setValue(lfo.smoothing,                    juce::dontSendNotification);
    row.smoothingLabel.setText(juce::String(lfo.smoothing * 20.0f, 1) + " ms",
                               juce::dontSendNotification);
    row.pitchCenterSlider.setValue(lfo.pitchCenter,                juce::dontSendNotification);
    row.pitchCenterLabel.setText(
        (lfo.pitchCenter >= 0 ? "+" : "") + juce::String(lfo.pitchCenter, 2),
        juce::dontSendNotification);

    bool isCustom = (lfo.shape == LFOShape::Custom);
    if (isCustom) row.waveEditor.setWaveform(lfo.customWave);
    row.waveEditor.setVisible(isCustom);
    row.updateControlVisibility();
    row.resized();
    updateLFORowHeights();
}

void PenguinLFOEditor::syncLFOFromRow(int i) {
    auto& row = lfoRows[i];
    row.updateControlVisibility();
    row.resized();
    processor.updateLFOParam(
        i,
        static_cast<LFOShape>(row.shapeBox.getSelectedId() - 1),
        row.rateBox.getSelectedId() - 1,
        static_cast<LFOTarget>(row.targetBox.getSelectedId() - 1),
        static_cast<float>(row.depthSlider.getValue()),
        row.enableToggle.getToggleState(),
        static_cast<float>(row.smoothingSlider.getValue()),
        static_cast<float>(row.pitchCenterSlider.getValue())
    );
}

void PenguinLFOEditor::onShapeChanged(int i) {
    bool isCustom = (lfoRows[i].shapeBox.getSelectedId() - 1 == static_cast<int>(LFOShape::Custom));
    for (int j = 0; j < 4; ++j) {
        if (j != i) {
            lfoRows[j].waveEditor.setVisible(false);
            lfoRows[j].resized();
        }
    }
    lfoRows[i].waveEditor.setVisible(isCustom);
    if (isCustom) lfoRows[i].waveEditor.setWaveform(processor.lfos[i].customWave);
    lfoRows[i].resized();
    updateLFORowHeights();
    syncLFOFromRow(i);
}

void PenguinLFOEditor::updateCustomWaveform(int i) {
    processor.updateCustomWaveform(i, lfoRows[i].waveEditor.getWaveform());
}
