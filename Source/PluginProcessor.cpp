#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

ChordHarpProcessor::ChordHarpProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pChord  = apvts.getRawParameterValue ("chord");
    pBass   = apvts.getRawParameterValue ("bass");
    pStrum  = apvts.getRawParameterValue ("strum");
    pDecay  = apvts.getRawParameterValue ("decay");
    pTone   = apvts.getRawParameterValue ("tone");
    pVib    = apvts.getRawParameterValue ("vibrato");
    pReverb = apvts.getRawParameterValue ("reverb");
    pMaster = apvts.getRawParameterValue ("master");
    pHold   = apvts.getRawParameterValue ("hold");
    pPlate  = apvts.getRawParameterValue ("plateVoice");
    pPad    = apvts.getRawParameterValue ("padVoice");

    uiMidi.reset (44100.0);
}

juce::AudioProcessorValueTreeState::ParameterLayout ChordHarpProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto unit = juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f);
    const auto pct  = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
    const auto secs = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (v, 2) + " s"; });

    const auto& d = chordharp::presets[0];

    juce::StringArray plateNames, padNames;
    for (auto* n : chordharp::plateVoiceNames) plateNames.add (juce::String::fromUTF8 (n));
    for (auto* n : chordharp::padVoiceNames)   padNames.add (juce::String::fromUTF8 (n));

    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "plateVoice", 1 }, "Son slide", plateNames, d.plateVoice));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "padVoice", 1 }, "Son accord", padNames, d.padVoice));
    layout.add (std::make_unique<P> (juce::ParameterID { "chord", 1 },   "Accord",  unit, d.chord, pct));
    layout.add (std::make_unique<P> (juce::ParameterID { "bass", 1 },    "Basse",   unit, d.bass, pct));
    layout.add (std::make_unique<P> (juce::ParameterID { "strum", 1 },   "Slide",   unit, d.strum, pct));
    layout.add (std::make_unique<P> (juce::ParameterID { "decay", 1 },   "Duree",
                                     juce::NormalisableRange<float> (0.1f, 5.0f, 0.01f, 0.5f), d.decay, secs));
    layout.add (std::make_unique<P> (juce::ParameterID { "tone", 1 },    "Timbre",  unit, d.tone, pct));
    layout.add (std::make_unique<P> (juce::ParameterID { "vibrato", 1 }, "Vibrato", unit, d.vibrato, pct));
    layout.add (std::make_unique<P> (juce::ParameterID { "reverb", 1 },  "Reverb",  unit, d.reverb, pct));
    layout.add (std::make_unique<P> (juce::ParameterID { "master", 1 },  "Volume",  unit, 0.8f, pct));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "hold", 1 }, "Hold", false));

    return layout;
}

void ChordHarpProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    uiMidi.reset (sampleRate);
    uiBuffer.ensureSize (2048);
    combined.ensureSize (4096);
}

bool ChordHarpProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void ChordHarpProcessor::sendUiMidi (juce::MidiMessage message)
{
    message.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    uiMidi.addMessageToQueue (message);
}

void ChordHarpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    uiBuffer.clear();
    uiMidi.removeNextBlockOfMessages (uiBuffer, n);

    combined.clear();
    combined.addEvents (midi, 0, n, 0);
    combined.addEvents (uiBuffer, 0, n, 0);

    chordharp::Params p;
    p.chordLevel = pChord->load();
    p.bassLevel  = pBass->load();
    p.strumLevel = pStrum->load();
    p.strumDecay = pDecay->load();
    p.strumTone  = pTone->load();
    p.vibrato    = pVib->load();
    p.reverb     = pReverb->load();
    p.master     = pMaster->load();
    p.hold       = pHold->load() > 0.5f;
    p.plateVoice = juce::roundToInt (pPlate->load());
    p.padVoice   = juce::roundToInt (pPad->load());

    engine.process (buffer, combined, p);
    midi.clear();
}

//==============================================================================
int ChordHarpProcessor::getNumPrograms()
{
    return chordharp::numPresets;
}

const juce::String ChordHarpProcessor::getProgramName (int index)
{
    return juce::String::fromUTF8 (chordharp::presets[juce::jlimit (0, chordharp::numPresets - 1, index)].name);
}

void ChordHarpProcessor::setCurrentProgram (int index)
{
    applyPreset (index);
}

void ChordHarpProcessor::applyPreset (int index)
{
    index = juce::jlimit (0, chordharp::numPresets - 1, index);
    const auto& pr = chordharp::presets[index];

    auto set = [this] (const char* id, float value)
    {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };

    set ("plateVoice", (float) pr.plateVoice);
    set ("padVoice",   (float) pr.padVoice);
    set ("chord",      pr.chord);
    set ("bass",       pr.bass);
    set ("strum",      pr.strum);
    set ("decay",      pr.decay);
    set ("tone",       pr.tone);
    set ("vibrato",    pr.vibrato);
    set ("reverb",     pr.reverb);

    currentPreset = index;
}

void ChordHarpProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ChordHarpProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset = (int) state.getProperty ("preset", 0);
            apvts.replaceState (state);
        }
}

juce::AudioProcessorEditor* ChordHarpProcessor::createEditor()
{
    return new ChordHarpEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChordHarpProcessor();
}
