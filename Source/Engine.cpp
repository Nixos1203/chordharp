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

    inline float triangle (double t) noexcept
    {
        return 1.0f - 4.0f * std::abs ((float) t - 0.5f);
    }

    inline double midiToHz (int note) noexcept
    {
        return 440.0 * std::pow (2.0, (note - 69) / 12.0);
    }

    inline void wrap (double& ph) noexcept
    {
        if (ph >= 1.0) ph -= 1.0;
    }
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

    const juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 2 };
    chorus.prepare (spec);
    chorus.setRate (0.6f);
    chorus.setDepth (0.22f);
    chorus.setCentreDelay (9.0f);
    chorus.setFeedback (0.0f);
    chorus.setMix (0.3f);

    reverb.prepare (spec);
    lastReverb = -1.0f;

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
    chorus.reset();
    reverb.reset();
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
        slot->inc = midiToHz (note) / sr;
        slot->ph1 = 0.0;
        slot->ph2 = 0.37;
        slot->lp1 = slot->lp2 = 0.0f;
        slot->age = ++ageCounter;
        slot->adsr.setParameters ({ 0.012f, 0.35f, 0.75f, 0.28f });
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
    const int note = plateNoteFor (root, type, index);

    Pluck* slot = &plucks[0];
    for (auto& v : plucks)
    {
        if (! v.active) { slot = &v; break; }
        if (v.age < slot->age) slot = &v;
    }

    auto& v = *slot;
    v.active = true;
    v.freq   = (float) midiToHz (note);
    v.inc    = v.freq / sr;
    v.phase  = 0.0;
    v.env    = 1.0f;
    v.att    = 0.0f;
    v.lp     = 0.0f;
    v.decay  = (float) std::exp (-6.9 / (juce::jmax (0.05f, p.strumDecay) * sr));   // -60 dB après "strumDecay" secondes
    v.vel    = 0.35f + 0.65f * velocity;

    const float pan   = -0.55f + 1.1f * (float) index / (float) (numPlate - 1);
    const float angle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
    v.gainL = std::cos (angle);
    v.gainR = std::sin (angle);
    v.age   = ++ageCounter;

    uiPlateFlash[(size_t) index].store (1.0f);
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
    else if (m.isAllNotesOff() || m.isAllSoundOff())
    {
        held.clear();
        releaseChord();

        if (m.isAllSoundOff())
            for (auto& v : plucks) v.active = false;
    }
}

//==============================================================================
void Engine::render (int start, int end)
{
    auto* L = mix.getWritePointer (0);
    auto* R = mix.getWritePointer (1);

    const float tone       = p.strumTone;
    const float strumGain  = p.strumLevel * 0.30f;
    const float padGain    = p.chordLevel * 0.10f;
    const float bassGain   = p.bassLevel * 0.40f;
    const double lfoInc    = 5.3 / sr;
    const double vibDepth  = p.vibrato * 0.0065;
    const float attStep    = (float) (1.0 / (0.0015 * sr));
    const float padG       = (float) juce::jmin (1.0, twoPi * 2200.0 / sr);
    const float baseFc     = 450.0f + tone * 6500.0f;
    const float fcScale    = (float) (twoPi / sr);

    for (int i = start; i < end; ++i)
    {
        lfoPhase += lfoInc;
        wrap (lfoPhase);
        const double lfo = std::sin (twoPi * lfoPhase);
        const double vibPluck = 1.0 + vibDepth * lfo;
        const double vibPad   = 1.0 + vibDepth * 0.4 * lfo;

        float l = 0.0f, r = 0.0f;

        // --- Plaque (slide) : son "harpe électronique" ---
        for (auto& v : plucks)
        {
            if (! v.active) continue;

            const double dt = v.inc * vibPluck;
            const float raw = (1.0f - tone) * triangle (v.phase) + tone * 0.55f * squareBlep (v.phase, dt);

            const float fc = baseFc * (0.2f + 0.8f * v.env) + v.freq * 1.5f;
            const float g  = juce::jmin (1.0f, fc * fcScale);
            v.lp += g * (raw - v.lp);

            v.att = juce::jmin (1.0f, v.att + attStep);
            const float s = v.lp * v.env * v.att * v.vel * strumGain;
            l += s * v.gainL;
            r += s * v.gainR;

            v.env *= v.decay;
            v.phase += dt;
            wrap (v.phase);

            if (v.env < 0.0005f)
                v.active = false;
        }

        // --- Accord tenu : deux dents de scie désaccordées filtrées ---
        for (auto& v : pads)
        {
            if (! v.active) continue;

            const float e  = v.adsr.getNextSample();
            const double dt = v.inc * vibPad;
            const double d1 = dt * 1.0035, d2 = dt * 0.9965;
            const float s  = sawBlep (v.ph1, d1) + sawBlep (v.ph2, d2);
            v.ph1 += d1; wrap (v.ph1);
            v.ph2 += d2; wrap (v.ph2);

            v.lp1 += padG * (s - v.lp1);
            v.lp2 += padG * (v.lp1 - v.lp2);

            const float o = v.lp2 * e * padGain;
            l += o;
            r += o;

            if (! v.adsr.isActive())
                v.active = false;
        }

        // --- Basse auto ---
        for (auto& v : basses)
        {
            if (! v.active) continue;

            const float e = v.adsr.getNextSample();
            const float s = (float) std::sin (twoPi * v.phase) + 0.2f * triangle (v.phase);
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

void Engine::applyEffects (int numSamples)
{
    if (p.reverb != lastReverb)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = 0.8f;
        rp.damping    = 0.45f;
        rp.wetLevel   = p.reverb * 0.45f;
        rp.dryLevel   = 1.0f - p.reverb * 0.3f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);
        lastReverb = p.reverb;
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
        mix.setSize (2, n, false, false, true);

    mix.clear (0, n);

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
