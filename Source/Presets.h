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

struct Preset
{
    const char* name;
    int plateVoice;
    int padVoice;
    float chord, bass, strum, decay, tone, vibrato, reverb;
};

inline const Preset presets[] = {
    //  nom                                  plaque      accord        acc.  basse slide durée timbre vib  reverb
    { "Omni 1",                              Omni1,      PadAnalog,    0.50f, 0.40f, 0.80f, 1.2f, 0.45f, 0.30f, 0.30f },
    { "Omni 2",                              Omni2,      PadChoir,     0.45f, 0.35f, 0.80f, 1.6f, 0.30f, 0.45f, 0.35f },
    { "Harpe",                               Harp,       PadStrings,   0.35f, 0.30f, 0.85f, 2.5f, 0.45f, 0.10f, 0.40f },
    { "Guitare",                             Guitar,     PadOrgan,     0.30f, 0.45f, 0.85f, 2.0f, 0.55f, 0.05f, 0.25f },
    { "Banjo",                               Banjo,      PadAccordion, 0.35f, 0.45f, 0.80f, 1.0f, 0.70f, 0.05f, 0.20f },
    { "Vibes",                               Vibes,      PadEPiano,    0.35f, 0.35f, 0.80f, 2.8f, 0.40f, 0.50f, 0.35f },
    { "Piano \xc3\xa9lec",                   EPiano,     PadEPiano,    0.40f, 0.40f, 0.80f, 2.2f, 0.45f, 0.20f, 0.30f },
    { "Cristal",                             Crystal,    PadChoir,     0.35f, 0.25f, 0.75f, 3.0f, 0.50f, 0.20f, 0.55f },
    { "Synth trance",                        SynthPluck, PadStrings,   0.45f, 0.45f, 0.80f, 0.8f, 0.60f, 0.10f, 0.45f },
    { "Bo\xc3\xaete \xc3\xa0 musique",       MusicBox,   PadChoir,     0.20f, 0.00f, 0.85f, 1.6f, 0.55f, 0.15f, 0.40f },
};

constexpr int numPresets = (int) (sizeof (presets) / sizeof (presets[0]));

inline Params paramsForPreset (int i, Params base = {})
{
    const auto& pr = presets[juce::jlimit (0, numPresets - 1, i)];
    base.plateVoice = pr.plateVoice;
    base.padVoice   = pr.padVoice;
    base.chordLevel = pr.chord;
    base.bassLevel  = pr.bass;
    base.strumLevel = pr.strum;
    base.strumDecay = pr.decay;
    base.strumTone  = pr.tone;
    base.vibrato    = pr.vibrato;
    base.reverb     = pr.reverb;
    return base;
}

} // namespace chordharp
