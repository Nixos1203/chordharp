#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

namespace chordharp
{

enum ChordType { Major = 0, Minor = 1, Seventh = 2, NumChordTypes = 3 };

/** Sons de la plaque (slide). */
enum PlateVoice
{
    Omni1 = 0, Omni2, Harp, Guitar, Banjo, Vibes, EPiano, Crystal, SynthPluck, MusicBox,
    NumPlateVoices
};

/** Sons de l'accord tenu. */
enum PadVoice
{
    PadAnalog = 0, PadOrgan, PadStrings, PadAccordion, PadChoir, PadEPiano,
    NumPadVoices
};

struct Params
{
    float chordLevel = 0.5f;   // volume de l'accord tenu
    float bassLevel  = 0.4f;   // volume de la basse auto
    float strumLevel = 0.8f;   // volume de la plaque (slide)
    float strumDecay = 1.2f;   // durée des notes de la plaque, en secondes
    float strumTone  = 0.4f;   // 0 = doux, 1 = brillant
    float vibrato    = 0.3f;
    float reverb     = 0.3f;
    float master     = 0.8f;
    bool  hold       = false;  // garde l'accord après relâchement de la touche
    int   plateVoice = Omni1;
    int   padVoice   = PadAnalog;

    // Couche (2e son superposé au slide, comme le "sub voice" de l'OM-108)
    int   layerVoice  = -1;     // -1 = aucune
    float layerLevel  = 0.5f;
    int   layerOctave = 0;      // -1 .. +2

    float attack      = 0.0f;   // attaque des notes du slide (s)

    // Bus du slide : filtre résonant + LFO, saturation, delay ping-pong
    float cutoff      = 20000.0f;
    float resonance   = 0.0f;
    float lfoRate     = 0.5f;   // Hz
    float lfoDepth    = 0.0f;   // 0..1 (= +/- 3 octaves)
    float drive       = 0.0f;
    int   delayDiv    = 2;      // index dans delayBeats
    float delayFeedback = 0.0f;
    float delayMix    = 0.0f;

    // Global
    float chorusMix   = 0.3f;
    float roomSize    = 0.8f;
    double bpm        = 120.0;
};

/** Divisions du delay, en temps (noires). */
inline constexpr double delayBeats[] = { 0.25, 0.5, 0.75, 1.0, 1.5, 2.0 };
inline constexpr int numDelayDivs = 6;

/*  Moteur sonore, indépendant du format plugin.

    Mapping MIDI (numéros de notes MIDI ; notation FL Studio entre parenthèses) :
      24-35  (C2-B2)  -> accord majeur  (la note = fondamentale)
      36-47  (C3-B3)  -> accord mineur
      48-59  (C4-B4)  -> accord de 7e
      60-72  (C5-C6)  -> les 13 positions de la plaque, de la plus grave à la plus aiguë
      CC1  (mod wheel) -> balayer la plaque
      CC74 (timbre)    -> brillance de la plaque, en direct (= axe horizontal du pad)
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
    std::atomic<int>   uiRoot { 0 };
    std::atomic<int>   uiType { 0 };
    std::atomic<bool>  uiHeld { false };
    std::atomic<float> uiBright { 0.5f };
    std::array<std::atomic<float>, numPlate> uiPlateFlash;

    // Gains de calibrage par modèle (publics pour l'outil de test)
    static float plateModelGain (int model) noexcept;
    static float padModelGain (int model) noexcept;

private:
    static constexpr int ksSize = 4096;
    static constexpr int ksMask = ksSize - 1;

    struct Pluck
    {
        bool active = false;
        int model = Omni1;
        double phase = 0, inc = 0, mphase = 0, minc = 0;
        float freq = 0, env = 0, decay = 0, envFast = 0, decayFast = 0;
        float lp = 0, lp2 = 0, att = 0, attStep = 1, gainL = 0, gainR = 0, vel = 0;
        // Karplus-Strong (harpe / guitare / banjo)
        float* ks = nullptr;
        int ksW = 0, ksExcite = 0, ksInt = 1;
        float ksFrac = 0, ksA = 0.5f, ksExA = 0.5f, ksGain = 0, exLp = 0;
        uint32_t age = 0;
    };

    struct Pad
    {
        bool active = false;
        int model = PadAnalog;
        double ph[3] = { 0, 0, 0 };
        double inc = 0;
        float lp1 = 0, lp2 = 0, fmEnv = 0, fmDecay = 0;
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
    void startPluck (int model, int note, float velocity, float level, float pan, float detuneCents);
    void processSlideBus (int numSamples);
    void render (int start, int end);
    void applyEffects (int numSamples);

    float renderPluck (Pluck& v, double vib, float lfo01, float tEff) noexcept;
    float renderPad (Pad& v, double vib) noexcept;

    double sr = 44100.0;
    int maxBlock = 512;
    Params p;
    juce::AudioBuffer<float> mix, slide;

    std::array<Pluck, 32> plucks;
    std::array<Pad, 16>   pads;
    std::array<Bass, 3>   basses;
    std::vector<float>    ksMemory;
    juce::Random          rng;

    std::vector<int> held;
    int root = 0, type = Major;
    int lastWheel = -1;
    float plateBright = 0.5f;
    uint32_t ageCounter = 0;
    double lfoPhase = 0;

    juce::dsp::StateVariableTPTFilter<float> filter;
    double filterLfoPhase = 0.0;
    int filterCounter = 0;

    std::vector<float> delayL, delayR;
    int delayWrite = 0;
    float delayFbLp = 0.0f;

    juce::dsp::Chorus<float> chorus;
    juce::dsp::Reverb reverb;
    float lastReverb = -1.0f, lastRoom = -1.0f;
    bool  lastHold = false;
};

} // namespace chordharp
