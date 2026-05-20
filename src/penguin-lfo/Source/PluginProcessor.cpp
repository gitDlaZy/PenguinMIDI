#include "PluginProcessor.h"
#include "PluginEditor.h"

PenguinLFOProcessor::PenguinLFOProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    auto pluginDir   = juce::File::getSpecialLocation(
                           juce::File::currentExecutableFile).getParentDirectory();
    auto presetsFile = pluginDir.getChildFile("PenguinLFO_Presets/presets.json");
    if (presetsFile.existsAsFile())
        factoryPresets = loadPresetsFromFile(presetsFile.getFullPathName().toStdString());

    for (auto& lfo : lfos) {
        lfo.shape     = LFOShape::Sine;
        lfo.rateIndex = LFO_RATE_1_4;
        lfo.target    = LFOTarget::Volume;
        lfo.depth     = 0.5f;
        lfo.enabled   = false;
    }
    lfos[0].enabled = true;

    if (!factoryPresets.empty())
        applyPreset(factoryPresets[0]);
}

void PenguinLFOProcessor::prepareToPlay(double sampleRate, int) {
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels      = 1;

    filterLeft.prepare(spec);
    filterRight.prepare(spec);
    filterLeft.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filterRight.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filterLeft.setCutoffFrequency(20000.0f);
    filterRight.setCutoffFrequency(20000.0f);
}

void PenguinLFOProcessor::applyPreset(const PresetData& preset) {
    for (int i = 0; i < 4; ++i)
        lfos[i] = preset.lfos[i];
}

void PenguinLFOProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer&) {
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                currentBPM = static_cast<float>(*bpm);

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    int   N = buffer.getNumSamples();

    for (int s = 0; s < N; ++s) {
        float gainMod     = 1.0f;
        float panMod      = 0.0f;
        float filterCutoff = 20000.0f;

        for (int i = 0; i < 4; ++i) {
            if (!lfos[i].enabled) continue;
            float inc = lfoPhaseIncrement(lfos[i].rateIndex,
                                          currentBPM,
                                          static_cast<float>(currentSampleRate));
            float val = lfoAdvance(lfos[i], inc); // [-1, 1]

            switch (lfos[i].target) {
                case LFOTarget::Volume:
                    gainMod *= 1.0f - lfos[i].depth * (1.0f - (val * 0.5f + 0.5f));
                    break;
                case LFOTarget::Filter:
                    filterCutoff = 200.0f * std::pow(100.0f,
                        (val * 0.5f + 0.5f) * lfos[i].depth + (1.0f - lfos[i].depth));
                    filterCutoff = juce::jlimit(200.0f, 20000.0f, filterCutoff);
                    break;
                case LFOTarget::Pan:
                    panMod = juce::jlimit(-1.0f, 1.0f, panMod + val * lfos[i].depth);
                    break;
                case LFOTarget::Pitch:
                    gainMod *= 1.0f + val * lfos[i].depth * 0.05f; // subtle ring-mod vibrato
                    break;
            }
        }

        filterLeft.setCutoffFrequency(filterCutoff);
        filterRight.setCutoffFrequency(filterCutoff);

        float leftGain  = std::cos((panMod + 1.0f) * juce::MathConstants<float>::pi / 4.0f);
        float rightGain = std::sin((panMod + 1.0f) * juce::MathConstants<float>::pi / 4.0f);

        L[s] = filterLeft.processSample(0, L[s]) * gainMod * leftGain;
        if (R)
            R[s] = filterRight.processSample(0, R[s]) * gainMod * rightGain;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PenguinLFOProcessor();
}
