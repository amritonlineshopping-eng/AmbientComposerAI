// =============================================================================
//  PluginEditor.h
//  The full custom dark UI: resizable, FlexBox-laid-out, tooltips everywhere,
//  a hero piano-roll with transport, and the complete control surface wired to
//  the processor's APVTS + generation API.
// =============================================================================
#pragma once

#include "PluginProcessor.h"
#include "GUI/Controls.h"
#include "GUI/LookAndFeelDark.h"
#include "GUI/PianoRollComponent.h"

class AmbientComposerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::ChangeListener,
                                            private juce::Timer
{
public:
    explicit AmbientComposerAudioProcessorEditor (AmbientComposerAudioProcessor&);
    ~AmbientComposerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refreshFromProcessor();
    void updateTransportButtons();
    bool lastPlayingState_ = false;

    AmbientComposerAudioProcessor& processor;
    acai::LookAndFeelDark lnf_;
    juce::TooltipWindow tooltips_ { this, 600 };

    // --- top bar -------------------------------------------------------------
    juce::Label titleLabel_;
    juce::TextButton undoButton_ { "Undo" }, redoButton_ { "Redo" };
    juce::TextButton favButton_ { "Save Fav" };
    juce::ComboBox   favMenu_;

    // --- left panel ----------------------------------------------------------
    std::unique_ptr<acai::LabeledCombo> keyCombo_, scaleCombo_, moodCombo_,
                                        barsCombo_, timeSigCombo_;
    std::unique_ptr<acai::LabeledKnob>  bpmKnob_, chordOctaveKnob_;
    acai::SectionHeader leftHeader_ { "Composition" };

    // --- right panel ---------------------------------------------------------
    std::unique_ptr<acai::LabeledCombo> melodyStyleCombo_, rhythmStyleCombo_, voicingCombo_;
    std::unique_ptr<acai::LabeledKnob>  complexityKnob_, velHumKnob_, timeHumKnob_,
                                        gateHumKnob_, sustainKnob_, melodyOctaveKnob_,
                                        midiChannelKnob_;
    std::unique_ptr<acai::LabeledToggle> octaveDoublingToggle_, autoPlayToggle_;
    acai::SectionHeader rightHeader_ { "Melody & Feel" };
    acai::SectionHeader humanizeHeader_ { "Humanize" };

    // --- center --------------------------------------------------------------
    acai::PianoRollComponent pianoRoll_;
    juce::Label progressionLabel_;
    juce::TextButton playButton_ { "Play" }, stopButton_ { "Stop" };
    juce::ToggleButton loopButton_ { "Loop" };

    // --- bottom bar ----------------------------------------------------------
    juce::TextButton genChordsButton_ { "Generate Chords" };
    juce::TextButton genMelodyButton_ { "Generate Melody" };
    juce::TextButton genBothButton_   { "Generate Both" };

    juce::Label seedCaption_;
    juce::TextEditor seedField_;
    juce::TextButton randomSeedButton_ { "New" }, copySeedButton_ { "Copy" }, pasteSeedButton_ { "Paste" };
    std::unique_ptr<acai::LabeledToggle> lockSeedToggle_;

    std::unique_ptr<acai::MidiDragTile> dragChords_, dragMelody_, dragCombined_;
    juce::TextButton exportButton_ { "Export…" };

    juce::Rectangle<int> leftPanelArea_, rightPanelArea_, centerArea_, bottomArea_, topArea_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbientComposerAudioProcessorEditor)
};
