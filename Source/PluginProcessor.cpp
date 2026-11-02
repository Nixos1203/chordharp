#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    juce::StringArray utf8Array (const char* const* names, int count)
    {
        juce::StringArray a;
        for (int i = 0; i < count; ++i)
            a.add (juce::String::fromUTF8 (names[i]));
        return a;
    }
}

ChordHarpProcessor::ChordHarpProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };

    pChord      = get ("chord");
    pBass       = get ("bass");
    pStrum      = get ("strum");
    pDecay      = get ("decay");
    pTone       = get ("tone");
    pVib        = get ("vibrato");
    pReverb     = get ("reverb");
    pMaster     = get ("master");
    pHold       = get ("hold");
    pPlate      = get ("plateVoice");
    pPad        = get ("padVoice");
    pLayer      = get ("layerVoice");
    pLayerLevel = get ("layerLevel");
    pLayerOct   = get ("layerOct");
    pAttack     = get ("attack");
    pCutoff     = get ("cutoff");
    pReso       = get ("reso");
    pLfoRate    = get ("lfoRate");
    pLfoDepth   = get ("lfoDepth");
    pDrive      = get ("drive");
    pChorus     = get ("chorus");
    pDelayDiv   = get ("delayDiv");
    pDelayFb    = get ("delayFb");
    pDelayMix   = get ("delayMix");
    pRoom       = get ("roomSize");

    uiMidi.reset (44100.0);
}

juce::AudioProcessorValueTreeState::ParameterLayout ChordHarpProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using ID = juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto unit = juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f);
    const auto pct  = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
    const auto secs = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (v, 2) + " s"; });
    const auto hz = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v < 1000.0f ? juce::String (juce::roundToInt (v)) + " Hz"
                                               : juce::String (v / 1000.0f, 1) + " kHz"; });
    const auto lfoHz = juce::AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return juce::String (v, 2) + " Hz"; });

    auto cutoffRange = juce::NormalisableRange<float> (100.0f, 20000.0f, 1.0f);
    cutoffRange.setSkewForCentre (1500.0f);
    auto lfoRange = juce::NormalisableRange<float> (0.02f, 10.0f, 0.01f);
    lfoRange.setSkewForCentre (0.6f);
    auto attackRange = juce::NormalisableRange<float> (0.0f, 2.0f, 0.001f);
    attackRange.setSkewForCentre (0.25f);

    const auto& d = chordharp::presets[0];

    juce::StringArray layerNames { juce::String::fromUTF8 ("Aucune") };
    layerNames.addArray (utf8Array (chordharp::plateVoiceNames, chordharp::NumPlateVoices));

    // Sources
    layout.add (std::make_unique<C> (ID { "plateVoice", 1 }, "Son slide",
                                     utf8Array (chordharp::plateVoiceNames, chordharp::NumPlateVoices), d.plate));
    layout.add (std::make_unique<C> (ID { "layerVoice", 1 }, "Couche", layerNames, d.layer + 1));
    layout.add (std::make_unique<C> (ID { "layerOct", 1 }, "Octave couche",
                                     utf8Array (chordharp::layerOctaveNames, 4), d.layerOct + 1));
    layout.add (std::make_unique<C> (ID { "padVoice", 1 }, "Son accord",
                                     utf8Array (chordharp::padVoiceNames, chordharp::NumPadVoices), d.pad));

    // Niveaux et jeu
    layout.add (std::make_unique<P> (ID { "chord", 1 },      "Accord",       unit, d.chord, pct));
    layout.add (std::make_unique<P> (ID { "bass", 1 },       "Basse",        unit, d.bass, pct));
    layout.add (std::make_unique<P> (ID { "strum", 1 },      "Slide",        unit, d.strum, pct));
    layout.add (std::make_unique<P> (ID { "layerLevel", 1 }, "Niveau couche", unit, d.layerLevel, pct));
    layout.add (std::make_unique<P> (ID { "attack", 1 },     "Attaque",      attackRange, d.attack, secs));
    layout.add (std::make_unique<P> (ID { "decay", 1 },      "Duree",
                                     juce::NormalisableRange<float> (0.1f, 6.0f, 0.01f, 0.5f), d.decay, secs));
    layout.add (std::make_unique<P> (ID { "tone", 1 },       "Timbre",       unit, d.tone, pct));
    layout.add (std::make_unique<P> (ID { "vibrato", 1 },    "Vibrato",      unit, d.vibrato, pct));

    // Effets
    layout.add (std::make_unique<P> (ID { "cutoff", 1 },     "Filtre",       cutoffRange, d.cutoff, hz));
    layout.add (std::make_unique<P> (ID { "reso", 1 },       "Resonance",    unit, d.reso, pct));
    layout.add (std::make_unique<P> (ID { "lfoRate", 1 },    "LFO vitesse",  lfoRange, d.lfoRate, lfoHz));
    layout.add (std::make_unique<P> (ID { "lfoDepth", 1 },   "LFO profondeur", unit, d.lfoDepth, pct));
    layout.add (std::make_unique<P> (ID { "drive", 1 },      "Drive",        unit, d.drive, pct));
    layout.add (std::make_unique<P> (ID { "chorus", 1 },     "Chorus",       unit, d.chorus, pct));
    layout.add (std::make_unique<C> (ID { "delayDiv", 1 },   "Delay",
                                     utf8Array (chordharp::delayDivNames, chordharp::numDelayDivs), d.delayDiv));
    layout.add (std::make_unique<P> (ID { "delayFb", 1 },    "Delay feedback", unit, d.delayFb, pct));
    layout.add (std::make_unique<P> (ID { "delayMix", 1 },   "Delay mix",    unit, d.delayMix, pct));
    layout.add (std::make_unique<P> (ID { "reverb", 1 },     "Reverb",       unit, d.reverb, pct));
    layout.add (std::make_unique<P> (ID { "roomSize", 1 },   "Taille reverb",
                                     juce::NormalisableRange<float> (0.3f, 0.97f, 0.001f), d.room, pct));
    layout.add (std::make_unique<P> (ID { "master", 1 },     "Volume",       unit, 0.8f, pct));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "hold", 1 }, "Hold", false));

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
    p.chordLevel    = pChord->load();
    p.bassLevel     = pBass->load();
    p.strumLevel    = pStrum->load();
    p.strumDecay    = pDecay->load();
    p.strumTone     = pTone->load();
    p.vibrato       = pVib->load();
    p.reverb        = pReverb->load();
    p.master        = pMaster->load();
    p.hold          = pHold->load() > 0.5f;
    p.plateVoice    = juce::roundToInt (pPlate->load());
    p.padVoice      = juce::roundToInt (pPad->load());
    p.layerVoice    = juce::roundToInt (pLayer->load()) - 1;
    p.layerLevel    = pLayerLevel->load();
    p.layerOctave   = juce::roundToInt (pLayerOct->load()) - 1;
    p.attack        = pAttack->load();
    p.cutoff        = pCutoff->load();
    p.resonance     = pReso->load();
    p.lfoRate       = pLfoRate->load();
    p.lfoDepth      = pLfoDepth->load();
    p.drive         = pDrive->load();
    p.chorusMix     = pChorus->load();
    p.delayDiv      = juce::roundToInt (pDelayDiv->load());
    p.delayFeedback = pDelayFb->load();
    p.delayMix      = pDelayMix->load();
    p.roomSize      = pRoom->load();

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                p.bpm = *bpm;

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

    set ("plateVoice", (float) pr.plate);
    set ("padVoice",   (float) pr.pad);
    set ("layerVoice", (float) (pr.layer + 1));
    set ("layerLevel", pr.layerLevel);
    set ("layerOct",   (float) (pr.layerOct + 1));
    set ("chord",      pr.chord);
    set ("bass",       pr.bass);
    set ("strum",      pr.strum);
    set ("attack",     pr.attack);
    set ("decay",      pr.decay);
    set ("tone",       pr.tone);
    set ("vibrato",    pr.vibrato);
    set ("cutoff",     pr.cutoff);
    set ("reso",       pr.reso);
    set ("lfoRate",    pr.lfoRate);
    set ("lfoDepth",   pr.lfoDepth);
    set ("drive",      pr.drive);
    set ("chorus",     pr.chorus);
    set ("delayDiv",   (float) pr.delayDiv);
    set ("delayFb",    pr.delayFb);
    set ("delayMix",   pr.delayMix);
    set ("reverb",     pr.reverb);
    set ("roomSize",   pr.room);

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
