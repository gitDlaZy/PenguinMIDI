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

void PenguinLFOProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels      = 1;

    filterLeft.prepare(spec);
    filterRight.prepare(spec);
    filterLeft.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filterRight.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filterLeft.setCutoffFrequency(20000.0f);
    filterRight.setCutoffFrequency(20000.0f);
}

void PenguinLFOProcessor::applyPreset(const PresetData& preset) {
    // Write all fields before signalling the audio thread (release ordering)
    pendingLfos = preset.lfos;
    presetPending.store(true, std::memory_order_release);
}

void PenguinLFOProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer&) {
    // Apply any pending preset at block boundary (acquire ordering)
    if (presetPending.exchange(false, std::memory_order_acquire)) {
        lfos = pendingLfos;
    }

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                currentBPM = static_cast<float>(*bpm);

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    int   N = buffer.getNumSamples();

    // Hoist per-LFO phase increments (constant across the block)
    float incs[4] = {};
    for (int i = 0; i < 4; ++i) {
        if (lfos[i].enabled)
            incs[i] = lfoPhaseIncrement(lfos[i].rateIndex,
                                        currentBPM,
                                        static_cast<float>(currentSampleRate));
    }

    for (int s = 0; s < N; ++s) {
        float gainMod      = 1.0f;
        float panMod       = 0.0f;
        float filterCutoff = 20000.0f;

        for (int i = 0; i < 4; ++i) {
            if (!lfos[i].enabled) continue;
            float val = lfoAdvance(lfos[i], incs[i]); // [-1, 1]

            switch (lfos[i].target) {
                case LFOTarget::Volume:
                    gainMod *= 1.0f - lfos[i].depth * (1.0f - (val * 0.5f + 0.5f));
                    break;
                case LFOTarget::Filter:
                    // Multiple Filter LFOs: last enabled one wins
                    filterCutoff = 200.0f * std::pow(100.0f,
                        (val * 0.5f + 0.5f) * lfos[i].depth + (1.0f - lfos[i].depth));
                    filterCutoff = juce::jlimit(200.0f, 20000.0f, filterCutoff);
                    break;
                case LFOTarget::Pan:
                    panMod = juce::jlimit(-1.0f, 1.0f, panMod + val * lfos[i].depth);
                    break;
                case LFOTarget::Pitch:
                    // Implemented as amplitude modulation (ring-mod approximation).
                    // True pitch shifting requires resampling and is not implemented.
                    gainMod *= 1.0f + val * lfos[i].depth * 0.05f;
                    break;
                default:
                    jassertfalse;
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
