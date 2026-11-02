// Outil de test hors-ligne.
//   ChordHarpRender demo  sortie.wav   -> tour des 10 presets
//   ChordHarpRender calib              -> mesure le volume de chaque modèle de son

#include "../Source/Engine.h"
#include "../Source/Presets.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

using namespace chordharp;

namespace
{
    constexpr double sr = 44100.0;
    constexpr int block = 512;

    using Events = std::vector<std::pair<double, juce::MidiMessage>>;

    void addChord (Events& ev, double t, double dur, int pc, int type)
    {
        const int k = Engine::chordKeyFor (pc, type);
        ev.push_back ({ t, juce::MidiMessage::noteOn (1, k, 0.8f) });
        ev.push_back ({ t + dur, juce::MidiMessage::noteOff (1, k) });
    }

    void addSwipe (Events& ev, double t, double stepTime, int from, int to)
    {
        const int step = to >= from ? 1 : -1;
        for (int i = from, k = 0; ; i += step, ++k)
        {
            ev.push_back ({ t + k * stepTime, juce::MidiMessage::noteOn (1, Engine::plateFirstKey + i, 0.85f) });
            if (i == to) break;
        }
    }

    // Rend une liste d'événements ; "paramsAt" donne les paramètres à chaque bloc
    template <typename ParamFn>
    juce::AudioBuffer<float> renderEvents (Events ev, double seconds, ParamFn paramsAt)
    {
        std::stable_sort (ev.begin(), ev.end(), [] (auto& a, auto& b) { return a.first < b.first; });

        Engine eng;
        eng.prepare (sr, block);

        const int total = (int) (seconds * sr);
        juce::AudioBuffer<float> all (2, total), buf (2, block);
        all.clear();
        juce::MidiBuffer midi;
        size_t e = 0;

        for (int pos = 0; pos < total; pos += block)
        {
            const int n = juce::jmin (block, total - pos);
            midi.clear();
            while (e < ev.size() && (int) (ev[e].first * sr) < pos + n)
            {
                midi.addEvent (ev[e].second, juce::jmax (0, (int) (ev[e].first * sr) - pos));
                ++e;
            }
            buf.setSize (2, n, false, false, true);
            eng.process (buf, midi, paramsAt (pos / sr));
            all.copyFrom (0, pos, buf, 0, 0, n);
            all.copyFrom (1, pos, buf, 1, 0, n);
        }
        return all;
    }

    double rms (const juce::AudioBuffer<float>& b, double from, double to)
    {
        const int s0 = (int) (from * sr), s1 = juce::jmin (b.getNumSamples(), (int) (to * sr));
        double acc = 0.0;
        for (int c = 0; c < 2; ++c)
            for (int i = s0; i < s1; ++i)
                acc += b.getSample (c, i) * b.getSample (c, i);
        return std::sqrt (acc / (2.0 * (s1 - s0)));
    }

    bool writeWav (const juce::AudioBuffer<float>& all, const juce::String& path)
    {
        juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (path);
        f.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
        JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wdeprecated-declarations")
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
        JUCE_END_IGNORE_WARNINGS_GCC_LIKE
        if (w == nullptr) return false;
        os.release();
        w->writeFromAudioSampleBuffer (all, 0, all.getNumSamples());
        std::cout << "ecrit : " << f.getFullPathName() << std::endl;
        return true;
    }
}

int main (int argc, char** argv)
{
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String ("demo");

    if (mode == "calib")
    {
        // Plaque seule : 2 balayages sur un accord de Do, timbre neutre
        for (int m = 0; m < NumPlateVoices; ++m)
        {
            Events ev;
            addChord (ev, 0.0, 3.0, 0, Major);
            addSwipe (ev, 0.05, 0.04, 0, 12);
            addSwipe (ev, 1.2, 0.05, 12, 0);
            auto out = renderEvents (ev, 3.0, [m] (double)
            {
                Params p; p.plateVoice = m; p.chordLevel = 0; p.bassLevel = 0; p.reverb = 0;
                p.strumDecay = 1.5f; p.strumTone = 0.5f; p.master = 0.5f; return p;
            });
            std::cout << "plate " << m << " " << plateVoiceNames[m] << " rms=" << rms (out, 0.1, 2.5) << std::endl;
        }
        // Accord seul
        for (int m = 0; m < NumPadVoices; ++m)
        {
            Events ev;
            addChord (ev, 0.0, 2.5, 0, Major);
            auto out = renderEvents (ev, 3.0, [m] (double)
            {
                Params p; p.padVoice = m; p.strumLevel = 0; p.bassLevel = 0; p.reverb = 0;
                p.chordLevel = 1.0f; p.master = 0.5f; return p;
            });
            std::cout << "pad " << m << " " << padVoiceNames[m] << " rms=" << rms (out, 0.05, 2.4) << std::endl;
        }
        return 0;
    }

    if (mode == "pitch")
    {
        // Une note isolée (position 6 puis 12 sur un accord de Do) pour chaque modèle de corde
        for (int m : { (int) Harp, (int) Guitar, (int) Banjo })
            for (int idx : { 6, 12 })
            {
                Events ev;
                addChord (ev, 0.0, 0.01, 0, Major);
                ev.push_back ({ 0.02, juce::MidiMessage::noteOn (1, Engine::plateFirstKey + idx, 0.9f) });
                auto out = renderEvents (ev, 1.0, [m] (double)
                {
                    Params p; p.plateVoice = m; p.chordLevel = 0; p.bassLevel = 0; p.reverb = 0;
                    p.vibrato = 0; p.master = 0.5f; return p;
                });
                writeWav (out, "pitch_" + juce::String (m) + "_" + juce::String (idx) + "_"
                               + juce::String (Engine::plateNoteFor (0, Major, idx)) + ".wav");
            }
        return 0;
    }

    // Démo : chaque preset joue deux accords avec des slides
    const double seg = 3.2;
    const int prog[][2] = { { 9, Minor }, { 5, Major }, { 0, Major }, { 7, Major }, { 2, Minor },
                            { 9, Minor }, { 5, Major }, { 0, Major }, { 7, Seventh }, { 4, Minor } };
    Events ev;

    for (int i = 0; i < numPresets; ++i)
    {
        const double t = i * seg;
        const int a = prog[i % 10][0], ta = prog[i % 10][1];
        const int b = prog[(i + 3) % 10][0], tb = prog[(i + 3) % 10][1];
        addChord (ev, t, seg * 0.5 - 0.05, a, ta);
        addSwipe (ev, t + 0.02, 0.035, 0, 12);
        addSwipe (ev, t + 0.9, 0.06, 9, 4);
        addChord (ev, t + seg * 0.5, seg * 0.5 - 0.1, b, tb);
        addSwipe (ev, t + seg * 0.5 + 0.02, 0.05, 2, 11);
        ev.push_back ({ t + seg * 0.5 + 0.7, juce::MidiMessage::noteOn (1, Engine::plateFirstKey + 8, 0.8f) });
        ev.push_back ({ t + seg * 0.5 + 1.0, juce::MidiMessage::noteOn (1, Engine::plateFirstKey + 10, 0.8f) });
    }

    const double total = numPresets * seg + 4.0;
    auto out = renderEvents (ev, total, [seg] (double time)
    {
        return paramsForPreset (juce::jmin (numPresets - 1, (int) (time / seg)));
    });

    float peak = 0.0f;
    bool finite = true;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const float s = out.getSample (c, i);
            if (! std::isfinite (s)) finite = false;
            peak = juce::jmax (peak, std::abs (s));
        }

    for (int i = 0; i < numPresets; ++i)
        std::cout << "preset " << i << " " << presets[i].name << " rms=" << rms (out, i * seg, (i + 1) * seg) << std::endl;
    std::cout << "peak=" << peak << " finite=" << (finite ? "yes" : "NO") << std::endl;

    if (! writeWav (out, argc > 2 ? juce::String (argv[2]) : juce::String ("demo.wav")))
        return 1;
    return finite ? 0 : 2;
}
