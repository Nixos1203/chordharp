#pragma once

#include "PluginProcessor.h"

namespace ui
{
    const juce::Colour bg       { 0xff1c1916 };
    const juce::Colour panel    { 0xff27231f };
    const juce::Colour cream    { 0xffefe6d2 };
    const juce::Colour creamDim { 0xffb9ae98 };
    const juce::Colour amber    { 0xfff2a541 };
    const juce::Colour teal     { 0xff57d6c9 };
    const juce::Colour ink      { 0xff2b2621 };
}

//==============================================================================
class RetroLook : public juce::LookAndFeel_V4
{
public:
    RetroLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

//==============================================================================
/** Barre de presets de sons. */
class PresetBar : public juce::Component
{
public:
    explicit PresetBar (ChordHarpProcessor& p) : proc (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> cell (int i) const;
    ChordHarpProcessor& proc;
};

//==============================================================================
/** Grille 3 x 12 de boutons d'accords, façon Omnichord. */
class ChordGrid : public juce::Component
{
public:
    explicit ChordGrid (ChordHarpProcessor& p) : proc (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> cellBounds (int row, int col) const;
    bool cellAt (juce::Point<float> pt, int& row, int& col) const;

    ChordHarpProcessor& proc;
    int pressedKey = -1;
};

//==============================================================================
/** Le pad de slide, d'une seule pièce comme la strumplate de l'Omnichord :
    vertical = notes de l'accord (grave en bas), horizontal = timbre (doux à gauche, brillant à droite). */
class StrumPad : public juce::Component
{
public:
    explicit StrumPad (ChordHarpProcessor& p) : proc (p) {}

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

    void tick();

private:
    juce::Rectangle<float> playArea() const;
    int indexAt (float y) const;
    void touch (juce::Point<float> pos, bool isNewTouch);
    void hit (int index);

    ChordHarpProcessor& proc;
    int last = -1;
    int lastCC = -1;
    bool touching = false;
    juce::Point<float> finger;
    float fingerAlpha = 0.0f;
};

//==============================================================================
class ChordHarpEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ChordHarpEditor (ChordHarpProcessor&);
    ~ChordHarpEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ChordHarpProcessor& proc;
    RetroLook look;
    PresetBar presetBar;
    ChordGrid grid;
    StrumPad pad;

    juce::ToggleButton holdButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> holdAttachment;

    juce::Label plateVoiceLabel, padVoiceLabel;
    juce::ComboBox plateVoiceBox, padVoiceBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> plateVoiceAttachment, padVoiceAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::array<Knob, 8> knobs;

    int shownPreset = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChordHarpEditor)
};
