#include "PluginEditor.h"

using chordharp::Engine;

namespace
{
    // Ordre des colonnes façon Omnichord (cycle des quintes)
    constexpr int columnPitchClass[12] = { 1, 8, 3, 10, 5, 0, 7, 2, 9, 4, 11, 6 };
    const char* const noteNames[12]   = { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const char* const typeSuffix[3]   = { "", "m", "7" };
    const char* const rowLabels[3]    = { "MAJ", "MIN", "7TH" };

    constexpr int editorW = 980;
    constexpr int editorH = 590;
    constexpr int headerH = 58;

    juce::String chordName (int pc, int type)
    {
        return juce::String (noteNames[juce::jlimit (0, 11, pc)]) + typeSuffix[juce::jlimit (0, 2, type)];
    }
}

//==============================================================================
RetroLook::RetroLook()
{
    setColour (juce::Slider::textBoxTextColourId,       ui::cream);
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId,               ui::creamDim);
    setColour (juce::ToggleButton::textColourId,        ui::cream);
    setColour (juce::ToggleButton::tickColourId,        ui::amber);
    setColour (juce::ToggleButton::tickDisabledColourId, ui::creamDim);
}

void RetroLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                  float startAngle, float endAngle, juce::Slider&)
{
    const auto b = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const juce::PathStrokeType stroke (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 3.0f, r - 3.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff3d3630));
    g.strokePath (track, stroke);

    juce::Path value;
    value.addCentredArc (c.x, c.y, r - 3.0f, r - 3.0f, 0.0f, startAngle, angle, true);
    g.setColour (ui::amber);
    g.strokePath (value, stroke);

    const float kr = r - 10.0f;
    g.setGradientFill (juce::ColourGradient (ui::cream, c.x, c.y - kr, ui::creamDim, c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);

    const auto tip = c.getPointOnCircumference (kr * 0.78f, angle);
    g.setColour (ui::ink);
    g.drawLine (c.x, c.y, tip.x, tip.y, 3.0f);
}

//==============================================================================
juce::Rectangle<float> ChordGrid::cellBounds (int row, int col) const
{
    const float labelW = 52.0f;
    const auto area = getLocalBounds().toFloat();
    const float cw = (area.getWidth() - labelW) / 12.0f;
    const float ch = area.getHeight() / 3.0f;
    return { labelW + (float) col * cw, (float) row * ch, cw, ch };
}

bool ChordGrid::cellAt (juce::Point<float> pt, int& row, int& col) const
{
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 12; ++c)
            if (cellBounds (r, c).contains (pt))
            {
                row = r;
                col = c;
                return true;
            }
    return false;
}

void ChordGrid::paint (juce::Graphics& g)
{
    const int curRoot = proc.engine.uiRoot.load();
    const int curType = proc.engine.uiType.load();
    const bool held   = proc.engine.uiHeld.load();

    for (int r = 0; r < 3; ++r)
    {
        const auto rowArea = cellBounds (r, 0);
        g.setColour (ui::creamDim);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (rowLabels[r], juce::Rectangle<float> (0.0f, rowArea.getY(), 46.0f, rowArea.getHeight()),
                    juce::Justification::centredLeft);

        for (int c = 0; c < 12; ++c)
        {
            const int pc = columnPitchClass[c];
            const auto cell = cellBounds (r, c).reduced (4.0f, 6.0f);
            const bool isCurrent = (pc == curRoot && r == curType);

            if (isCurrent && held)
            {
                g.setColour (ui::amber.withAlpha (0.35f));
                g.fillRoundedRectangle (cell.expanded (3.0f), 9.0f);
                g.setColour (ui::amber);
            }
            else
            {
                g.setColour (ui::cream);
            }
            g.fillRoundedRectangle (cell, 7.0f);

            if (isCurrent && ! held)
            {
                g.setColour (ui::amber);
                g.drawRoundedRectangle (cell.reduced (1.0f), 7.0f, 2.5f);
            }

            // petit relief en bas du bouton
            g.setColour (juce::Colours::black.withAlpha (0.18f));
            g.fillRoundedRectangle (cell.withTop (cell.getBottom() - 5.0f), 3.0f);

            g.setColour (ui::ink);
            g.setFont (juce::FontOptions (cell.getWidth() > 40.0f ? 15.0f : 13.0f, juce::Font::bold));
            g.drawText (chordName (pc, r), cell, juce::Justification::centred);
        }
    }
}

void ChordGrid::mouseDown (const juce::MouseEvent& e)
{
    int row = 0, col = 0;
    if (! cellAt (e.position, row, col))
        return;

    pressedKey = Engine::chordKeyFor (columnPitchClass[col], row);
    proc.sendUiMidi (juce::MidiMessage::noteOn (1, pressedKey, (juce::uint8) 100));
}

void ChordGrid::mouseUp (const juce::MouseEvent&)
{
    if (pressedKey >= 0)
        proc.sendUiMidi (juce::MidiMessage::noteOff (1, pressedKey));
    pressedKey = -1;
}

//==============================================================================
int StrumPlate::indexAt (float y) const
{
    const auto area = getLocalBounds().toFloat().reduced (10.0f);
    const float barH = area.getHeight() / (float) Engine::numPlate;
    const int idx = (int) std::floor ((area.getBottom() - y) / barH);
    return juce::jlimit (0, Engine::numPlate - 1, idx);
}

void StrumPlate::hit (int index)
{
    proc.sendUiMidi (juce::MidiMessage::noteOn (1, Engine::plateFirstKey + index, (juce::uint8) 100));
}

void StrumPlate::mouseDown (const juce::MouseEvent& e)
{
    last = indexAt (e.position.y);
    hit (last);
}

void StrumPlate::mouseDrag (const juce::MouseEvent& e)
{
    const int idx = indexAt (e.position.y);
    if (last < 0)
    {
        last = idx;
        hit (idx);
        return;
    }
    if (idx == last)
        return;

    // on joue toutes les cases traversées, comme un vrai glissé de doigt
    const int step = idx > last ? 1 : -1;
    for (int i = last + step; ; i += step)
    {
        hit (i);
        if (i == idx) break;
    }
    last = idx;
}

void StrumPlate::mouseUp (const juce::MouseEvent&)
{
    last = -1;
}

void StrumPlate::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (ui::teal.withAlpha (0.25f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);

    const auto area = bounds.reduced (10.0f);
    const float barH = area.getHeight() / (float) Engine::numPlate;
    const int root = proc.engine.uiRoot.load();
    const int type = proc.engine.uiType.load();

    for (int i = 0; i < Engine::numPlate; ++i)
    {
        const float flash = proc.engine.uiPlateFlash[(size_t) i].load();
        const juce::Rectangle<float> bar (area.getX(), area.getBottom() - (float) (i + 1) * barH, area.getWidth(), barH);
        const auto cell = bar.reduced (2.0f, 2.0f);

        if (flash > 0.02f)
        {
            g.setColour (ui::teal.withAlpha (0.25f * flash));
            g.fillRoundedRectangle (cell.expanded (3.0f, 2.0f), 6.0f);
        }

        g.setColour (ui::teal.withAlpha (0.10f + 0.80f * flash));
        g.fillRoundedRectangle (cell, 5.0f);
        g.setColour (ui::teal.withAlpha (0.35f + 0.5f * flash));
        g.drawRoundedRectangle (cell, 5.0f, 1.0f);

        const int note = Engine::plateNoteFor (root, type, i);
        g.setColour (flash > 0.4f ? ui::ink : ui::creamDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (juce::MidiMessage::getMidiNoteName (note, false, true, 5),
                    cell.reduced (8.0f, 0.0f), juce::Justification::centredRight);
    }
}

//==============================================================================
ChordHarpEditor::ChordHarpEditor (ChordHarpProcessor& p)
    : AudioProcessorEditor (p), proc (p), grid (p), plate (p)
{
    setLookAndFeel (&look);

    addAndMakeVisible (grid);
    addAndMakeVisible (plate);

    holdButton.setButtonText ("HOLD");
    holdButton.setTooltip ("Garde l'accord après avoir relâché la touche");
    addAndMakeVisible (holdButton);
    holdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "hold", holdButton);

    const char* const ids[8] = { "chord", "bass", "strum", "decay", "tone", "vibrato", "reverb", "master" };
    const juce::String names[8] = { "ACCORD", "BASSE", "SLIDE", juce::CharPointer_UTF8 ("DUR\xc3\x89" "E"),
                                    "TIMBRE", "VIBRATO", "REVERB", "VOLUME" };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.slider.setColour (juce::Slider::textBoxTextColourId, ui::cream);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ids[i], k.slider);
    }

    setSize (editorW, editorH);
    startTimerHz (30);
}

ChordHarpEditor::~ChordHarpEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ChordHarpEditor::timerCallback()
{
    for (auto& f : proc.engine.uiPlateFlash)
    {
        const float v = f.load();
        f.store (v > 0.02f ? v * 0.82f : 0.0f);
    }
    grid.repaint();
    plate.repaint();
    repaint (0, 0, getWidth(), headerH);
}

void ChordHarpEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);

    // En-tête
    g.setColour (ui::panel);
    g.fillRect (0, 0, getWidth(), headerH);
    g.setColour (ui::amber);
    g.fillRect (0, headerH - 3, getWidth(), 3);

    g.setColour (ui::cream);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("CHORDHARP", 24, 8, 260, 40, juce::Justification::centredLeft);
    g.setColour (ui::amber);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("by MIDISOUDRINK", 228, 22, 160, 20, juce::Justification::centredLeft);

    const bool held = proc.engine.uiHeld.load();
    g.setColour (held ? ui::amber : ui::creamDim);
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold));
    g.drawText (chordName (proc.engine.uiRoot.load(), proc.engine.uiType.load()),
                getWidth() - 200, 8, 176, 40, juce::Justification::centredRight);

    // Titres de sections
    g.setColour (ui::creamDim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("ACCORDS", grid.getX() + 52, headerH + 8, 120, 16, juce::Justification::centredLeft);
    g.drawText ("SLIDE", plate.getX(), headerH + 8, plate.getWidth(), 16, juce::Justification::centred);

    // Fond de la zone des boutons
    g.setColour (ui::panel);
    g.fillRoundedRectangle (juce::Rectangle<int> (14, 444, getWidth() - 28, 112).toFloat(), 12.0f);

    // Aide
    g.setColour (ui::creamDim.withAlpha (0.8f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("MIDI (notation FL Studio) :  C2-B2 = majeur   |   C3-B3 = mineur   |   C4-B4 = 7e   |   C5-C6 = plaque   |   mod wheel = slide",
                0, getHeight() - 30, getWidth(), 22, juce::Justification::centred);
}

void ChordHarpEditor::resized()
{
    const int top = headerH + 30;
    grid.setBounds (16, top, 640, 340);
    plate.setBounds (690, top, 270, 340);
    holdButton.setBounds (grid.getRight() - 90, headerH + 4, 90, 24);

    auto row = juce::Rectangle<int> (24, 450, getWidth() - 48, 102);
    const int w = row.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto cell = row.removeFromLeft (w);
        k.label.setBounds (cell.removeFromTop (16));
        k.slider.setBounds (cell);
    }
}
