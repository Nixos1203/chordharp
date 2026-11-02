#pragma once

#include "Engine.h"

namespace chordharp
{

// Noms en UTF-8 (échappés pour compiler partout)
inline const char* const plateVoiceNames[NumPlateVoices] = {
    "Omni 1", "Omni 2", "Harpe", "Guitare", "Banjo", "Vibraphone",
    "Piano \xc3\xa9lectrique", "Cristal", "Synth pluck", "Bo\xc3\xaete \xc3\xa0 musique"
};

inline const char* const padVoiceNames[NumPadVoices] = {
    "Analog", "Orgue", "Cordes", "Accord\xc3\xa9on", "Ch\xc5\x93ur", "Piano \xc3\xa9lectrique"
};

inline const char* const delayDivNames[numDelayDivs] = {
    "1/16", "1/8", "1/8 point\xc3\xa9" "e", "1/4", "1/4 point\xc3\xa9" "e", "1/2"
};

inline const char* const layerOctaveNames[4] = { "-1 oct", "0", "+1 oct", "+2 oct" };

constexpr int NoLayer = -1;
enum DelayDiv { D16 = 0, D8, D8d, D4, D4d, D2 };

struct Preset
{
    const char* name;
    const char* category;
    int   plate, pad, layer;
    float layerLevel; int layerOct;
    float chord, bass, strum;
    float attack, decay, tone, vibrato;
    float cutoff, reso, lfoRate, lfoDepth;
    float drive, chorus;
    int   delayDiv; float delayFb, delayMix;
    float reverb, room;
};


inline const Preset presets[] = {
//   nom                                    catégorie                 plaque      accord        couche      niv  oct   acc   bas   slide  att   durée timb  vib    cutoff  reso  lfoHz lfoP   drive chor  delay fb    mix    rev   taille
// --- Classique -----------------------------------------------------------------------------------------------------------------------------------------------------------
   { "Omni 1",                              "CLASSIQUE",               Omni1,      PadAnalog,    NoLayer,    0.0f, 0,  0.50f, 0.40f, 0.80f, 0.00f, 1.2f, 0.45f, 0.30f, 20000, 0.0f, 0.5f, 0.0f,  0.0f, 0.30f, D8,  0.0f, 0.0f,  0.30f, 0.80f },
   { "Omni 2",                              "CLASSIQUE",               Omni2,      PadChoir,     NoLayer,    0.0f, 0,  0.45f, 0.35f, 0.80f, 0.00f, 1.6f, 0.30f, 0.45f, 20000, 0.0f, 0.5f, 0.0f,  0.0f, 0.40f, D8,  0.0f, 0.0f,  0.35f, 0.80f },
   { "Harpe de concert",                    "CLASSIQUE",               Harp,       PadStrings,   Crystal,    0.20f, 1, 0.35f, 0.30f, 0.85f, 0.00f, 2.6f, 0.45f, 0.10f, 20000, 0.0f, 0.5f, 0.0f,  0.0f, 0.25f, D8,  0.0f, 0.0f,  0.45f, 0.85f },
   { "Folk 12 cordes",                      "CLASSIQUE",               Guitar,     PadOrgan,     Guitar,     0.55f, 1, 0.25f, 0.45f, 0.80f, 0.00f, 2.0f, 0.55f, 0.05f, 20000, 0.0f, 0.5f, 0.0f,  0.0f, 0.35f, D8,  0.0f, 0.0f,  0.25f, 0.70f },
// --- Cinéma --------------------------------------------------------------------------------------------------------------------------------------------------------------
   { "Cath\xc3\xa9" "drale de verre",       "CIN\xc3\x89MA",           Crystal,    PadChoir,     MusicBox,   0.35f, 1, 0.40f, 0.25f, 0.75f, 0.00f, 4.0f, 0.50f, 0.20f, 12000, 0.1f, 0.5f, 0.0f,  0.0f, 0.35f, D4d, 0.45f, 0.30f, 0.65f, 0.95f },
   { "Harpe fant\xc3\xb4me",                "CIN\xc3\x89MA",           Harp,       PadStrings,   Omni2,      0.40f, 0, 0.45f, 0.30f, 0.85f, 0.08f, 3.0f, 0.40f, 0.20f,  3500, 0.2f, 0.15f, 0.25f, 0.0f, 0.35f, D8d, 0.40f, 0.25f, 0.60f, 0.92f },
   { "Interstellaire",                      "CIN\xc3\x89MA",           EPiano,     PadChoir,     Crystal,    0.30f, 1, 0.40f, 0.30f, 0.65f, 0.02f, 3.5f, 0.45f, 0.25f, 20000, 0.0f, 0.5f, 0.0f,  0.0f, 0.45f, D4,  0.55f, 0.30f, 0.70f, 0.95f },
   { "Nappe d'aube",                        "CIN\xc3\x89MA",           Omni2,      PadStrings,   Vibes,      0.30f, 0, 0.50f, 0.35f, 0.85f, 0.45f, 3.0f, 0.40f, 0.30f,  6000, 0.1f, 0.10f, 0.30f, 0.0f, 0.40f, D2,  0.50f, 0.25f, 0.70f, 0.90f },
// --- Trance --------------------------------------------------------------------------------------------------------------------------------------------------------------
   { "Supersaw pluck",                      "TRANCE",                  SynthPluck, PadStrings,   SynthPluck, 0.45f, 1, 0.45f, 0.45f, 0.80f, 0.00f, 0.7f, 0.65f, 0.05f,  9000, 0.25f, 0.5f, 0.0f, 0.25f, 0.40f, D8d, 0.45f, 0.30f, 0.45f, 0.85f },
   { "Euphoria",                            "TRANCE",                  SynthPluck, PadStrings,   Crystal,    0.30f, 1, 0.50f, 0.40f, 0.80f, 0.00f, 1.0f, 0.55f, 0.10f, 20000, 0.0f, 0.5f, 0.0f, 0.15f, 0.45f, D8d, 0.50f, 0.35f, 0.55f, 0.90f },
   { "Acid harp",                           "TRANCE",                  SynthPluck, PadAnalog,    NoLayer,    0.0f, 0,  0.30f, 0.40f, 0.65f, 0.00f, 0.6f, 0.70f, 0.05f,  1200, 0.70f, 0.5f, 0.50f, 0.50f, 0.20f, D16, 0.35f, 0.20f, 0.25f, 0.70f },
   { "Hard trance stab",                    "TRANCE",                  Omni1,      PadAnalog,    SynthPluck, 0.60f, 0, 0.50f, 0.50f, 0.65f, 0.00f, 0.5f, 0.80f, 0.05f,  7000, 0.35f, 0.5f, 0.0f, 0.60f, 0.30f, D8,  0.30f, 0.20f, 0.30f, 0.75f },
// --- Rétro ---------------------------------------------------------------------------------------------------------------------------------------------------------------
   { "Italo 84",                            "R\xc3\x89TRO",            Omni1,      PadAnalog,    Vibes,      0.30f, 1, 0.45f, 0.45f, 0.80f, 0.00f, 1.4f, 0.55f, 0.35f, 20000, 0.0f, 0.5f, 0.0f, 0.20f, 0.55f, D8,  0.30f, 0.20f, 0.30f, 0.75f },
   { "VHS",                                 "R\xc3\x89TRO",            Omni2,      PadChoir,     EPiano,     0.35f, 0, 0.40f, 0.35f, 0.80f, 0.00f, 1.8f, 0.40f, 0.70f,  4000, 0.1f, 0.30f, 0.15f, 0.30f, 0.60f, D4,  0.30f, 0.15f, 0.35f, 0.80f },
   { "Arcade",                              "R\xc3\x89TRO",            MusicBox,   PadOrgan,     Omni1,      0.40f, 0, 0.30f, 0.40f, 0.80f, 0.00f, 0.6f, 0.90f, 0.10f,  8000, 0.2f, 0.5f, 0.0f, 0.20f, 0.20f, D16, 0.35f, 0.25f, 0.20f, 0.60f },
   { "Bal musette",                         "R\xc3\x89TRO",            Banjo,      PadAccordion, NoLayer,    0.0f, 0,  0.40f, 0.45f, 0.72f, 0.00f, 1.0f, 0.70f, 0.05f, 20000, 0.0f, 0.5f, 0.0f,  0.0f, 0.20f, D8,  0.0f, 0.0f,  0.20f, 0.65f },
// --- Ambient -------------------------------------------------------------------------------------------------------------------------------------------------------------
   { "Brume",                               "AMBIENT",                 Vibes,      PadChoir,     Crystal,    0.25f, 1, 0.45f, 0.25f, 0.80f, 0.30f, 4.5f, 0.40f, 0.40f,  3000, 0.1f, 0.07f, 0.30f, 0.0f, 0.50f, D2,  0.60f, 0.35f, 0.80f, 0.95f },
   { "Bo\xc3\xaete \xc3\xa0 musique hant\xc3\xa9" "e", "AMBIENT",      MusicBox,   PadChoir,     Crystal,    0.30f, 0, 0.25f, 0.00f, 0.85f, 0.00f, 2.0f, 0.55f, 0.60f,  7000, 0.1f, 0.5f, 0.0f,  0.0f, 0.40f, D4d, 0.50f, 0.30f, 0.65f, 0.90f },
   { "Oc\xc3\xa9" "an",                     "AMBIENT",                 Harp,       PadStrings,   Omni2,      0.35f, -1, 0.40f, 0.30f, 0.75f, 0.15f, 3.0f, 0.45f, 0.20f,  2500, 0.2f, 0.08f, 0.45f, 0.0f, 0.40f, D4,  0.50f, 0.30f, 0.75f, 0.95f },
   { "Kalimba nocturne",                    "AMBIENT",                 Crystal,    PadEPiano,    Harp,       0.40f, 0, 0.30f, 0.30f, 0.80f, 0.00f, 2.0f, 0.40f, 0.15f, 20000, 0.0f, 0.5f, 0.0f,  0.0f, 0.30f, D8d, 0.35f, 0.20f, 0.45f, 0.85f },
};


constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

inline Params paramsForPreset (int i, Params base = {})
{
    const auto& pr = presets[juce::jlimit (0, numPresets - 1, i)];
    base.plateVoice    = pr.plate;
    base.padVoice      = pr.pad;
    base.layerVoice    = pr.layer;
    base.layerLevel    = pr.layerLevel;
    base.layerOctave   = pr.layerOct;
    base.chordLevel    = pr.chord;
    base.bassLevel     = pr.bass;
    base.strumLevel    = pr.strum;
    base.attack        = pr.attack;
    base.strumDecay    = pr.decay;
    base.strumTone     = pr.tone;
    base.vibrato       = pr.vibrato;
    base.cutoff        = pr.cutoff;
    base.resonance     = pr.reso;
    base.lfoRate       = pr.lfoRate;
    base.lfoDepth      = pr.lfoDepth;
    base.drive         = pr.drive;
    base.chorusMix     = pr.chorus;
    base.delayDiv      = pr.delayDiv;
    base.delayFeedback = pr.delayFb;
    base.delayMix      = pr.delayMix;
    base.reverb        = pr.reverb;
    base.roomSize      = pr.room;
    return base;
}

} // namespace chordharp
