// =============================================================================
//  PluginEditor.cpp — full custom dark UI implementation.
// =============================================================================
#include "PluginEditor.h"

using namespace acai;

AmbientComposerAudioProcessorEditor::AmbientComposerAudioProcessorEditor (
        AmbientComposerAudioProcessor& p)
    : AudioProcessorEditor (p), processor (p)
{
    setLookAndFeel (&lnf_);
    auto& s = processor.apvts;

    // --- top bar -------------------------------------------------------------
    titleLabel_.setText ("Ambient Composer AI", juce::dontSendNotification);
    titleLabel_.setFont (juce::Font (18.0f, juce::Font::bold));
    titleLabel_.setColour (juce::Label::textColourId, palette::textPrimary);
    addAndMakeVisible (titleLabel_);

    undoButton_.setTooltip ("Step back to the previous generation.");
    redoButton_.setTooltip ("Step forward again.");
    undoButton_.onClick = [this] { processor.undo(); };
    redoButton_.onClick = [this] { processor.redo(); };
    addAndMakeVisible (undoButton_);
    addAndMakeVisible (redoButton_);

    favButton_.setTooltip ("Bookmark this generation (its seed + all settings) as a favourite.");
    favButton_.onClick = [this]
    {
        processor.addFavorite ({});
        refreshFromProcessor();
    };
    addAndMakeVisible (favButton_);

    favMenu_.setTextWhenNothingSelected ("Favorites");
    favMenu_.setTooltip ("Recall a saved favourite.");
    favMenu_.onChange = [this]
    {
        const int idx = favMenu_.getSelectedItemIndex();
        if (idx >= 0) processor.recallFavorite (idx);
    };
    addAndMakeVisible (favMenu_);

    // --- left panel ----------------------------------------------------------
    addAndMakeVisible (leftHeader_);
    keyCombo_     = std::make_unique<LabeledCombo> (s, ParamIDs::key, "Key",
                        ParamChoices::keys, "Musical key. 'Random' picks one from the seed.");
    scaleCombo_   = std::make_unique<LabeledCombo> (s, ParamIDs::scale, "Scale",
                        ParamChoices::scales, "Scale/mode. 'Auto' chooses one that fits the mood.");
    moodCombo_    = std::make_unique<LabeledCombo> (s, ParamIDs::mood, "Mood",
                        ParamChoices::moods, "The emotional character. Drives scale, chords, contour, density, dynamics and note length.");
    barsCombo_    = std::make_unique<LabeledCombo> (s, ParamIDs::barsPerChord, "Bars / Chord",
                        ParamChoices::barsPerChordChoices, "How long each chord is held. 2 bars x 4 chords = an 8-bar ambient loop.");
    timeSigCombo_ = std::make_unique<LabeledCombo> (s, ParamIDs::timeSig, "Time Sig",
                        ParamChoices::timeSigs, "Time signature written into the exported MIDI.");
    bpmKnob_      = std::make_unique<LabeledKnob>  (s, ParamIDs::bpm, "BPM",
                        "Tempo (40-140). Slow suits ambient. Written into the exported MIDI.");
    chordOctaveKnob_ = std::make_unique<LabeledKnob> (s, ParamIDs::chordOctave, "Chord Oct",
                        "Register the pad sits in. Lower = deeper.");
    for (juce::Component* c : { (juce::Component*) keyCombo_.get(), (juce::Component*) scaleCombo_.get(),
                                (juce::Component*) moodCombo_.get(), (juce::Component*) barsCombo_.get(),
                                (juce::Component*) timeSigCombo_.get() }) addAndMakeVisible (c);
    addAndMakeVisible (*bpmKnob_);
    addAndMakeVisible (*chordOctaveKnob_);

    // --- right panel ---------------------------------------------------------
    addAndMakeVisible (rightHeader_);
    addAndMakeVisible (humanizeHeader_);
    melodyStyleCombo_ = std::make_unique<LabeledCombo> (s, ParamIDs::melodyStyle, "Melody Style",
                        ParamChoices::melodyStyles, "Shape of the lead: motif+variation, flowing lines, minimal, arpeggiated, or call & response.");
    rhythmStyleCombo_ = std::make_unique<LabeledCombo> (s, ParamIDs::rhythmStyle, "Rhythm",
                        ParamChoices::rhythmStyles, "Rhythmic feel of the melody. 'Free Ambient' is spacious and rubato.");
    voicingCombo_     = std::make_unique<LabeledCombo> (s, ParamIDs::voicingWidth, "Voicing",
                        ParamChoices::voicingWidths, "Chord spacing. Open/Spread suit lush ambient pads.");
    complexityKnob_   = std::make_unique<LabeledKnob> (s, ParamIDs::complexity, "Complexity",
                        "0% = sparse, all chord tones, long notes. 100% = active, syncopated, ornamented (still in-key).");
    velHumKnob_       = std::make_unique<LabeledKnob> (s, ParamIDs::velHumanize, "Velocity",
                        "Humanize: random variation in note loudness.");
    timeHumKnob_      = std::make_unique<LabeledKnob> (s, ParamIDs::timeHumanize, "Timing",
                        "Humanize: subtle push/pull of note timing (never before the bar).");
    gateHumKnob_      = std::make_unique<LabeledKnob> (s, ParamIDs::gateHumanize, "Gate",
                        "Humanize: variation in note lengths.");
    sustainKnob_      = std::make_unique<LabeledKnob> (s, ParamIDs::sustainOverlap, "Sustain",
                        "How far chords ring past their boundary, overlapping into the next for a legato pad.");
    melodyOctaveKnob_ = std::make_unique<LabeledKnob> (s, ParamIDs::melodyOctave, "Melody Oct",
                        "Register of the melody, above the chords.");
    midiChannelKnob_  = std::make_unique<LabeledKnob> (s, ParamIDs::midiChannel, "MIDI Ch",
                        "MIDI channel written into exported files.");
    octaveDoublingToggle_ = std::make_unique<LabeledToggle> (s, ParamIDs::octaveDoubling, "Octave Doubling",
                        "Double the chord root an octave down for extra depth.");
    autoPlayToggle_       = std::make_unique<LabeledToggle> (s, ParamIDs::autoPlay, "Auto-Play",
                        "Start the preview automatically after each generation.");
    for (juce::Component* c : { (juce::Component*) melodyStyleCombo_.get(),
                                (juce::Component*) rhythmStyleCombo_.get(),
                                (juce::Component*) voicingCombo_.get() })
        addAndMakeVisible (c);
    for (juce::Component* k : { (juce::Component*) complexityKnob_.get(), (juce::Component*) velHumKnob_.get(),
                                (juce::Component*) timeHumKnob_.get(), (juce::Component*) gateHumKnob_.get(),
                                (juce::Component*) sustainKnob_.get(), (juce::Component*) melodyOctaveKnob_.get(),
                                (juce::Component*) midiChannelKnob_.get() })
        addAndMakeVisible (k);
    addAndMakeVisible (*octaveDoublingToggle_);
    addAndMakeVisible (*autoPlayToggle_);

    // --- center: piano roll + transport --------------------------------------
    addAndMakeVisible (pianoRoll_);
    pianoRoll_.setPlayheadSource ([this]
    {
        return processor.previewIsPlaying() ? processor.previewPlayheadTicks() : -1.0;
    });

    progressionLabel_.setJustificationType (juce::Justification::centred);
    progressionLabel_.setColour (juce::Label::textColourId, palette::textDim);
    progressionLabel_.setFont (juce::Font (14.0f));
    addAndMakeVisible (progressionLabel_);

    playButton_.setTooltip ("Audition the pattern with the built-in preview synth.");
    stopButton_.setTooltip ("Stop preview (sends all-notes-off).");
    loopButton_.setTooltip ("Loop the preview.");
    loopButton_.setToggleState (processor.previewIsLooping(), juce::dontSendNotification);
    playButton_.onClick = [this] { processor.previewPlay();  updateTransportButtons(); };
    stopButton_.onClick = [this] { processor.previewStop();  updateTransportButtons(); };
    loopButton_.onClick = [this] { processor.previewSetLooping (loopButton_.getToggleState()); };
    addAndMakeVisible (playButton_);
    addAndMakeVisible (stopButton_);
    addAndMakeVisible (loopButton_);

    // --- bottom: generate + seed + drag --------------------------------------
    genChordsButton_.setTooltip ("Generate a new chord progression, keeping the melody.");
    genMelodyButton_.setTooltip ("Generate a new melody over the same chords.");
    genBothButton_.setTooltip ("Generate a brand new progression AND melody.");
    genBothButton_.setColour (juce::TextButton::buttonColourId, palette::accent.withAlpha (0.85f));
    genChordsButton_.onClick = [this] { processor.generateChords(); };
    genMelodyButton_.onClick = [this] { processor.generateMelody(); };
    genBothButton_.onClick   = [this] { processor.generateBoth(); };
    addAndMakeVisible (genChordsButton_);
    addAndMakeVisible (genMelodyButton_);
    addAndMakeVisible (genBothButton_);

    seedCaption_.setText ("Seed", juce::dontSendNotification);
    seedCaption_.setColour (juce::Label::textColourId, palette::textDim);
    seedCaption_.setFont (juce::Font (12.0f));
    addAndMakeVisible (seedCaption_);

    seedField_.setTooltip ("The seed that produced this music. Paste one and press Enter to reproduce a vibe.");
    seedField_.setFont (juce::Font (juce::Font::getDefaultMonospacedFontName(), 13.0f, 0));
    seedField_.onReturnKey = [this] { processor.setSeedString (seedField_.getText()); };
    addAndMakeVisible (seedField_);

    randomSeedButton_.setTooltip ("Roll a new random seed and generate (ignores Lock).");
    randomSeedButton_.onClick = [this] { processor.rollNewSeed(); };
    copySeedButton_.setTooltip ("Copy the seed to the clipboard.");
    copySeedButton_.onClick = [this] { juce::SystemClipboard::copyTextToClipboard (processor.getSeedString()); };
    pasteSeedButton_.setTooltip ("Paste a seed from the clipboard and reproduce it.");
    pasteSeedButton_.onClick = [this] { processor.setSeedString (juce::SystemClipboard::getTextFromClipboard()); };
    addAndMakeVisible (randomSeedButton_);
    addAndMakeVisible (copySeedButton_);
    addAndMakeVisible (pasteSeedButton_);

    lockSeedToggle_ = std::make_unique<LabeledToggle> (s, ParamIDs::seedLock, "Lock",
                        "Lock the seed: regenerating reproduces the exact same music so you can tweak one setting.");
    addAndMakeVisible (*lockSeedToggle_);

    dragChords_   = std::make_unique<MidiDragTile> ("Chords", palette::chordNote,
                        [this] { return processor.writeTempFileForDrag (ExportMode::ChordsOnly); });
    dragMelody_   = std::make_unique<MidiDragTile> ("Melody", palette::melodyNote,
                        [this] { return processor.writeTempFileForDrag (ExportMode::MelodyOnly); });
    dragCombined_ = std::make_unique<MidiDragTile> ("Combined", palette::accent,
                        [this] { return processor.writeTempFileForDrag (ExportMode::Combined); });
    addAndMakeVisible (*dragChords_);
    addAndMakeVisible (*dragMelody_);
    addAndMakeVisible (*dragCombined_);

    exportButton_.setTooltip ("Save the combined MIDI to a file.");
    exportButton_.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> (
            "Export MIDI", juce::File::getSpecialLocation (juce::File::userMusicDirectory)
                .getChildFile (processor.suggestedFileName (ExportMode::Combined)), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode
                            | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& fc)
            {
                auto f = fc.getResult();
                if (f != juce::File()) processor.exportToFile (ExportMode::Combined, f);
            });
    };
    addAndMakeVisible (exportButton_);

    processor.addChangeListener (this);
    refreshFromProcessor();
    updateTransportButtons();

    setResizable (true, true);
    setResizeLimits (900, 620, 1800, 1300);
    getConstrainer()->setFixedAspectRatio (0.0);
    setSize (1040, 720);

    startTimerHz (12);   // keep transport button states in sync with playback
}

void AmbientComposerAudioProcessorEditor::timerCallback()
{
    // The preview's playing flag can flip on the audio thread (a one-shot
    // ending). Reflect that in the transport buttons without spamming repaints.
    const bool playing = processor.previewIsPlaying();
    if (playing != lastPlayingState_)
    {
        lastPlayingState_ = playing;
        updateTransportButtons();
    }
}

AmbientComposerAudioProcessorEditor::~AmbientComposerAudioProcessorEditor()
{
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void AmbientComposerAudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshFromProcessor();
}

void AmbientComposerAudioProcessorEditor::refreshFromProcessor()
{
    pianoRoll_.setPattern (processor.getCurrentPattern());

    if (auto p = processor.getCurrentPattern())
        progressionLabel_.setText (p->progressionText(), juce::dontSendNotification);
    else
        progressionLabel_.setText ("Press Generate to compose", juce::dontSendNotification);

    if (! seedField_.hasKeyboardFocus (true))
        seedField_.setText (processor.getSeedString(), juce::dontSendNotification);

    undoButton_.setEnabled (processor.canUndo());
    redoButton_.setEnabled (processor.canRedo());

    // rebuild favourites menu
    auto names = processor.getFavoriteNames();
    favMenu_.clear (juce::dontSendNotification);
    for (int i = 0; i < names.size(); ++i)
        favMenu_.addItem (names[i], i + 1);

    const bool has = processor.hasPattern();
    for (auto* b : { &playButton_, &exportButton_ }) b->setEnabled (has);
    dragChords_->setEnabled (has);
    dragMelody_->setEnabled (has);
    dragCombined_->setEnabled (has);
}

void AmbientComposerAudioProcessorEditor::updateTransportButtons()
{
    playButton_.setEnabled (processor.hasPattern() && ! processor.previewIsPlaying());
    stopButton_.setEnabled (processor.previewIsPlaying());
}

// -----------------------------------------------------------------------------
void AmbientComposerAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (palette::windowBg);

    auto panel = [&g] (juce::Rectangle<int> r)
    {
        g.setColour (palette::panelBg);
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        g.setColour (palette::panelStroke);
        g.drawRoundedRectangle (r.toFloat(), 10.0f, 1.0f);
    };
    panel (topArea_);
    panel (leftPanelArea_);
    panel (rightPanelArea_);
    panel (bottomArea_);
}

// -----------------------------------------------------------------------------
void AmbientComposerAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);

    // --- top bar -------------------------------------------------------------
    topArea_ = area.removeFromTop (48);
    {
        auto r = topArea_.reduced (12, 8);
        titleLabel_.setBounds (r.removeFromLeft (260));
        auto right = r;
        favMenu_.setBounds (right.removeFromRight (140).reduced (0, 2));
        right.removeFromRight (6);
        favButton_.setBounds (right.removeFromRight (80).reduced (0, 2));
        right.removeFromRight (12);
        redoButton_.setBounds (right.removeFromRight (66).reduced (0, 2));
        right.removeFromRight (4);
        undoButton_.setBounds (right.removeFromRight (66).reduced (0, 2));
    }
    area.removeFromTop (10);

    // --- bottom bar ----------------------------------------------------------
    bottomArea_ = area.removeFromBottom (128);
    area.removeFromBottom (10);

    // --- left / right panels + center ---------------------------------------
    leftPanelArea_  = area.removeFromLeft (232);
    area.removeFromLeft (10);
    rightPanelArea_ = area.removeFromRight (300);
    area.removeFromRight (10);
    centerArea_ = area;

    // ---- left panel contents ----
    {
        auto r = leftPanelArea_.reduced (14, 12);
        leftHeader_.setBounds (r.removeFromTop (20));
        r.removeFromTop (8);
        auto combo = [&r] (juce::Component* c) { c->setBounds (r.removeFromTop (44)); r.removeFromTop (8); };
        combo (keyCombo_.get());
        combo (scaleCombo_.get());
        combo (moodCombo_.get());
        combo (barsCombo_.get());
        combo (timeSigCombo_.get());
        r.removeFromTop (6);
        auto knobs = r.removeFromTop (92);
        bpmKnob_->setBounds (knobs.removeFromLeft (knobs.getWidth() / 2).reduced (2));
        chordOctaveKnob_->setBounds (knobs.reduced (2));
    }

    // ---- center contents ----
    {
        auto r = centerArea_;
        auto transport = r.removeFromBottom (44);
        auto prog = r.removeFromBottom (26);
        r.removeFromBottom (6);
        pianoRoll_.setBounds (r);
        progressionLabel_.setBounds (prog);

        auto t = transport;
        const int bw = 92;
        auto centreRow = t.withSizeKeepingCentre (bw * 3 + 20, 34);
        playButton_.setBounds (centreRow.removeFromLeft (bw));
        centreRow.removeFromLeft (10);
        stopButton_.setBounds (centreRow.removeFromLeft (bw));
        centreRow.removeFromLeft (10);
        loopButton_.setBounds (centreRow.removeFromLeft (bw));
    }

    // ---- right panel contents ----
    {
        auto r = rightPanelArea_.reduced (14, 12);
        rightHeader_.setBounds (r.removeFromTop (20));
        r.removeFromTop (8);
        auto combo = [&r] (juce::Component* c) { c->setBounds (r.removeFromTop (44)); r.removeFromTop (8); };
        combo (melodyStyleCombo_.get());
        combo (rhythmStyleCombo_.get());
        combo (voicingCombo_.get());
        r.removeFromTop (2);

        auto row1 = r.removeFromTop (86);
        const int kw = row1.getWidth() / 3;
        complexityKnob_->setBounds (row1.removeFromLeft (kw).reduced (2));
        sustainKnob_->setBounds    (row1.removeFromLeft (kw).reduced (2));
        melodyOctaveKnob_->setBounds (row1.reduced (2));
        r.removeFromTop (6);

        humanizeHeader_.setBounds (r.removeFromTop (18));
        r.removeFromTop (4);
        auto row2 = r.removeFromTop (86);
        const int kw2 = row2.getWidth() / 4;
        velHumKnob_->setBounds  (row2.removeFromLeft (kw2).reduced (2));
        timeHumKnob_->setBounds (row2.removeFromLeft (kw2).reduced (2));
        gateHumKnob_->setBounds (row2.removeFromLeft (kw2).reduced (2));
        midiChannelKnob_->setBounds (row2.reduced (2));
        r.removeFromTop (6);

        auto toggles = r.removeFromTop (26);
        octaveDoublingToggle_->setBounds (toggles.removeFromLeft (toggles.getWidth() / 2));
        autoPlayToggle_->setBounds (toggles);
    }

    // ---- bottom bar contents ----
    {
        auto r = bottomArea_.reduced (14, 12);

        // row 1: generate buttons
        auto genRow = r.removeFromTop (38);
        const int gw = (genRow.getWidth() - 20) / 3;
        genChordsButton_.setBounds (genRow.removeFromLeft (gw));
        genRow.removeFromLeft (10);
        genMelodyButton_.setBounds (genRow.removeFromLeft (gw));
        genRow.removeFromLeft (10);
        genBothButton_.setBounds (genRow);
        r.removeFromTop (10);

        // row 2: seed controls (left) + drag tiles + export (right)
        auto row2 = r;
        auto seedArea = row2.removeFromLeft (row2.getWidth() * 0.52f);
        seedCaption_.setBounds (seedArea.removeFromLeft (36));
        lockSeedToggle_->setBounds (seedArea.removeFromRight (66));
        pasteSeedButton_.setBounds (seedArea.removeFromRight (58).reduced (2, 4));
        copySeedButton_.setBounds  (seedArea.removeFromRight (54).reduced (2, 4));
        randomSeedButton_.setBounds (seedArea.removeFromRight (40).reduced (2, 4));
        seedField_.setBounds (seedArea.reduced (2, 4));

        row2.removeFromLeft (12);
        auto dragArea = row2;
        exportButton_.setBounds (dragArea.removeFromRight (78).reduced (2, 4));
        dragArea.removeFromRight (8);
        const int tw = (dragArea.getWidth() - 12) / 3;
        dragChords_->setBounds   (dragArea.removeFromLeft (tw).reduced (2));
        dragArea.removeFromLeft (6);
        dragMelody_->setBounds   (dragArea.removeFromLeft (tw).reduced (2));
        dragArea.removeFromLeft (6);
        dragCombined_->setBounds (dragArea.reduced (2));
    }
}
