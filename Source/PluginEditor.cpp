#include "PluginEditor.h"
#include "Presets.h"

using chordharp::Engine;

namespace
{
    // Ordre des colonnes façon Omnichord (cycle des quintes)
    constexpr int columnPitchClass[12] = { 1, 8, 3, 10, 5, 0, 7, 2, 9, 4, 11, 6 };
    const char* const noteNames[12]   = { "C", "Db", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const char* const typeSuffix[3]   = { "", "m", "7" };
    const char* const rowLabels[3]    = { "MAJ", "MIN", "7TH" };

    constexpr int editorW = 1040;
    constexpr int editorH = 700;
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

    setColour (juce::ComboBox::backgroundColourId,           juce::Colour (0xff1a1714));
    setColour (juce::ComboBox::textColourId,                 ui::cream);
    setColour (juce::ComboBox::outlineColourId,              ui::amber.withAlpha (0.45f));
    setColour (juce::ComboBox::arrowColourId,                ui::amber);
    setColour (juce::ComboBox::focusedOutlineColourId,       ui::amber);
    setColour (juce::PopupMenu::backgroundColourId,          ui::panel);
    setColour (juce::PopupMenu::textColourId,                ui::cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::amber);
    setColour (juce::PopupMenu::highlightedTextColourId,     ui::ink);
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
juce::Rectangle<float> PresetBar::cell (int i) const
{
    const auto area = getLocalBounds().toFloat();
    const float gap = 8.0f;
    const float w = (area.getWidth() - gap * (float) (chordharp::numPresets - 1)) / (float) chordharp::numPresets;
    return { area.getX() + (float) i * (w + gap), area.getY(), w, area.getHeight() };
}

void PresetBar::paint (juce::Graphics& g)
{
    const int current = proc.currentPreset.load();

    for (int i = 0; i < chordharp::numPresets; ++i)
    {
        const auto c = cell (i);
        const bool on = (i == current);

        g.setColour (on ? ui::amber : ui::panel);
        g.fillRoundedRectangle (c, 8.0f);
        g.setColour (on ? ui::amber.brighter (0.2f) : ui::amber.withAlpha (0.25f));
        g.drawRoundedRectangle (c.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (on ? ui::ink.withAlpha (0.6f) : ui::creamDim.withAlpha (0.6f));
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (juce::String (i + 1).paddedLeft ('0', 2), c.withHeight (16.0f).translated (0.0f, 4.0f),
                    juce::Justification::centred);

        g.setColour (on ? ui::ink : ui::cream);
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawFittedText (juce::String::fromUTF8 (chordharp::presets[i].name),
                          c.withTrimmedTop (16.0f).reduced (4.0f, 2.0f).toNearestInt(),
                          juce::Justification::centred, 1, 0.8f);
    }
}

void PresetBar::mouseDown (const juce::MouseEvent& e)
{
    for (int i = 0; i < chordharp::numPresets; ++i)
        if (cell (i).contains (e.position))
        {
            proc.applyPreset (i);
            repaint();
            return;
        }
}

//==============================================================================
juce::Rectangle<float> StrumPad::playArea() const
{
    return getLocalBounds().toFloat().reduced (12.0f).withTrimmedBottom (14.0f);
}

int StrumPad::indexAt (float y) const
{
    const auto area = playArea();
    const float barH = area.getHeight() / (float) Engine::numPlate;
    const int idx = (int) std::floor ((area.getBottom() - y) / barH);
    return juce::jlimit (0, Engine::numPlate - 1, idx);
}

void StrumPad::hit (int index)
{
    proc.sendUiMidi (juce::MidiMessage::noteOn (1, Engine::plateFirstKey + index, (juce::uint8) 100));
}

void StrumPad::touch (juce::Point<float> pos, bool isNewTouch)
{
    const auto area = playArea();

    // Horizontal : timbre (envoyé en CC74, comme un contrôleur MPE)
    const float x01 = juce::jlimit (0.0f, 1.0f, (pos.x - area.getX()) / area.getWidth());
    const int cc = juce::roundToInt (x01 * 127.0f);
    if (cc != lastCC)
    {
        proc.sendUiMidi (juce::MidiMessage::controllerEvent (1, 74, cc));
        lastCC = cc;
    }

    // Vertical : les notes de l'accord. On joue toutes les cases traversées, comme un vrai doigt.
    const int idx = indexAt (pos.y);
    if (isNewTouch || last < 0)
    {
        hit (idx);
    }
    else if (idx != last)
    {
        const int step = idx > last ? 1 : -1;
        for (int i = last + step; ; i += step)
        {
            hit (i);
            if (i == idx) break;
        }
    }
    last = idx;

    finger = pos;
    fingerAlpha = 1.0f;
    touching = true;
}

void StrumPad::mouseDown (const juce::MouseEvent& e) { touch (e.position, true); }
void StrumPad::mouseDrag (const juce::MouseEvent& e) { touch (e.position, false); }

void StrumPad::mouseUp (const juce::MouseEvent&)
{
    last = -1;
    touching = false;
}

void StrumPad::tick()
{
    if (! touching)
        fingerAlpha = fingerAlpha > 0.01f ? fingerAlpha * 0.8f : 0.0f;
}

void StrumPad::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    // Surface "verre" d'une seule pièce
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff23312f), b.getX(), b.getY(),
                                             juce::Colour (0xff151c1b), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 14.0f);

    // Teinte horizontale : doux (gauche) -> brillant (droite)
    g.setGradientFill (juce::ColourGradient (ui::teal.withAlpha (0.0f), b.getX(), 0.0f,
                                             ui::amber.withAlpha (0.08f), b.getRight(), 0.0f, false));
    g.fillRoundedRectangle (b, 14.0f);

    g.setColour (ui::teal.withAlpha (0.3f));
    g.drawRoundedRectangle (b.reduced (0.5f), 14.0f, 1.0f);

    const auto area = playArea();
    const float barH = area.getHeight() / (float) Engine::numPlate;
    const float labelW = 30.0f;
    const int root = proc.engine.uiRoot.load();
    const int type = proc.engine.uiType.load();

    for (int i = 0; i < Engine::numPlate; ++i)
    {
        const float yC = area.getBottom() - ((float) i + 0.5f) * barH;
        const float flash = proc.engine.uiPlateFlash[(size_t) i].load();

        // lignes guides discrètes
        g.setColour (ui::cream.withAlpha (0.06f));
        g.drawHorizontalLine (juce::roundToInt (yC), area.getX() + labelW, area.getRight());

        if (flash > 0.02f)
        {
            const juce::Rectangle<float> band (area.getX(), yC - barH * 1.3f, area.getWidth(), barH * 2.6f);
            juce::ColourGradient cg (ui::teal.withAlpha (0.0f), 0.0f, band.getY(),
                                     ui::teal.withAlpha (0.0f), 0.0f, band.getBottom(), false);
            cg.addColour (0.5, ui::teal.withAlpha (0.5f * flash));
            g.setGradientFill (cg);
            g.fillRect (band);

            g.setColour (ui::cream.withAlpha (0.85f * flash));
            g.drawLine (area.getX() + labelW, yC, area.getRight(), yC, 1.5f);
        }

        const int note = Engine::plateNoteFor (root, type, i);
        g.setColour (ui::creamDim.withAlpha (0.4f + 0.6f * flash));
        g.setFont (juce::FontOptions (10.0f));
        g.drawText (juce::MidiMessage::getMidiNoteName (note, false, true, 5),
                    juce::Rectangle<float> (area.getX(), yC - 7.0f, labelW, 14.0f), juce::Justification::centredLeft);
    }

    // Doigt
    if (fingerAlpha > 0.01f)
    {
        const float r = 30.0f;
        g.setGradientFill (juce::ColourGradient (ui::cream.withAlpha (0.55f * fingerAlpha), finger.x, finger.y,
                                                 ui::cream.withAlpha (0.0f), finger.x + r, finger.y, true));
        g.fillEllipse (finger.x - r, finger.y - r, r * 2.0f, r * 2.0f);
    }

    // Repère du timbre actuel (CC74) en bas
    const float bx = area.getX() + labelW + (area.getWidth() - labelW) * proc.engine.uiBright.load();
    g.setColour (ui::amber.withAlpha (0.8f));
    g.fillEllipse (bx - 3.0f, b.getBottom() - 15.0f, 6.0f, 6.0f);

    g.setColour (ui::creamDim.withAlpha (0.6f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("doux", juce::Rectangle<float> (area.getX() + labelW, b.getBottom() - 20.0f, 60.0f, 16.0f),
                juce::Justification::centredLeft);
    g.drawText ("brillant", juce::Rectangle<float> (area.getRight() - 60.0f, b.getBottom() - 20.0f, 60.0f, 16.0f),
                juce::Justification::centredRight);
}

//==============================================================================
ChordHarpEditor::ChordHarpEditor (ChordHarpProcessor& p)
    : AudioProcessorEditor (p), proc (p), presetBar (p), grid (p), pad (p)
{
    setLookAndFeel (&look);

    addAndMakeVisible (presetBar);
    addAndMakeVisible (grid);
    addAndMakeVisible (pad);

    holdButton.setButtonText ("HOLD");
    holdButton.setTooltip ("Garde l'accord apr\xc3\xa8s avoir rel\xc3\xa2" "ch\xc3\xa9 la touche");
    addAndMakeVisible (holdButton);
    holdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "hold", holdButton);

    auto setupBox = [this] (juce::ComboBox& box, juce::Label& label, const juce::String& title,
                            const char* const* names, int count, const char* paramId,
                            std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>& att)
    {
        for (int i = 0; i < count; ++i)
            box.addItem (juce::String::fromUTF8 (names[i]), i + 1);
        addAndMakeVisible (box);

        label.setText (title, juce::dontSendNotification);
        label.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);

        att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramId, box);
    };

    setupBox (plateVoiceBox, plateVoiceLabel, "SON DU SLIDE", chordharp::plateVoiceNames, chordharp::NumPlateVoices,
              "plateVoice", plateVoiceAttachment);
    setupBox (padVoiceBox, padVoiceLabel, "SON DE L'ACCORD", chordharp::padVoiceNames, chordharp::NumPadVoices,
              "padVoice", padVoiceAttachment);

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

    if (shownPreset != proc.currentPreset.load())
    {
        shownPreset = proc.currentPreset.load();
        presetBar.repaint();
    }

    pad.tick();
    grid.repaint();
    pad.repaint();
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
    g.drawText ("PRESETS", presetBar.getX(), presetBar.getY() - 20, 200, 16, juce::Justification::centredLeft);
    g.drawText ("ACCORDS", grid.getX() + 52, grid.getY() - 22, 120, 16, juce::Justification::centredLeft);
    g.drawText ("SLIDE", pad.getX(), pad.getY() - 22, pad.getWidth(), 16, juce::Justification::centred);

    // Fond de la zone des réglages
    g.setColour (ui::panel);
    g.fillRoundedRectangle (juce::Rectangle<int> (16, 516, getWidth() - 32, 132).toFloat(), 12.0f);
    g.setColour (ui::cream.withAlpha (0.08f));
    g.drawVerticalLine (236, 530.0f, 634.0f);

    // Aide
    g.setColour (ui::creamDim.withAlpha (0.8f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Pad SLIDE : haut/bas = notes, gauche/droite = timbre.   MIDI (FL Studio) : C2-B2 majeur | C3-B3 mineur | C4-B4 7e | C5-C6 plaque | mod wheel = slide | CC74 = timbre",
                0, getHeight() - 40, getWidth(), 22, juce::Justification::centred);
}

void ChordHarpEditor::resized()
{
    presetBar.setBounds (16, headerH + 30, getWidth() - 32, 50);

    const int top = headerH + 30 + 50 + 34;
    grid.setBounds (16, top, 660, 330);
    pad.setBounds (700, top, getWidth() - 716, 330);
    holdButton.setBounds (grid.getRight() - 90, top - 28, 90, 24);

    plateVoiceLabel.setBounds (34, 528, 190, 16);
    plateVoiceBox.setBounds (34, 546, 190, 28);
    padVoiceLabel.setBounds (34, 584, 190, 16);
    padVoiceBox.setBounds (34, 602, 190, 28);

    auto row = juce::Rectangle<int> (248, 522, getWidth() - 248 - 28, 120);
    const int w = row.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto cell = row.removeFromLeft (w);
        k.label.setBounds (cell.removeFromTop (16));
        k.slider.setBounds (cell);
    }
}
