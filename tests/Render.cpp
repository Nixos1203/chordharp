// Rend une petite démo (Am - F - C - G) en .wav, sans DAW.
// Usage : ChordHarpRender sortie.wav

#include "../Source/Engine.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

using chordharp::Engine;

int main (int argc, char** argv)
{
    const double sr = 44100.0;
    const int block = 512;
    const juce::String outPath = argc > 1 ? juce::String (argv[1]) : juce::String ("demo.wav");

    Engine eng;
    eng.prepare (sr, block);

    std::vector<std::pair<double, juce::MidiMessage>> ev;

    auto chord = [&] (double t, double dur, int pc, int type)
    {
        const int k = Engine::chordKeyFor (pc, type);
        ev.push_back ({ t, juce::MidiMessage::noteOn (1, k, 0.8f) });
        ev.push_back ({ t + dur, juce::MidiMessage::noteOff (1, k) });
    };

    auto swipe = [&] (double t, double stepTime, int from, int to)
    {
        const int step = to >= from ? 1 : -1;
        for (int i = from, k = 0; ; i += step, ++k)
        {
            ev.push_back ({ t + k * stepTime, juce::MidiMessage::noteOn (1, Engine::plateFirstKey + i, 0.85f) });
            if (i == to) break;
        }
    };

    // Progression Am - F - C - G, 2.4 s par accord
    const int prog[4][2] = { { 9, chordharp::Minor }, { 5, chordharp::Major }, { 0, chordharp::Major }, { 7, chordharp::Major } };
    const double len = 2.4;

    for (int i = 0; i < 4; ++i)
    {
        const double t = i * len;
        chord (t, len - 0.05, prog[i][0], prog[i][1]);
        swipe (t + 0.02, 0.035, 0, 12);
        swipe (t + 1.2, 0.05, 10, 4);
        swipe (t + 1.8, 0.09, 6, 9);
    }

    // Final : accord de 7e + balayage au mod wheel
    const double tEnd = 4 * len;
    chord (tEnd, 2.5, 7, chordharp::Seventh);
    for (int k = 0; k <= 40; ++k)
        ev.push_back ({ tEnd + 0.05 + k * 0.025, juce::MidiMessage::controllerEvent (1, 1, juce::roundToInt (k / 40.0 * 127.0)) });

    std::stable_sort (ev.begin(), ev.end(), [] (auto& a, auto& b) { return a.first < b.first; });

    const double total = tEnd + 5.5;
    const int totalSamples = (int) (total * sr);
    juce::AudioBuffer<float> all (2, totalSamples);
    all.clear();

    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    chordharp::Params params;
    size_t e = 0;

    for (int pos = 0; pos < totalSamples; pos += block)
    {
        const int n = juce::jmin (block, totalSamples - pos);
        midi.clear();
        while (e < ev.size() && (int) (ev[e].first * sr) < pos + n)
        {
            midi.addEvent (ev[e].second, juce::jmax (0, (int) (ev[e].first * sr) - pos));
            ++e;
        }
        buf.setSize (2, n, false, false, true);
        eng.process (buf, midi, params);
        all.copyFrom (0, pos, buf, 0, 0, n);
        all.copyFrom (1, pos, buf, 1, 0, n);
    }

    float peak = 0.0f;
    double sumSq = 0.0;
    bool finite = true;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < totalSamples; ++i)
        {
            const float s = all.getSample (c, i);
            if (! std::isfinite (s)) finite = false;
            peak = juce::jmax (peak, std::abs (s));
            sumSq += s * s;
        }

    std::cout << "samples=" << totalSamples << " peak=" << peak
              << " rms=" << std::sqrt (sumSq / (2.0 * totalSamples))
              << " finite=" << (finite ? "yes" : "NO") << std::endl;

    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (outPath);
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0));
    if (w == nullptr) { std::cerr << "écriture wav impossible" << std::endl; return 1; }
    os.release();
    w->writeFromAudioSampleBuffer (all, 0, totalSamples);
    std::cout << "écrit : " << f.getFullPathName() << std::endl;
    return finite ? 0 : 2;
}
