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
    double getTailLengthSeconds() const override { return 5.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordHarpProcessor)
};
