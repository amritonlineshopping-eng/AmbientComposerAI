// =============================================================================
//  LookAndFeelDark.h
//  The plugin's visual identity: a modern minimal dark theme (Scaler/Output/
//  Arturia direction). Centralizes every colour so the whole UI stays coherent.
// =============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace acai
{
    /// Central palette. All GUI components use these — never ad-hoc colours.
    namespace palette
    {
        inline const juce::Colour windowBg     { 0xff121319 };  ///< deep near-black blue
        inline const juce::Colour panelBg      { 0xff191b23 };  ///< raised panels
        inline const juce::Colour panelStroke  { 0xff262a36 };  ///< hairline borders
        inline const juce::Colour textPrimary  { 0xffe8eaf2 };
        inline const juce::Colour textDim      { 0xff8b90a3 };
        inline const juce::Colour accent       { 0xff7c9cff };  ///< soft periwinkle
        inline const juce::Colour accentWarm   { 0xffd7a26c };  ///< warm highlight
        inline const juce::Colour chordNote    { 0xff5b79d9 };  ///< piano-roll chords
        inline const juce::Colour melodyNote   { 0xffe6b36a };  ///< piano-roll melody
        inline const juce::Colour playhead     { 0xffe8eaf2 };
        inline const juce::Colour rollBg       { 0xff0d0e13 };
        inline const juce::Colour rollGridBar  { 0xff2a2e3c };
        inline const juce::Colour rollGridBeat { 0xff1a1d26 };
        inline const juce::Colour buttonBg     { 0xff232734 };
        inline const juce::Colour buttonHover  { 0xff2d3243 };
        inline const juce::Colour danger       { 0xffd96a6a };
    }

    /// Custom LookAndFeel: flat rotary knobs with an arc value ring, pill
    /// buttons, clean combo boxes, subtle focus states. Animations are done
    /// by the components; this class keeps drawing stateless.
    class LookAndFeelDark : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeelDark();

        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPosProportional, float rotaryStartAngle,
                               float rotaryEndAngle, juce::Slider&) override;

        void drawButtonBackground (juce::Graphics&, juce::Button&,
                                   const juce::Colour& backgroundColour,
                                   bool shouldDrawButtonAsHighlighted,
                                   bool shouldDrawButtonAsDown) override;

        void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                           int buttonX, int buttonY, int buttonW, int buttonH,
                           juce::ComboBox&) override;

        void drawToggleButton (juce::Graphics&, juce::ToggleButton&,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               juce::Slider::SliderStyle, juce::Slider&) override;

        juce::Font getComboBoxFont (juce::ComboBox&) override;
        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getPopupMenuFont() override;
    };
}
