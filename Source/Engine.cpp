#include "Engine.h"

namespace chordharp
{

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    inline double polyBlep (double t, double dt) noexcept
    {
        if (t < dt)        { t /= dt;               return t + t - t * t - 1.0; }
        if (t > 1.0 - dt)  { t = (t - 1.0) / dt;    return t * t + t + t + 1.0; }
        return 0.0;
    }

    inline float sawBlep (double t, double dt) noexcept
    {
        return (float) (2.0 * t - 1.0 - polyBlep (t, dt));
    }

    inline float squareBlep (double t, double dt) noexcept
    {
        double v = t < 0.5 ? 1.0 : -1.0;
        v += polyBlep (t, dt);
        double t2 = t + 0.5;
        if (t2 >= 1.0) t2 -= 1.0;
        v -= polyBlep (t2, dt);
        return (float) v;
    }

    inline float pulseBlep (double t, double dt, double width) noexcept
    {
        double t2 = t + width;
        if (t2 >= 1.0) t2 -= 1.0;
        return sawBlep (t, dt) - sawBlep (t2, dt);
    }

    inline float triangle (double t) noexcept   { return 1.0f - 4.0f * std::abs ((float) t - 0.5f); }
    inline float sine (double t) noexcept       { return (float) std::sin (twoPi * t); }
    inline double midiToHz (int note) noexcept  { return 440.0 * std::pow (2.0, (note - 69) / 12.0); }

    inline void wrap (double& ph) noexcept
    {
        if (ph >= 1.0) ph -= std::floor (ph);
    }

    // Réglages par modèle de son de plaque
    //                                      Omni1 Omni2 Harpe Guit. Banjo Vibes EPno  Crist Synth MBox
    constexpr float plateDecayMul[]     = { 1.0f, 1.1f, 1.2f, 1.0f, 0.45f, 1.3f, 1.0f, 1.3f, 0.7f, 0.55f };
    constexpr float plateFastTime[]     = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.45f, 0.35f, 0.9f, 0.28f, 0.12f };
    constexpr double plateModRatio[]    = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 3.5, 1.006, 5.4 };

    // Filtre passe-bas fixe par modèle d'accord (Hz)
    constexpr double padCutoff[]        = { 2200.0, 20000.0, 3200.0, 2600.0, 1300.0, 20000.0 };

    inline bool isPlucked (int model) noexcept { return model == Harp || model == Guitar || model == Banjo; }
}

//==============================================================================
float Engine::plateModelGain (int model) noexcept
{
    //                              Omni1 Omni2 Harpe Guit. Banjo Vibes EPno  Crist Synth MBox
    constexpr float gains[]     = { 1.0f, 0.62f, 3.1f, 3.0f, 3.7f, 0.52f, 0.6f, 0.49f, 0.92f, 0.8f };
    return gains[juce::jlimit (0, NumPlateVoices - 1, model)];
}

float Engine::padModelGain (int model) noexcept
{
    //                              Analog Orgue Cordes Accord. Choeur EPiano
    constexpr float gains[]     = { 1.0f,  0.6f, 0.72f, 0.54f,  0.88f, 1.4f };
    return gains[juce::jlimit (0, NumPadVoices - 1, model)];
}

//==============================================================================
int Engine::typeForKey (int key) noexcept
{
    if (key < chordFirstKey) return Major;
    return juce::jmin ((int) Seventh, (key - chordFirstKey) / 12);
}

int Engine::chordIntervals (int t, int* out) noexcept
{
    switch (t)
    {
        case Minor:   out[0] = 0; out[1] = 3; out[2] = 7;               return 3;
        case Seventh: out[0] = 0; out[1] = 4; out[2] = 7; out[3] = 10;  return 4;
        default:      out[0] = 0; out[1] = 4; out[2] = 7;               return 3;
    }
}

int Engine::plateNoteFor (int r, int t, int index) noexcept
{
    int iv[4];
    const int n = chordIntervals (t, iv);
    bool pcs[12] = {};
    for (int i = 0; i < n; ++i)
        pcs[(r + iv[i]) % 12] = true;

    int k = -1;
    for (int note = 53; note < 128; ++note)          // la plaque démarre autour de F3
        if (pcs[note % 12] && ++k == index)
            return note;
    return 127;
}

//==============================================================================
Engine::Engine()
{
    for (auto& f : uiPlateFlash)
        f.store (0.0f);
    held.reserve (128);
}

void Engine::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = juce::jmax (1, maxBlockSize);
    mix.setSize (2, maxBlock);
    slide.setSize (2, maxBlock);

    delayL.assign ((size_t) (2.1 * sr) + 4, 0.0f);
    delayR.assign (delayL.size(), 0.0f);

    ksMemory.assign (plucks.size() * (size_t) ksSize, 0.0f);
    for (size_t i = 0; i < plucks.size(); ++i)
        plucks[i].ks = ksMemory.data() + i * (size_t) ksSize;

    const juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 2 };
    chorus.prepare (spec);
    chorus.setRate (0.6f);
    chorus.setDepth (0.22f);
    chorus.setCentreDelay (9.0f);
    chorus.setFeedback (0.0f);
    chorus.setMix (0.3f);

    filter.prepare (spec);
    filter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    reverb.prepare (spec);
    lastReverb = lastRoom = -1.0f;

    for (auto& v : pads)   v.adsr.setSampleRate (sr);
    for (auto& v : basses) v.adsr.setSampleRate (sr);

    reset();
}

void Engine::reset()
{
    for (auto& v : plucks) v.active = false;
    for (auto& v : pads)   { v.active = false; v.adsr.reset(); }
    for (auto& v : basses) { v.active = false; v.adsr.reset(); }
    held.clear();
    lastWheel = -1;
    plateBright = 0.5f;
    uiBright = 0.5f;
    chorus.reset();
    reverb.reset();
    filter.reset();
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    delayWrite = 0;
    delayFbLp = 0.0f;
    uiHeld = false;
}

//==============================================================================
void Engine::startChord (int r, int t)
{
    root = r;
    type = t;
    uiRoot = r;
    uiType = t;
    uiHeld = true;

    for (auto& v : pads)   if (v.active) v.adsr.noteOff();
    for (auto& v : basses) if (v.active) v.adsr.noteOff();

    const int model = juce::jlimit (0, NumPadVoices - 1, p.padVoice);

    juce::ADSR::Parameters env;
    switch (model)
    {
        case PadOrgan:     env = { 0.004f, 0.05f, 1.0f,  0.07f }; break;
        case PadStrings:   env = { 0.22f,  0.5f,  0.85f, 0.55f }; break;
        case PadAccordion: env = { 0.03f,  0.1f,  0.9f,  0.12f }; break;
        case PadChoir:     env = { 0.35f,  0.6f,  0.8f,  0.8f  }; break;
        case PadEPiano:    env = { 0.002f, 1.8f,  0.2f,  0.35f }; break;
        default:           env = { 0.012f, 0.35f, 0.75f, 0.28f }; break;
    }

    int iv[4];
    const int n = chordIntervals (t, iv);

    for (int i = 0; i < n; ++i)
    {
        int note = 48 + (r + iv[i]) % 12;
        if (note < 53) note += 12;                    // voicing serré entre F3 et E4

        Pad* slot = &pads[0];
        for (auto& v : pads)
        {
            if (! v.active) { slot = &v; break; }
            if (v.age < slot->age) slot = &v;
        }

        slot->active = true;
        slot->model = model;
        slot->inc = midiToHz (note) / sr;
        slot->ph[0] = 0.0;
        slot->ph[1] = 0.37;
        slot->ph[2] = 0.71;
        slot->lp1 = slot->lp2 = 0.0f;
        slot->fmEnv = 1.0f;
        slot->fmDecay = (float) std::exp (-6.9 / (0.5 * sr));
        slot->age = ++ageCounter;
        slot->adsr.setParameters (env);
        slot->adsr.reset();
        slot->adsr.noteOn();
    }

    int bassNote = 36 + r;
    if (bassNote > 43) bassNote -= 12;

    Bass* b = &basses[0];
    for (auto& v : basses)
    {
        if (! v.active) { b = &v; break; }
        if (v.age < b->age) b = &v;
    }

    b->active = true;
    b->inc = midiToHz (bassNote) / sr;
    b->phase = 0.0;
    b->age = ++ageCounter;
    b->adsr.setParameters ({ 0.005f, 0.45f, 0.55f, 0.22f });
    b->adsr.reset();
    b->adsr.noteOn();
}

void Engine::releaseChord()
{
    for (auto& v : pads)   if (v.active) v.adsr.noteOff();
    for (auto& v : basses) if (v.active) v.adsr.noteOff();
    uiHeld = false;
}

void Engine::triggerPlate (int index, float velocity)
{
    index = juce::jlimit (0, numPlate - 1, index);
    const int note  = plateNoteFor (root, type, index);
    const float pan = -0.55f + 1.1f * (float) index / (float) (numPlate - 1);

    startPluck (juce::jlimit (0, NumPlateVoices - 1, p.plateVoice), note, velocity, 1.0f, pan, 0.0f);

    if (p.layerVoice >= 0 && p.layerLevel > 0.001f)
    {
        const int layerNote = juce::jlimit (24, 120, note + 12 * p.layerOctave);
        startPluck (juce::jlimit (0, NumPlateVoices - 1, p.layerVoice), layerNote, velocity, p.layerLevel,
                    -pan * 0.7f, p.layerOctave == 0 ? 7.0f : 0.0f);
    }

    uiPlateFlash[(size_t) index].store (1.0f);
}

void Engine::startPluck (int model, int note, float velocity, float level, float pan, float detuneCents)
{
    Pluck* slot = &plucks[0];
    for (auto& v : plucks)
    {
        if (! v.active) { slot = &v; break; }
        if (v.age < slot->age) slot = &v;
    }

    auto& v = *slot;
    const float decaySec = juce::jmax (0.05f, p.strumDecay) * plateDecayMul[model];

    v.active    = true;
    v.model     = model;
    v.freq      = (float) (midiToHz (note) * std::pow (2.0, detuneCents / 1200.0));
    v.inc       = v.freq / sr;
    v.minc      = v.inc * plateModRatio[model];
    v.phase     = 0.0;
    v.mphase    = 0.0;
    v.env       = 1.0f;
    v.envFast   = 1.0f;
    v.att       = 0.0f;
    v.attStep   = (float) (1.0 / (juce::jmax (0.0015, (double) p.attack) * sr));
    v.lp        = 0.0f;
    v.lp2       = 0.0f;
    v.decay     = (float) std::exp (-6.9 / ((decaySec + p.attack) * sr));   // -60 dB après decaySec
    v.decayFast = (float) std::exp (-6.9 / (plateFastTime[model] * sr));
    v.vel       = (0.35f + 0.65f * velocity) * level;

    if (isPlucked (model))
    {
        // Karplus-Strong : la corde se décroît dans la boucle, l'enveloppe ne sert que de garde-fou
        const float t = juce::jlimit (0.0f, 1.0f, p.strumTone + (plateBright - 0.5f) * 0.9f);
        // Le filtre de boucle suit la hauteur : coupure = N harmoniques au-dessus de la note,
        // pour que les aigus sonnent aussi longtemps que les graves.
        float harmonics, exA;
        if (model == Harp)        { harmonics = 5.0f + 7.0f * t;   exA = 0.15f + 0.40f * t; }
        else if (model == Guitar) { harmonics = 8.0f + 16.0f * t;  exA = 0.30f + 0.60f * t; }
        else                      { harmonics = 14.0f + 26.0f * t; exA = 0.60f + 0.40f * t; }

        const double fc = juce::jmin ((double) (harmonics * v.freq), 0.45 * sr);
        v.ksA   = (float) (1.0 - std::exp (-twoPi * fc / sr));
        v.ksExA = juce::jmax (exA, v.ksA);

        // Accordage exact : on retire le retard de phase du filtre de boucle et de l'interpolation
        const double w   = twoPi * v.freq / sr;
        const double b   = 1.0 - v.ksA;
        const double lpDelay = std::atan2 (b * std::sin (w), 1.0 - b * std::cos (w)) / w;
        const double target  = sr / v.freq - lpDelay;
        v.ksInt = juce::jmax (1, (int) std::floor (target));
        const double frac = target - v.ksInt;

        double lo = 0.0, hi = 1.0;               // poids de l'échantillon le plus récent
        for (int it = 0; it < 30; ++it)
        {
            const double fr = 0.5 * (lo + hi);
            const double d = std::atan2 ((1.0 - fr) * std::sin (w), fr + (1.0 - fr) * std::cos (w)) / w;
            if (d > frac) lo = fr; else hi = fr;
        }
        v.ksFrac = (float) (0.5 * (lo + hi));

        // Excitation : une période de bruit filtré, centrée et normalisée (volume régulier d'une note à l'autre)
        std::fill (v.ks, v.ks + ksSize, 0.0f);
        const int len = juce::jmin (ksSize - 1, v.ksInt + 2);
        float lpState = 0.0f, mean = 0.0f;
        for (int j = 0; j < len; ++j)
        {
            lpState += v.ksExA * ((rng.nextFloat() * 2.0f - 1.0f) - lpState);
            v.ks[ksSize - 1 - j] = lpState;
            mean += lpState;
        }
        mean /= (float) len;
        float energy = 0.0f;
        for (int j = 0; j < len; ++j)
        {
            auto& x = v.ks[ksSize - 1 - j];
            x -= mean;
            energy += x * x;
        }
        const float norm = 0.5f / std::sqrt (energy / (float) len + 1.0e-12f);
        for (int j = 0; j < len; ++j)
            v.ks[ksSize - 1 - j] *= norm;

        v.ksW      = 0;
        v.ksExcite = 0;
        const double lpMag = v.ksA / std::sqrt (1.0 - 2.0 * b * std::cos (w) + b * b);
        v.ksGain   = (float) juce::jmin (0.9995, std::pow (10.0, -3.0 / (decaySec * v.freq)) / lpMag);
        v.exLp     = 0.0f;
        v.decay    = (float) std::exp (-6.9 / ((decaySec * 1.4 + p.attack) * sr));
    }

    const float angle = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    v.gainL = std::cos (angle);
    v.gainR = std::sin (angle);
    v.age   = ++ageCounter;
}

//==============================================================================
void Engine::handle (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();

        if (n >= plateFirstKey)
        {
            if (n < plateFirstKey + numPlate)
                triggerPlate (n - plateFirstKey, m.getFloatVelocity());
        }
        else
        {
            held.erase (std::remove (held.begin(), held.end(), n), held.end());
            if (held.size() < 128)
                held.push_back (n);
            startChord (n % 12, typeForKey (n));
        }
    }
    else if (m.isNoteOff())
    {
        const int n = m.getNoteNumber();

        if (n < plateFirstKey)
        {
            const bool wasCurrent = ! held.empty() && held.back() == n;
            held.erase (std::remove (held.begin(), held.end(), n), held.end());

            if (p.hold)
                return;                       // en mode HOLD l'accord reste jusqu'au suivant

            if (held.empty())
                releaseChord();
            else if (wasCurrent)
                startChord (held.back() % 12, typeForKey (held.back()));
        }
    }
    else if (m.isController() && m.getControllerNumber() == 1)
    {
        const int pos = juce::roundToInt ((float) m.getControllerValue() / 127.0f * (float) (numPlate - 1));

        if (lastWheel < 0)
        {
            triggerPlate (pos, 0.8f);
        }
        else if (pos != lastWheel)
        {
            const int step = pos > lastWheel ? 1 : -1;
            for (int i = lastWheel + step; ; i += step)
            {
                triggerPlate (i, 0.8f);
                if (i == pos) break;
            }
        }
        lastWheel = pos;
    }
    else if (m.isController() && m.getControllerNumber() == 74)
    {
        plateBright = (float) m.getControllerValue() / 127.0f;
        uiBright = plateBright;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        held.clear();
        releaseChord();

        if (m.isAllSoundOff())
            for (auto& v : plucks) v.active = false;
    }
}

//==============================================================================
float Engine::renderPluck (Pluck& v, double vib, float lfo01, float t) noexcept
{
    const float fcScale = (float) (twoPi / sr);
    const double dt = v.inc * vib;
    float s = 0.0f;

    switch (v.model)
    {
        case Omni1:
        {
            // le son "analogique" classique : triangle -> carré, filtre qui se referme
            const float raw = (1.0f - t) * triangle (v.phase) + t * 0.55f * squareBlep (v.phase, dt);
            const float fc  = (450.0f + t * 6500.0f) * (0.2f + 0.8f * v.env) + v.freq * 1.5f;
            v.lp += juce::jmin (1.0f, fc * fcScale) * (raw - v.lp);
            s = v.lp;
            v.phase += dt;
            break;
        }

        case Omni2:
        {
            // plus rond et plus "chantant", vibrato plus marqué
            const double dt2 = v.inc * (1.0 + (vib - 1.0) * 1.6);
            const float raw = triangle (v.phase) + 0.3f * sine (2.0 * v.phase);
            const float fc  = (300.0f + t * 3000.0f) * (0.3f + 0.7f * v.env) + v.freq * 2.0f;
            v.lp += juce::jmin (1.0f, fc * fcScale) * (raw - v.lp);
            s = v.lp;
            v.phase += dt2;
            break;
        }

        case Harp:
        case Guitar:
        case Banjo:
        {
            const float newer = v.ks[(v.ksW - v.ksInt) & ksMask];
            const float older = v.ks[(v.ksW - v.ksInt - 1) & ksMask];
            const float d = v.ksFrac * newer + (1.0f - v.ksFrac) * older;

            v.lp += v.ksA * (d - v.lp);

            float in = 0.0f;
            if (v.ksExcite > 0)
            {
                const float nz = rng.nextFloat() * 2.0f - 1.0f;
                v.exLp += v.ksExA * (nz - v.exLp);
                in = v.exLp * std::sqrt ((2.0f - v.ksExA) / v.ksExA) * 0.6f;
                --v.ksExcite;
            }

            const float y = in + v.ksGain * v.lp;
            v.ks[v.ksW] = y;
            v.ksW = (v.ksW + 1) & ksMask;
            s = y;
            break;
        }

        case Vibes:
        {
            s = sine (v.phase)
              + (0.25f + 0.4f * t) * v.envFast * sine (4.0 * v.phase)
              + 0.15f * t * v.envFast * v.envFast * sine (10.0 * v.phase);
            s *= 1.0f - 0.6f * p.vibrato * lfo01;              // trémolo façon vibraphone
            v.phase += v.inc;
            break;
        }

        case EPiano:
        {
            const float index = 2.4f * (0.25f + t) * v.envFast;
            s = (float) std::sin (twoPi * v.phase + index * std::sin (twoPi * v.phase));
            v.phase += dt;
            break;
        }

        case Crystal:
        {
            const float index = 2.6f * (0.25f + t) * v.envFast;
            s = (float) std::sin (twoPi * v.phase + index * std::sin (twoPi * v.mphase));
            v.phase += dt;
            v.mphase += v.minc * vib;
            break;
        }

        case SynthPluck:
        {
            const double mdt = v.minc * vib;
            const float raw = sawBlep (v.phase, dt) + 0.6f * sawBlep (v.mphase, mdt);
            const float fc  = 120.0f + (250.0f + t * 8500.0f) * v.envFast;
            const float g   = juce::jmin (1.0f, fc * fcScale);
            v.lp  += g * (raw - v.lp);
            v.lp2 += g * (v.lp - v.lp2);
            s = v.lp2;
            v.phase += dt;
            v.mphase += mdt;
            break;
        }

        case MusicBox:
        default:
        {
            s = sine (v.phase)
              + 0.3f * v.envFast * sine (v.mphase)
              + 0.15f * v.envFast * sine (2.0 * v.phase);
            v.phase += dt;
            v.mphase += v.minc * vib;
            break;
        }
    }

    wrap (v.phase);
    wrap (v.mphase);

    v.att = juce::jmin (1.0f, v.att + v.attStep);
    const float out = s * v.env * v.att * v.vel;

    v.env     *= v.decay;
    v.envFast *= v.decayFast;

    if (v.env < 0.0005f)
        v.active = false;

    return out;
}

float Engine::renderPad (Pad& v, double vib) noexcept
{
    const float e = v.adsr.getNextSample();
    const double dt = v.inc * vib;
    float s = 0.0f;
    bool filtered = true;

    switch (v.model)
    {
        case PadOrgan:
            s = sine (v.ph[0]) + 0.8f * sine (2.0 * v.ph[0]) + 0.5f * sine (3.0 * v.ph[0]) + 0.3f * sine (4.0 * v.ph[0]);
            v.ph[0] += dt;
            filtered = false;
            break;

        case PadStrings:
        {
            const double d0 = dt, d1 = dt * 1.006, d2 = dt * 0.994;
            s = sawBlep (v.ph[0], d0) + sawBlep (v.ph[1], d1) + sawBlep (v.ph[2], d2);
            v.ph[0] += d0; v.ph[1] += d1; v.ph[2] += d2;
            break;
        }

        case PadAccordion:
        {
            const double d0 = dt * 1.0045, d1 = dt * 0.9955;   // effet "musette"
            s = pulseBlep (v.ph[0], d0, 0.3) + pulseBlep (v.ph[1], d1, 0.3);
            v.ph[0] += d0; v.ph[1] += d1;
            break;
        }

        case PadChoir:
        {
            const double d0 = dt * 1.002, d1 = dt * 0.998;
            s = triangle (v.ph[0]) + triangle (v.ph[1]) + 0.3f * sine (2.0 * v.ph[2]);
            v.ph[0] += d0; v.ph[1] += d1; v.ph[2] += dt;
            break;
        }

        case PadEPiano:
        {
            const float index = 1.8f * v.fmEnv;
            s = (float) std::sin (twoPi * v.ph[0] + index * std::sin (twoPi * v.ph[0]));
            v.fmEnv *= v.fmDecay;
            v.ph[0] += dt;
            filtered = false;
            break;
        }

        case PadAnalog:
        default:
        {
            const double d0 = dt * 1.0035, d1 = dt * 0.9965;
            s = sawBlep (v.ph[0], d0) + sawBlep (v.ph[1], d1);
            v.ph[0] += d0; v.ph[1] += d1;
            break;
        }
    }

    for (auto& ph : v.ph)
        wrap (ph);

    if (filtered)
    {
        const float g = (float) juce::jmin (1.0, twoPi * padCutoff[v.model] / sr);
        v.lp1 += g * (s - v.lp1);
        v.lp2 += g * (v.lp1 - v.lp2);
        s = v.lp2;
    }

    if (! v.adsr.isActive())
        v.active = false;

    return s * e * padModelGain (v.model);
}

void Engine::render (int start, int end)
{
    auto* L  = mix.getWritePointer (0);
    auto* R  = mix.getWritePointer (1);
    auto* SL = slide.getWritePointer (0);
    auto* SR = slide.getWritePointer (1);

    const float strumGain = p.strumLevel * 0.30f;
    const float padGain   = p.chordLevel * 0.10f;
    const float bassGain  = p.bassLevel * 0.40f;
    const double lfoInc   = 5.3 / sr;
    const double vibDepth = p.vibrato * 0.0065;

    // Timbre effectif : bouton TIMBRE + position horizontale sur le pad (CC74)
    const float tEff = juce::jlimit (0.0f, 1.0f, p.strumTone + (plateBright - 0.5f) * 0.9f);

    float plateGains[NumPlateVoices];
    for (int m = 0; m < NumPlateVoices; ++m)
        plateGains[m] = strumGain * plateModelGain (m);

    for (int i = start; i < end; ++i)
    {
        lfoPhase += lfoInc;
        wrap (lfoPhase);
        const double lfo = std::sin (twoPi * lfoPhase);
        const float lfo01 = (float) (0.5 + 0.5 * lfo);
        const double vibPluck = 1.0 + vibDepth * lfo;
        const double vibPad   = 1.0 + vibDepth * 0.4 * lfo;

        float sl = 0.0f, sr2 = 0.0f;

        for (auto& v : plucks)
        {
            if (! v.active) continue;
            const float s = renderPluck (v, vibPluck, lfo01, tEff) * plateGains[v.model];
            sl  += s * v.gainL;
            sr2 += s * v.gainR;
        }

        SL[i] += sl;
        SR[i] += sr2;

        float l = 0.0f, r = 0.0f;

        for (auto& v : pads)
        {
            if (! v.active) continue;
            const float o = renderPad (v, vibPad) * padGain;
            l += o;
            r += o;
        }

        for (auto& v : basses)
        {
            if (! v.active) continue;

            const float e = v.adsr.getNextSample();
            const float s = sine (v.phase) + 0.2f * triangle (v.phase);
            v.phase += v.inc;
            wrap (v.phase);

            const float o = s * e * bassGain;
            l += o;
            r += o;

            if (! v.adsr.isActive())
                v.active = false;
        }

        L[i] += l;
        R[i] += r;
    }
}

void Engine::processSlideBus (int numSamples)
{
    auto* L = slide.getWritePointer (0);
    auto* R = slide.getWritePointer (1);

    // 1. Filtre résonant + LFO
    const bool filterOn = p.cutoff < 19000.0f || p.lfoDepth > 0.001f;
    if (filterOn)
    {
        filter.setResonance (0.707f + p.resonance * p.resonance * 9.0f);
        const double lfoInc = p.lfoRate / sr;

        for (int i = 0; i < numSamples; ++i)
        {
            filterLfoPhase += lfoInc;
            wrap (filterLfoPhase);

            if ((filterCounter++ & 7) == 0)
            {
                const double mod = std::pow (2.0, p.lfoDepth * 3.0 * std::sin (twoPi * filterLfoPhase));
                filter.setCutoffFrequency ((float) juce::jlimit (30.0, 0.45 * sr, (double) p.cutoff * mod));
            }
            L[i] = filter.processSample (0, L[i]);
            R[i] = filter.processSample (1, R[i]);
        }
    }

    // 2. Saturation
    if (p.drive > 0.001f)
    {
        const float k = 1.0f + 6.0f * p.drive;
        const float makeup = 1.0f / std::pow (k, 0.75f);
        for (int i = 0; i < numSamples; ++i)
        {
            L[i] = std::tanh (k * L[i]) * makeup;
            R[i] = std::tanh (k * R[i]) * makeup;
        }
    }

    // 3. Delay ping-pong calé sur le tempo
    if (p.delayMix > 0.001f && ! delayL.empty())
    {
        const int size = (int) delayL.size();
        const double seconds = delayBeats[juce::jlimit (0, numDelayDivs - 1, p.delayDiv)] * 60.0 / juce::jlimit (30.0, 300.0, p.bpm);
        const int d = juce::jlimit (1, size - 2, (int) (seconds * sr));
        const float fb = juce::jlimit (0.0f, 0.92f, p.delayFeedback);
        const float damp = (float) juce::jmin (1.0, twoPi * 4500.0 / sr);

        for (int i = 0; i < numSamples; ++i)
        {
            int rd = delayWrite - d;
            if (rd < 0) rd += size;
            const float dl = delayL[(size_t) rd];
            const float dr = delayR[(size_t) rd];

            delayFbLp += damp * (dr * fb - delayFbLp);
            delayL[(size_t) delayWrite] = 0.5f * (L[i] + R[i]) + delayFbLp;
            delayR[(size_t) delayWrite] = dl * fb;
            if (++delayWrite >= size) delayWrite = 0;

            L[i] += dl * p.delayMix;
            R[i] += dr * p.delayMix;
        }
    }
}

void Engine::applyEffects (int numSamples)
{
    chorus.setMix (p.chorusMix);

    if (p.reverb != lastReverb || p.roomSize != lastRoom)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = p.roomSize;
        rp.damping    = 0.45f;
        rp.wetLevel   = p.reverb * 0.45f;
        rp.dryLevel   = 1.0f - p.reverb * 0.3f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        lastReverb = p.reverb;
        lastRoom = p.roomSize;
    }

    juce::dsp::AudioBlock<float> full (mix);

    for (int s = 0; s < numSamples; s += maxBlock)
    {
        const int len = juce::jmin (maxBlock, numSamples - s);
        auto sub = full.getSubBlock ((size_t) s, (size_t) len);
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        chorus.process (ctx);
        reverb.process (ctx);
    }
}

//==============================================================================
void Engine::process (juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi, const Params& params)
{
    const int n = output.getNumSamples();
    if (n <= 0) return;

    p = params;

    if (lastHold && ! p.hold && held.empty())
        releaseChord();                       // on coupe l'accord quand on désactive HOLD
    lastHold = p.hold;

    if (mix.getNumSamples() < n)
    {
        mix.setSize (2, n, false, false, true);
        slide.setSize (2, n, false, false, true);
    }

    mix.clear (0, n);
    slide.clear (0, n);

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos)
        {
            render (pos, t);
            pos = t;
        }
        handle (meta.getMessage());
    }
    if (pos < n)
        render (pos, n);

    processSlideBus (n);
    mix.addFrom (0, 0, slide, 0, 0, n);
    mix.addFrom (1, 0, slide, 1, 0, n);

    applyEffects (n);

    auto* L = mix.getWritePointer (0);
    auto* R = mix.getWritePointer (1);
    const float g = p.master;

    for (int i = 0; i < n; ++i)
    {
        L[i] = std::tanh (L[i] * g);
        R[i] = std::tanh (R[i] * g);
    }

    const int nch = output.getNumChannels();
    if (nch == 1)
    {
        auto* o = output.getWritePointer (0);
        for (int i = 0; i < n; ++i)
            o[i] = 0.5f * (L[i] + R[i]);
    }
    else if (nch >= 2)
    {
        output.copyFrom (0, 0, mix, 0, 0, n);
        output.copyFrom (1, 0, mix, 1, 0, n);
        for (int c = 2; c < nch; ++c)
            output.clear (c, 0, n);
    }
}

} // namespace chordharp
