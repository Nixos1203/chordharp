#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "Engine.h"

class ChordHarpProcessor : public juce::AudioProcessor
{
public:
    ChordHarpProcessor();
    ~ChordHarpProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    /** Charge un preset (à appeler depuis le thread de l'interface). */
    void applyPreset (int index);
    std::atomic<int> currentPreset { 0 };

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Appelé par l'interface (clic sur un accord, slide sur la plaque). */
    void sendUiMidi (juce::MidiMessage message);

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    chordharp::Engine engine;
    juce::AudioProcessorValueTreeState apvts;

private:
    juce::MidiMessageCollector uiMidi;
    juce::MidiBuffer uiBuffer, combined;

    std::atomic<float>* pChord  = nullptr;
    std::atomic<float>* pBass   = nullptr;
    std::atomic<float>* pStrum  = nullptr;
    std::atomic<float>* pDecay  = nullptr;
    std::atomic<float>* pTone   = nullptr;
    std::atomic<float>* pVib    = nullptr;
    std::atomic<float>* pReverb = nullptr;
    std::atomic<float>* pMaster = nullptr;
    std::atomic<float>* pHold   = nullptr;
    std::atomic<float>* pPlate  = nullptr;
    std::atomic<float>* pPad    = nullptr;
    std::atomic<float>* pLayer = nullptr, * pLayerLevel = nullptr, * pLayerOct = nullptr, * pAttack = nullptr;
    std::atomic<float>* pCutoff = nullptr, * pReso = nullptr, * pLfoRate = nullptr, * pLfoDepth = nullptr;
    std::atomic<float>* pDrive = nullptr, * pChorus = nullptr, * pDelayDiv = nullptr, * pDelayFb = nullptr;
    std::atomic<float>* pDelayMix = nullptr, * pRoom = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordHarpProcessor)
};
