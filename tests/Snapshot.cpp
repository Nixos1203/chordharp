// Génère une capture PNG de l'interface (sans DAW ni fenêtre).
// Usage : ChordHarpSnapshot capture.png

#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    ChordHarpProcessor proc;
    proc.prepareToPlay (44100.0, 512);

    // Simule un La mineur tenu
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, chordharp::Engine::chordKeyFor (9, chordharp::Minor), 0.8f), 0);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 74, 90), 0);
    juce::AudioBuffer<float> buffer (2, 512);
    proc.applyPreset (argc > 2 ? juce::String (argv[2]).getIntValue() : 2);
    proc.processBlock (buffer, midi);

    // Quelques cases de la plaque "allumées", comme pendant un slide
    const float flashes[] = { 0.0f, 0.0f, 0.1f, 0.25f, 0.45f, 0.7f, 1.0f, 0.0f };
    for (int i = 0; i < 8; ++i)
        proc.engine.uiPlateFlash[(size_t) i].store (flashes[i]);

    std::unique_ptr<juce::AudioProcessorEditor> editor (proc.createEditor());
    const auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.5f / 0.8f * 0.8f);

    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (argc > 1 ? argv[1] : "snapshot.png");
    out.deleteFile();
    juce::FileOutputStream os (out);
    juce::PNGImageFormat png;
    if (! png.writeImageToStream (img, os)) { std::cerr << "PNG impossible" << std::endl; return 1; }
    std::cout << "écrit : " << out.getFullPathName() << std::endl;
    return 0;
}
