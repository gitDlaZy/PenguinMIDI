#include "PluginProcessor.h"
#include "PluginEditor.h"

PenguinLFOProcessor::PenguinLFOProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    auto pluginDir   = juce::File::getSpecialLocation(
                           juce::File::currentExecutableFile).getParentDirectory();
    auto presetsDir  = pluginDir.getChildFile("PenguinLFO_Presets");
    auto presetsFile = presetsDir.getChildFile("presets.json");

    if (presetsFile.existsAsFile())
        factoryPresets = loadPresetsFromFile(presetsFile.getFullPathName().toStdString());

    auto userFile   = presetsDir.getChildFile("user_presets.json");
    userPresetsFilePath = userFile.getFullPathName();
    if (userFile.existsAsFile())
        userPresets = loadPresetsFromFile(userFile.getFullPathName().toStdString());

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

    hpLeft.prepare(spec);
    hpRight.prepare(spec);
    lpLeft.prepare(spec);
    lpRight.prepare(spec);
    hpLeft.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    hpRight.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    lpLeft.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lpRight.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    hpLeft.setCutoffFrequency(filterLowCut);
    hpRight.setCutoffFrequency(filterLowCut);
    lpLeft.setCutoffFrequency(filterHighCut);
    lpRight.setCutoffFrequency(filterHighCut);

    const int bufSize    = static_cast<int>(0.05 * sampleRate) + 2;
    vibratoBaseDelay     = static_cast<int>(0.025 * sampleRate);
    vibratoL.assign(bufSize, 0.0f);
    vibratoR.assign(bufSize, 0.0f);
    vibratoWritePos = 0;
}

void PenguinLFOProcessor::applyPreset(const PresetData& preset) {
    pendingLfos          = preset.lfos;
    pendingFilterLowCut  = preset.filterLowCut;
    pendingFilterHighCut = preset.filterHighCut;
    presetPending.store(true, std::memory_order_release);
}

void PenguinLFOProcessor::updateLFOParam(int index, LFOShape shape, int rateIndex,
                                          LFOTarget target, float depth, bool enabled,
                                          float smoothing, float pitchCenter) {
    if (index < 0 || index >= 4) return;
    pendingParamUpdates[index]              = lfos[index];
    pendingParamUpdates[index].shape        = shape;
    pendingParamUpdates[index].rateIndex    = rateIndex;
    pendingParamUpdates[index].target       = target;
    pendingParamUpdates[index].depth        = depth;
    pendingParamUpdates[index].enabled      = enabled;
    pendingParamUpdates[index].smoothing    = smoothing;
    pendingParamUpdates[index].pitchCenter  = pitchCenter;
    paramUpdateMask.fetch_or(1u << index, std::memory_order_release);
}

void PenguinLFOProcessor::updateCustomWaveform(int index, const CustomWaveform& wf) {
    if (index < 0 || index >= 4) return;
    pendingParamUpdates[index]            = lfos[index];
    pendingParamUpdates[index].customWave = wf;
    paramUpdateMask.fetch_or(1u << index, std::memory_order_release);
}

void PenguinLFOProcessor::saveUserPresets() const {
    if (userPresetsFilePath.isEmpty()) return;
    savePresetsToFile(userPresets, userPresetsFilePath.toStdString());
}

void PenguinLFOProcessor::getStateInformation(juce::MemoryBlock& destData) {
    PresetData current;
    current.name          = "__state__";
    current.lfos          = lfos;
    current.filterLowCut  = filterLowCut;
    current.filterHighCut = filterHighCut;
    std::string json = serializePreset(current);
    destData.replaceAll(json.data(), json.size());
}

void PenguinLFOProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (sizeInBytes <= 0) return;
    std::string json(static_cast<const char*>(data),
                     static_cast<size_t>(sizeInBytes));
    auto presets = parsePresetsJson("{\"presets\":[" + json + "]}");
    if (!presets.empty()) {
        for (int i = 0; i < 4; ++i) {
            lfos[i].shape        = presets[0].lfos[i].shape;
            lfos[i].rateIndex    = presets[0].lfos[i].rateIndex;
            lfos[i].target       = presets[0].lfos[i].target;
            lfos[i].depth        = presets[0].lfos[i].depth;
            lfos[i].enabled      = presets[0].lfos[i].enabled;
            lfos[i].smoothing    = presets[0].lfos[i].smoothing;
            lfos[i].pitchCenter  = presets[0].lfos[i].pitchCenter;
            lfos[i].customWave   = presets[0].lfos[i].customWave;
        }
        filterLowCut  = presets[0].filterLowCut;
        filterHighCut = presets[0].filterHighCut;
    }
}

void PenguinLFOProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer&) {
    if (presetPending.exchange(false, std::memory_order_acquire)) {
        lfos          = pendingLfos;
        filterLowCut  = pendingFilterLowCut;
        filterHighCut = pendingFilterHighCut;
    }

    if (uint32_t mask = paramUpdateMask.exchange(0, std::memory_order_acquire); mask != 0) {
        for (int i = 0; i < 4; ++i)
            if (mask & (1u << i))
                lfos[i] = pendingParamUpdates[i];
    }

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                currentBPM = static_cast<float>(*bpm);

    auto* L = buffer.getWritePointer(0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;
    int   N = buffer.getNumSamples();

    float incs[4] = {};
    for (int i = 0; i < 4; ++i) {
        if (lfos[i].enabled)
            incs[i] = lfoPhaseIncrement(lfos[i].rateIndex,
                                        currentBPM,
                                        static_cast<float>(currentSampleRate));
    }

    const int bufSize = static_cast<int>(vibratoL.size());

    for (int s = 0; s < N; ++s) {
        if (bufSize > 0) {
            vibratoL[vibratoWritePos] = L[s];
            if (R) vibratoR[vibratoWritePos] = R[s];
        }

        float gainMod      = 1.0f;
        float panMod       = 0.0f;
        float filterCutoff = filterHighCut;
        float pitchMod     = 0.0f;
        bool  hasPitch     = false;

        for (int i = 0; i < 4; ++i) {
            if (!lfos[i].enabled) continue;
            float val = lfoAdvance(lfos[i], incs[i], static_cast<float>(currentSampleRate));

            switch (lfos[i].target) {
                case LFOTarget::Volume:
                    gainMod *= 1.0f - lfos[i].depth * (1.0f - (val * 0.5f + 0.5f));
                    break;
                case LFOTarget::Filter: {
                    float t = val * 0.5f + 0.5f; // [-1,1] → [0,1]
                    float lo = std::max(filterLowCut, 20.0f);
                    float hi = std::max(filterHighCut, lo + 1.0f);
                    float logRange = std::log(hi / lo);
                    filterCutoff = lo * std::exp(logRange * (t * lfos[i].depth + (1.0f - lfos[i].depth) * 0.5f));
                    filterCutoff = juce::jlimit(lo, hi, filterCutoff);
                    break;
                }
                case LFOTarget::Pan:
                    panMod = juce::jlimit(-1.0f, 1.0f, panMod + val * lfos[i].depth);
                    break;
                case LFOTarget::Pitch:
                    pitchMod += (val + lfos[i].pitchCenter) * lfos[i].depth;
                    hasPitch = true;
                    break;
                default:
                    jassertfalse;
                    break;
            }
        }

        float Ls, Rs;
        if (hasPitch && bufSize > 0 && vibratoBaseDelay > 0) {
            pitchMod = juce::jlimit(-0.95f, 0.95f, pitchMod);
            float delay   = static_cast<float>(vibratoBaseDelay) * (1.0f - pitchMod);
            float readPos = static_cast<float>(vibratoWritePos) - delay;
            while (readPos < 0.0f) readPos += static_cast<float>(bufSize);
            int   i0   = static_cast<int>(readPos) % bufSize;
            int   i1   = (i0 + 1) % bufSize;
            float frac  = readPos - std::floor(readPos);
            Ls = vibratoL[i0] * (1.0f - frac) + vibratoL[i1] * frac;
            Rs = R ? (vibratoR[i0] * (1.0f - frac) + vibratoR[i1] * frac) : 0.0f;
        } else {
            Ls = L[s];
            Rs = R ? R[s] : 0.0f;
        }

        if (bufSize > 0)
            vibratoWritePos = (vibratoWritePos + 1) % bufSize;

        hpLeft.setCutoffFrequency(std::max(filterLowCut, 20.0f));
        hpRight.setCutoffFrequency(std::max(filterLowCut, 20.0f));
        lpLeft.setCutoffFrequency(filterCutoff);
        lpRight.setCutoffFrequency(filterCutoff);

        float leftGain  = std::cos((panMod + 1.0f) * juce::MathConstants<float>::pi / 4.0f);
        float rightGain = std::sin((panMod + 1.0f) * juce::MathConstants<float>::pi / 4.0f);

        float filtL = lpLeft.processSample(0, hpLeft.processSample(0, Ls));
        float filtR = R ? lpRight.processSample(0, hpRight.processSample(0, Rs)) : 0.0f;

        L[s] = filtL * gainMod * leftGain;
        if (R) R[s] = filtR * gainMod * rightGain;
    }
}

juce::AudioProcessorEditor* PenguinLFOProcessor::createEditor() {
    return new PenguinLFOEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PenguinLFOProcessor();
}
