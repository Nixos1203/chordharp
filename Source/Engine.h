#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

namespace chordharp
{

enum ChordType { Major = 0, Minor = 1, Seventh = 2, NumChordTypes = 3 };

struct Params
{
    float chordLevel = 0.5f;   // volume de l'accord tenu (nappe)
    float bassLevel  = 0.4f;   // volume de la basse auto
    float strumLevel = 0.8f;   // volume de la plaque (slide)
    float strumDecay = 1.2f;   // durée des notes de la plaque, en secondes
    float strumTone  = 0.4f;   // 0 = doux, 1 = brillant
    float vibrato    = 0.3f;
    float reverb     = 0.3f;
    float master     = 0.8f;
    bool  hold       = false;  // garde l'accord après relâchement de la touche
};

/*  Moteur sonore, indépendant du format plugin.

    Mapping MIDI (numéros de notes MIDI ; notation FL Studio entre parenthèses) :
      24-35  (C2-B2)  -> accord majeur  (la note = fondamentale)
      36-47  (C3-B3)  -> accord mineur
      48-59  (C4-B4)  -> accord de 7e
      60-72  (C5-C6)  -> les 13 cases de la plaque (slide), de la plus grave à la plus aiguë
      CC1 (mod wheel) -> balayer la plaque
*/
class Engine
{
public:
    static constexpr int numPlate      = 13;
    static constexpr int chordFirstKey = 24;
    static constexpr int plateFirstKey = 60;

    static int chordKeyFor (int pitchClass, int type) noexcept { return chordFirstKey + type * 12 + pitchClass; }
    static int typeForKey (int key) noexcept;
    static int chordIntervals (int type, int* out) noexcept;
    static int plateNoteFor (int root, int type, int index) noexcept;

    Engine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi, const Params& params);

    // État lu par l'interface (thread-safe)
    std::atomic<int>  uiRoot { 0 };
    std::atomic<int>  uiType { 0 };
    std::atomic<bool> uiHeld { false };
    std::array<std::atomic<float>, numPlate> uiPlateFlash;

private:
    struct Pluck
    {
        bool active = false;
        double phase = 0, inc = 0;
        float freq = 0, env = 0, decay = 0, lp = 0, att = 0, gainL = 0, gainR = 0, vel = 0;
        uint32_t age = 0;
    };

    struct Pad
    {
        bool active = false;
        double ph1 = 0, ph2 = 0, inc = 0;
        float lp1 = 0, lp2 = 0;
        uint32_t age = 0;
        juce::ADSR adsr;
    };

    struct Bass
    {
        bool active = false;
        double phase = 0, inc = 0;
        uint32_t age = 0;
        juce::ADSR adsr;
    };

    void handle (const juce::MidiMessage&);
    void startChord (int root, int type);
    void releaseChord();
    void triggerPlate (int index, float velocity);
    void render (int start, int end);
    void applyEffects (int numSamples);

    double sr = 44100.0;
    int maxBlock = 512;
    Params p;
    juce::AudioBuffer<float> mix;

    std::array<Pluck, 32> plucks;
    std::array<Pad, 12>   pads;
    std::array<Bass, 3>   basses;

    std::vector<int> held;
    int root = 0, type = Major;
    int lastWheel = -1;
    uint32_t ageCounter = 0;
    double lfoPhase = 0;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    float lastReverb = -1.0f;
    bool  lastHold = false;
};

} // namespace chordharp
