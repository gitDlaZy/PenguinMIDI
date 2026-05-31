#include "PluginProcessor.h"
#include "PluginEditor.h"

PenguinMIDIProcessor::PenguinMIDIProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{}

void PenguinMIDIProcessor::prepareToPlay(double sampleRate, int)
{
    midiCollector.reset(sampleRate);
}

void PenguinMIDIProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi)
{
    audio.clear();
    midiCollector.removeNextBlockOfMessages(midi, audio.getNumSamples());

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                lastBpm.store(*bpm);
}

juce::AudioProcessorEditor* PenguinMIDIProcessor::createEditor()
{
    return new PenguinMIDIEditor(*this);
}

void PenguinMIDIProcessor::scheduleMidiNote(int channel, int note, int velocity, int durationMs)
{
    auto t = juce::Time::getMillisecondCounterHiRes() * 0.001;
    midiCollector.addMessageToQueue(
        juce::MidiMessage::noteOn(channel, note, (juce::uint8)velocity).withTimeStamp(t));

    juce::Timer::callAfterDelay(durationMs, [this, channel, note] {
        midiCollector.addMessageToQueue(
            juce::MidiMessage::noteOff(channel, note).withTimeStamp(
                juce::Time::getMillisecondCounterHiRes() * 0.001));
    });
}

juce::File PenguinMIDIProcessor::getPatternsFile() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("PenguinSuite");
    dir.createDirectory();
    return dir.getChildFile("midi-patterns.json");
}

void PenguinMIDIProcessor::savePatterns(const juce::String& jsonString)
{
    getPatternsFile().replaceWithText(jsonString);
}

juce::String PenguinMIDIProcessor::loadPatternsJson()
{
    auto f = getPatternsFile();
    if (f.existsAsFile())
        return f.loadFileAsString();
    return "[]";
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PenguinMIDIProcessor();
}
