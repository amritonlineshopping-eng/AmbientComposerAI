// =============================================================================
//  PluginProcessor.h
//  The hub: APVTS parameters, the generation pipeline, preview playback,
//  generation history / favorites, and full session state save/restore
//  (parameters + last pattern + seed).
//
//  Threading: all generation happens on the message thread into a new
//  GeneratedPattern, then handed to the preview player (lock-free sequence
//  swap) and the GUI (ChangeBroadcaster). The audio thread only reads the
//  preview player's current PlaybackSequence — no locks, no allocation.
// =============================================================================
#pragma once

#include <memory>
#include <random>
#include <vector>

#include <juce_audio_utils/juce_audio_utils.h>

#include "Parameters.h"
#include "Engines/PatternGenerator.h"
#include "IO/MidiExporter.h"
#include "Playback/PreviewPlayer.h"

class AmbientComposerAudioProcessor : public juce::AudioProcessor,
                                      public juce::ChangeBroadcaster
{
public:
    AmbientComposerAudioProcessor();
    ~AmbientComposerAudioProcessor() override = default;

    // --- audio lifecycle -----------------------------------------------------
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // --- editor --------------------------------------------------------------
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // --- plugin metadata -----------------------------------------------------
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override            { return false; }
    bool producesMidi() const override           { return true;  }
    bool isMidiEffect() const override           { return false; }
    double getTailLengthSeconds() const override { return 0.0;  }

    int getNumPrograms() override                             { return 1;  }
    int getCurrentProgram() override                          { return 0;  }
    void setCurrentProgram (int) override                     {}
    const juce::String getProgramName (int) override          { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // --- state ---------------------------------------------------------------
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // =========================================================================
    //  Public API used by the editor
    // =========================================================================
    juce::AudioProcessorValueTreeState apvts;

    // generation (message thread)
    void generateBoth();
    void generateChords();
    void generateMelody();
    void rollNewSeed();   // force a brand-new seed even while Lock is engaged

    // history
    bool canUndo() const { return historyIndex_ > 0; }
    bool canRedo() const { return historyIndex_ + 1 < (int) history_.size(); }
    void undo();
    void redo();

    // current music (message thread; GUI reads this)
    std::shared_ptr<const acai::GeneratedPattern> getCurrentPattern() const;

    // preview transport
    void previewPlay();
    void previewStop();
    void previewSetLooping (bool shouldLoop);
    bool previewIsPlaying() const  { return previewPlayer_.isPlaying(); }
    bool previewIsLooping() const  { return previewPlayer_.isLooping(); }
    double previewPlayheadTicks() const { return previewPlayer_.playheadTicks(); }

    // seed
    juce::String getSeedString() const;
    void setSeedString (const juce::String& text);   // paste + reproduce
    bool isSeedLocked() const;

    // favorites
    void addFavorite (const juce::String& name);
    juce::StringArray getFavoriteNames() const;
    void recallFavorite (int index);
    void removeFavorite (int index);

    // export
    bool exportToFile (acai::ExportMode, const juce::File&);
    juce::File writeTempFileForDrag (acai::ExportMode);
    juce::String suggestedFileName (acai::ExportMode) const;
    bool hasPattern() const;

private:
    acai::GenerationSettings readSettings() const;
    void doGenerate (acai::GenerateTarget);
    void publish (std::shared_ptr<const acai::GeneratedPattern>);   // no history push
    void pushHistory (std::shared_ptr<const acai::GeneratedPattern>);
    uint64_t nextRandomSeed();

    // --- preview -------------------------------------------------------------
    acai::PreviewPlayer previewPlayer_;

    // --- current pattern (message thread) ------------------------------------
    mutable juce::CriticalSection patternLock_;
    std::shared_ptr<const acai::GeneratedPattern> currentPattern_;

    // --- seed ----------------------------------------------------------------
    acai::Seeds currentSeeds_;
    std::mt19937_64 seedRng_;

    // --- history + favorites -------------------------------------------------
    std::vector<std::shared_ptr<const acai::GeneratedPattern>> history_;
    int historyIndex_ = -1;
    static constexpr int kMaxHistory = 64;

    std::vector<juce::ValueTree> favorites_;

    double sampleRate_ = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AmbientComposerAudioProcessor)
};
