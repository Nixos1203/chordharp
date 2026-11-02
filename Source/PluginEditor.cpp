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

    constexpr int editorW = 1100;
    constexpr int editorH = 900;
    constexpr int headerH = 58;
    constexpr int panelH = 136;
    constexpr int sourcesY = 568;
    constexpr int fxY = 724;
    constexpr int comboColW = 200;

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
    const int perRow = 10;
    const float gap = 8.0f;
    const float w = (area.getWidth() - gap * (float) (perRow - 1)) / (float) perRow;
    const float h = (area.getHeight() - gap) * 0.5f;
    const int row = i / perRow, col = i % perRow;
    return { area.getX() + (float) col * (w + gap), area.getY() + (float) row * (h + gap), w, h };
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

        g.setColour (on ? ui::ink.withAlpha (0.65f) : ui::teal.withAlpha (0.75f));
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (juce::String::fromUTF8 (chordharp::presets[i].category),
                    c.withHeight (14.0f).translated (0.0f, 4.0f), juce::Justification::centred);

        g.setColour (on ? ui::ink : ui::cream);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawFittedText (juce::String::fromUTF8 (chordharp::presets[i].name),
                          c.withTrimmedTop (16.0f).reduced (4.0f, 2.0f).toNearestInt(),
                          juce::Justification::centred, 2, 0.85f);
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

    auto items = [] (const char* const* names, int count)
    {
        juce::StringArray a;
        for (int i = 0; i < count; ++i) a.add (juce::String::fromUTF8 (names[i]));
        return a;
    };

    juce::StringArray layerItems { juce::String::fromUTF8 ("Aucune") };
    layerItems.addArray (items (chordharp::plateVoiceNames, chordharp::NumPlateVoices));

    setupCombo (plateCombo,    "SON DU SLIDE", items (chordharp::plateVoiceNames, chordharp::NumPlateVoices), "plateVoice");
    setupCombo (layerCombo,    "COUCHE",       layerItems, "layerVoice");
    setupCombo (layerOctCombo, "OCTAVE",       items (chordharp::layerOctaveNames, 4), "layerOct");
    setupCombo (padCombo,      "SON DE L'ACCORD", items (chordharp::padVoiceNames, chordharp::NumPadVoices), "padVoice");
    setupCombo (delayCombo,    "DELAY",        items (chordharp::delayDivNames, chordharp::numDelayDivs), "delayDiv");

    const char* const srcIds[8] = { "chord", "bass", "strum", "layerLevel", "attack", "decay", "tone", "vibrato" };
    const juce::String srcNames[8] = { "ACCORD", "BASSE", "SLIDE", "COUCHE", "ATTAQUE",
                                       juce::CharPointer_UTF8 ("DUR\xc3\x89" "E"), "TIMBRE", "VIBRATO" };
    for (size_t i = 0; i < sourceKnobs.size(); ++i)
        setupKnob (sourceKnobs[i], srcNames[i], srcIds[i]);

    const char* const fxIds[11] = { "cutoff", "reso", "lfoRate", "lfoDepth", "drive", "chorus",
                                    "delayFb", "delayMix", "reverb", "roomSize", "master" };
    const juce::String fxNames[11] = { "FILTRE", juce::CharPointer_UTF8 ("R\xc3\x89SO"), "LFO VIT.", "LFO PROF.",
                                       "DRIVE", "CHORUS", "FEEDBACK", "MIX DELAY", "REVERB", "TAILLE", "VOLUME" };
    for (size_t i = 0; i < fxKnobs.size(); ++i)
        setupKnob (fxKnobs[i], fxNames[i], fxIds[i]);

    setResizable (true, true);
    setResizeLimits (juce::roundToInt (editorW * 0.55f), juce::roundToInt (editorH * 0.55f),
                     juce::roundToInt (editorW * 1.4f),  juce::roundToInt (editorH * 1.4f));
    getConstrainer()->setFixedAspectRatio ((double) editorW / (double) editorH);
    setSize (juce::roundToInt (editorW * 0.8f), juce::roundToInt (editorH * 0.8f));
    startTimerHz (30);
}

float ChordHarpEditor::uiScale() const
{
    return (float) getWidth() / (float) editorW;
}

void ChordHarpEditor::setupCombo (Combo& c, const juce::String& title, const juce::StringArray& itemNames, const char* paramId)
{
    c.box.addItemList (itemNames, 1);
    addAndMakeVisible (c.box);

    c.label.setText (title, juce::dontSendNotification);
    c.label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    c.label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (c.label);

    c.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramId, c.box);
}

void ChordHarpEditor::setupKnob (Knob& k, const juce::String& title, const char* paramId)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 18);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxTextColourId, ui::cream);
    addAndMakeVisible (k.slider);

    k.label.setText (title, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
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
    repaint (0, 0, getWidth(), juce::roundToInt ((float) headerH * uiScale()) + 1);
}

void ChordHarpEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::bg);
    g.addTransform (juce::AffineTransform::scale (uiScale()));

    // En-tête
    g.setColour (ui::panel);
    g.fillRect (0, 0, editorW, headerH);
    g.setColour (ui::amber);
    g.fillRect (0, headerH - 3, editorW, 3);

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
                editorW - 200, 8, 176, 40, juce::Justification::centredRight);

    // Titres de sections
    g.setColour (ui::creamDim);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("PRESETS", presetBar.getX(), presetBar.getY() - 20, 200, 16, juce::Justification::centredLeft);
    g.drawText ("ACCORDS", grid.getX() + 52, grid.getY() - 22, 120, 16, juce::Justification::centredLeft);
    g.drawText ("SLIDE", pad.getX(), pad.getY() - 22, pad.getWidth(), 16, juce::Justification::centred);

    // Panneaux de réglages
    for (auto y : { sourcesY, fxY })
    {
        g.setColour (ui::panel);
        g.fillRoundedRectangle (juce::Rectangle<int> (16, y, editorW - 32, panelH).toFloat(), 12.0f);
    }
    g.setColour (ui::cream.withAlpha (0.08f));
    g.drawVerticalLine (comboColW + 30, (float) sourcesY + 12.0f, (float) (sourcesY + panelH - 12));
    g.drawVerticalLine (comboColW + 30, (float) fxY + 12.0f, (float) (fxY + panelH - 12));

    g.setColour (ui::amber.withAlpha (0.8f));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("SOURCES", 28, sourcesY - 18, 200, 16, juce::Justification::centredLeft);
    g.drawText ("EFFETS DU SLIDE  /  GLOBAL", 28, fxY - 18, 300, 16, juce::Justification::centredLeft);

    // Aide
    g.setColour (ui::creamDim.withAlpha (0.8f));
    g.setFont (juce::FontOptions (12.0f));
    g.drawText ("Pad SLIDE : haut/bas = notes, gauche/droite = timbre.   MIDI (FL Studio) : C2-B2 majeur | C3-B3 mineur | C4-B4 7e | C5-C6 plaque | mod wheel = slide | CC74 = timbre",
                0, editorH - 32, editorW, 22, juce::Justification::centred);
}

void ChordHarpEditor::resized()
{
    presetBar.setBounds (16, headerH + 30, editorW - 32, 104);

    const int top = presetBar.getBottom() + 34;
    grid.setBounds (16, top, 680, 300);
    pad.setBounds (720, top, editorW - 736, 300);
    holdButton.setBounds (grid.getRight() - 90, top - 28, 90, 24);

    // Panneau sources : 3 menus + 8 boutons
    {
        const int x = 30, y = sourcesY + 10;
        plateCombo.label.setBounds (x, y, comboColW, 14);
        plateCombo.box.setBounds   (x, y + 15, comboColW, 24);
        layerCombo.label.setBounds (x, y + 42, comboColW - 80, 14);
        layerCombo.box.setBounds   (x, y + 57, comboColW - 80, 24);
        layerOctCombo.label.setBounds (x + comboColW - 74, y + 42, 74, 14);
        layerOctCombo.box.setBounds   (x + comboColW - 74, y + 57, 74, 24);
        padCombo.label.setBounds   (x, y + 84, comboColW, 14);
        padCombo.box.setBounds     (x, y + 99, comboColW, 24);

        auto row = juce::Rectangle<int> (comboColW + 44, sourcesY + 6, editorW - comboColW - 44 - 28, panelH - 12);
        const int w = row.getWidth() / (int) sourceKnobs.size();
        for (auto& k : sourceKnobs)
        {
            auto cell = row.removeFromLeft (w);
            k.label.setBounds (cell.removeFromTop (16));
            k.slider.setBounds (cell);
        }
    }

    // Panneau effets : menu delay + 11 boutons
    {
        delayCombo.label.setBounds (30, fxY + 40, comboColW, 14);
        delayCombo.box.setBounds   (30, fxY + 55, comboColW, 24);

        auto row = juce::Rectangle<int> (comboColW + 44, fxY + 6, editorW - comboColW - 44 - 28, panelH - 12);
        const int w = row.getWidth() / (int) fxKnobs.size();
        for (auto& k : fxKnobs)
        {
            auto cell = row.removeFromLeft (w);
            k.label.setBounds (cell.removeFromTop (16));
            k.slider.setBounds (cell);
        }
    }

    // Mise à l'échelle : tout est placé en coordonnées "design", puis agrandi/réduit
    const auto t = juce::AffineTransform::scale (uiScale());
    for (auto* c : getChildren())
        if (dynamic_cast<juce::ResizableCornerComponent*> (c) == nullptr
             && dynamic_cast<juce::ResizableBorderComponent*> (c) == nullptr)
            c->setTransform (t);
}
