// =============================================================================
//  Controls.h
//  Small reusable, APVTS-attached widgets for the editor: a labelled combo,
//  a labelled rotary knob, a labelled toggle, a section header, and a MIDI
//  drag-out tile. Header-only.
// =============================================================================
#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include "LookAndFeelDark.h"

namespace acai
{
    using APVTS = juce::AudioProcessorValueTreeState;

    // A caption above a combo box, attached to a choice parameter.
    class LabeledCombo : public juce::Component
    {
    public:
        LabeledCombo (APVTS& s, const juce::String& paramID,
                      const juce::String& caption, const juce::StringArray& items,
                      const juce::String& tooltip)
        {
            label_.setText (caption, juce::dontSendNotification);
            label_.setJustificationType (juce::Justification::centredLeft);
            label_.setColour (juce::Label::textColourId, palette::textDim);
            label_.setFont (juce::Font (12.0f));
            addAndMakeVisible (label_);

            combo_.addItemList (items, 1);
            combo_.setTooltip (tooltip);
            addAndMakeVisible (combo_);
            attachment_ = std::make_unique<APVTS::ComboBoxAttachment> (s, paramID, combo_);
        }

        void resized() override
        {
            auto b = getLocalBounds();
            label_.setBounds (b.removeFromTop (16));
            combo_.setBounds (b.reduced (0, 1));
        }

        juce::ComboBox& box() { return combo_; }

    private:
        juce::Label label_;
        juce::ComboBox combo_;
        std::unique_ptr<APVTS::ComboBoxAttachment> attachment_;
    };

    // A rotary knob with a caption above and value below, attached to a float/int param.
    class LabeledKnob : public juce::Component
    {
    public:
        LabeledKnob (APVTS& s, const juce::String& paramID,
                     const juce::String& caption, const juce::String& tooltip)
        {
            label_.setText (caption, juce::dontSendNotification);
            label_.setJustificationType (juce::Justification::centred);
            label_.setColour (juce::Label::textColourId, palette::textDim);
            label_.setFont (juce::Font (12.0f));
            addAndMakeVisible (label_);

            knob_.setSliderStyle (juce::Slider::RotaryVerticalDrag);
            knob_.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
            knob_.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
            knob_.setTooltip (tooltip);
            addAndMakeVisible (knob_);
            attachment_ = std::make_unique<APVTS::SliderAttachment> (s, paramID, knob_);
        }

        void resized() override
        {
            auto b = getLocalBounds();
            label_.setBounds (b.removeFromTop (16));
            knob_.setBounds (b);
        }

    private:
        juce::Label label_;
        juce::Slider knob_;
        std::unique_ptr<APVTS::SliderAttachment> attachment_;
    };

    // A toggle switch with a caption, attached to a bool param.
    class LabeledToggle : public juce::Component
    {
    public:
        LabeledToggle (APVTS& s, const juce::String& paramID,
                       const juce::String& caption, const juce::String& tooltip)
        {
            toggle_.setButtonText (caption);
            toggle_.setTooltip (tooltip);
            addAndMakeVisible (toggle_);
            attachment_ = std::make_unique<APVTS::ButtonAttachment> (s, paramID, toggle_);
        }

        void resized() override { toggle_.setBounds (getLocalBounds()); }

    private:
        juce::ToggleButton toggle_;
        std::unique_ptr<APVTS::ButtonAttachment> attachment_;
    };

    class SectionHeader : public juce::Component
    {
    public:
        explicit SectionHeader (const juce::String& text) : text_ (text) {}
        void paint (juce::Graphics& g) override
        {
            g.setColour (palette::accent);
            g.setFont (juce::Font (12.0f, juce::Font::bold));
            g.drawText (text_.toUpperCase(), getLocalBounds(), juce::Justification::centredLeft);
            auto line = getLocalBounds().toFloat();
            const float y = line.getBottom() - 1.0f;
            g.setColour (palette::panelStroke);
            g.drawLine (line.getX(), y, line.getRight(), y, 1.0f);
        }
    private:
        juce::String text_;
    };

    // A drag-out tile that writes a fresh temp .mid and starts an external drag.
    class MidiDragTile : public juce::Component,
                         public juce::SettableTooltipClient
    {
    public:
        MidiDragTile (const juce::String& caption, juce::Colour accent,
                      std::function<juce::File()> fileProvider)
            : caption_ (caption), accent_ (accent), fileProvider_ (std::move (fileProvider))
        {
            setTooltip ("Drag this into FL Studio (or any DAW) to drop the " + caption + " MIDI.");
        }

        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat().reduced (1.0f);
            g.setColour (hover_ ? accent_.withAlpha (0.22f) : palette::buttonBg);
            g.fillRoundedRectangle (b, 6.0f);

            juce::Path dashed;
            dashed.addRoundedRectangle (b.reduced (1.0f), 6.0f);
            const float dashes[] = { 4.0f, 3.0f };
            g.setColour (accent_.withAlpha (hover_ ? 0.9f : 0.6f));
            juce::Path stroked;
            juce::PathStrokeType (1.2f).createDashedStroke (stroked, dashed, dashes, 2);
            g.fillPath (stroked);

            g.setColour (palette::textPrimary);
            g.setFont (juce::Font (12.0f, juce::Font::bold));
            g.drawText (caption_, b.reduced (4).removeFromTop (b.getHeight() * 0.55f),
                        juce::Justification::centredBottom, true);
            g.setColour (palette::textDim);
            g.setFont (juce::Font (10.0f));
            g.drawText (juce::String::fromUTF8 ("\xe2\x87\xa3  drag out"),
                        b.reduced (4).removeFromBottom (b.getHeight() * 0.4f),
                        juce::Justification::centredTop, true);
        }

        void mouseEnter (const juce::MouseEvent&) override { hover_ = true;  repaint(); }
        void mouseExit  (const juce::MouseEvent&) override { hover_ = false; repaint(); }

        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (dragging_ || e.getDistanceFromDragStart() < 6) return;
            dragging_ = true;

            if (fileProvider_)
            {
                const juce::File f = fileProvider_();
                if (f.existsAsFile())
                {
                    juce::StringArray files;
                    files.add (f.getFullPathName());
                    juce::DragAndDropContainer::performExternalDragDropOfFiles (files, false, this);
                }
            }
        }

        void mouseUp (const juce::MouseEvent&) override { dragging_ = false; }

    private:
        juce::String caption_;
        juce::Colour accent_;
        std::function<juce::File()> fileProvider_;
        bool hover_ = false, dragging_ = false;
    };
}
