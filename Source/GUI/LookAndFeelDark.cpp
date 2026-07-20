// =============================================================================
//  LookAndFeelDark.cpp — the dark, minimal visual identity.
// =============================================================================
#include "LookAndFeelDark.h"

#include <cmath>

namespace acai
{
    LookAndFeelDark::LookAndFeelDark()
    {
        setColour (juce::ResizableWindow::backgroundColourId, palette::windowBg);
        setColour (juce::Label::textColourId,                 palette::textPrimary);
        setColour (juce::Slider::textBoxTextColourId,         palette::textPrimary);
        setColour (juce::Slider::textBoxBackgroundColourId,   palette::panelBg);
        setColour (juce::Slider::textBoxOutlineColourId,      palette::panelStroke);
        setColour (juce::ComboBox::backgroundColourId,        palette::buttonBg);
        setColour (juce::ComboBox::textColourId,              palette::textPrimary);
        setColour (juce::ComboBox::outlineColourId,           palette::panelStroke);
        setColour (juce::ComboBox::arrowColourId,             palette::accent);
        setColour (juce::PopupMenu::backgroundColourId,       palette::panelBg);
        setColour (juce::PopupMenu::textColourId,             palette::textPrimary);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, palette::accent.withAlpha (0.35f));
        setColour (juce::TextButton::buttonColourId,          palette::buttonBg);
        setColour (juce::TextButton::textColourOffId,         palette::textPrimary);
        setColour (juce::TextButton::textColourOnId,          palette::textPrimary);
        setColour (juce::TextEditor::backgroundColourId,      palette::rollBg);
        setColour (juce::TextEditor::textColourId,            palette::textPrimary);
        setColour (juce::TextEditor::outlineColourId,         palette::panelStroke);
        setColour (juce::TooltipWindow::backgroundColourId,   palette::panelBg);
        setColour (juce::TooltipWindow::textColourId,         palette::textPrimary);
        setColour (juce::TooltipWindow::outlineColourId,      palette::panelStroke);
    }

    void LookAndFeelDark::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                            float pos, float startAngle, float endAngle,
                                            juce::Slider& slider)
    {
        auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float track = radius * 0.82f;
        const float thickness = juce::jmax (2.5f, radius * 0.14f);

        // background track
        juce::Path bg;
        bg.addCentredArc (centre.x, centre.y, track, track, 0.0f, startAngle, endAngle, true);
        g.setColour (palette::panelStroke);
        g.strokePath (bg, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

        // value arc
        juce::Path val;
        val.addCentredArc (centre.x, centre.y, track, track, 0.0f, startAngle, angle, true);
        const bool over = slider.isMouseOverOrDragging();
        g.setColour (over ? palette::accent.brighter (0.2f) : palette::accent);
        g.strokePath (val, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

        // centre dot / pointer
        juce::Point<float> tip (centre.x + track * std::cos (angle - juce::MathConstants<float>::halfPi),
                                centre.y + track * std::sin (angle - juce::MathConstants<float>::halfPi));
        g.setColour (palette::textPrimary);
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (tip));

        g.setColour (palette::panelBg);
        g.fillEllipse (juce::Rectangle<float> (radius * 0.9f, radius * 0.9f).withCentre (centre));
    }

    void LookAndFeelDark::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                const juce::Colour& backgroundColour,
                                                bool highlighted, bool down)
    {
        auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
        const float r = juce::jmin (10.0f, bounds.getHeight() * 0.3f);

        juce::Colour base = backgroundColour;
        if (down)             base = base.brighter (0.18f);
        else if (highlighted) base = base.brighter (0.09f);

        g.setColour (base);
        g.fillRoundedRectangle (bounds, r);
        g.setColour (palette::panelStroke);
        g.drawRoundedRectangle (bounds, r, 1.0f);
    }

    void LookAndFeelDark::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                        int, int, int, int, juce::ComboBox& box)
    {
        auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (0.5f);
        const float r = 7.0f;
        g.setColour (findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (bounds, r);
        g.setColour (box.isMouseOver() ? palette::accent.withAlpha (0.6f) : palette::panelStroke);
        g.drawRoundedRectangle (bounds, r, 1.0f);

        // chevron
        juce::Path p;
        const float cx = (float) width - 16.0f, cy = (float) height * 0.5f;
        p.startNewSubPath (cx - 4, cy - 2);
        p.lineTo (cx, cy + 3);
        p.lineTo (cx + 4, cy - 2);
        g.setColour (palette::accent);
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

    void LookAndFeelDark::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                            bool highlighted, bool)
    {
        auto bounds = b.getLocalBounds().toFloat();
        auto sw = bounds.removeFromLeft (38.0f).withSizeKeepingCentre (34.0f, 18.0f);
        const bool on = b.getToggleState();

        g.setColour (on ? palette::accent : palette::buttonBg);
        g.fillRoundedRectangle (sw, 9.0f);
        g.setColour (palette::panelStroke);
        g.drawRoundedRectangle (sw, 9.0f, 1.0f);

        auto knob = juce::Rectangle<float> (14.0f, 14.0f)
                        .withCentre ({ on ? sw.getRight() - 9.0f : sw.getX() + 9.0f, sw.getCentreY() });
        g.setColour (on ? palette::windowBg : palette::textDim);
        g.fillEllipse (knob);

        g.setFont (13.0f);
        g.setColour (highlighted ? palette::textPrimary : palette::textPrimary.withAlpha (0.85f));
        g.drawText (b.getButtonText(), bounds.withTrimmedLeft (6.0f),
                    juce::Justification::centredLeft, true);
    }

    void LookAndFeelDark::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                            float sliderPos, float, float,
                                            juce::Slider::SliderStyle style, juce::Slider& slider)
    {
        if (style == juce::Slider::LinearHorizontal)
        {
            const float cy = y + h * 0.5f;
            juce::Rectangle<float> track (x + 2.0f, cy - 2.5f, w - 4.0f, 5.0f);
            g.setColour (palette::panelStroke);
            g.fillRoundedRectangle (track, 2.5f);
            g.setColour (palette::accent);
            g.fillRoundedRectangle (track.withWidth (sliderPos - x), 2.5f);
            g.setColour (palette::textPrimary);
            g.fillEllipse (juce::Rectangle<float> (13.0f, 13.0f).withCentre ({ sliderPos, cy }));
        }
        else
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, slider);
        }
    }

    juce::Font LookAndFeelDark::getComboBoxFont (juce::ComboBox&)   { return juce::Font (14.0f); }
    juce::Font LookAndFeelDark::getLabelFont (juce::Label&)         { return juce::Font (14.0f); }
    juce::Font LookAndFeelDark::getPopupMenuFont()                  { return juce::Font (14.0f); }
}
