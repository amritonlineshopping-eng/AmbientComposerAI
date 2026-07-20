// =============================================================================
//  MidiExporter.h
//  Builds Type-1 MIDI files from a GeneratedPattern: tempo + time-signature
//  meta events, named "Chords" and "Melody" tracks, 960 PPQ, every note-on
//  paired with a note-off. Also owns the temp files used for drag-and-drop
//  into FL Studio and the "Export to file..." fallback.
// =============================================================================
#pragma once

// Only core + audio_basics so the headless test target can link this too.
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include "../Model/GeneratedPattern.h"

namespace acai
{
    enum class ExportMode : int { ChordsOnly, MelodyOnly, Combined };

    class MidiExporter
    {
    public:
        /// Build the complete in-memory MIDI file.
        /// midiChannel is 1-based (the user-facing parameter value).
        static juce::MidiFile buildMidiFile (const GeneratedPattern&,
                                             ExportMode, int midiChannel);

        /// "AmbientComposer_Amin_Dreamy_seed1A2B3C4D_chords.mid" style name —
        /// key, scale-aware spelling, mood and seed are all recoverable from it.
        static juce::String suggestedFileName (const GeneratedPattern&, ExportMode);

        /// Write into `file` (overwrites). Returns false on IO failure.
        static bool writeToFile (const GeneratedPattern&, ExportMode,
                                 int midiChannel, const juce::File& file);

        /// Write a fresh temp .mid for an external drag. Files land in a
        /// stable per-plugin temp folder that is cleaned of stale siblings on
        /// each call — the current file stays alive for the whole drag.
        /// Returns an invalid File on failure.
        static juce::File writeTempFileForDrag (const GeneratedPattern&,
                                                ExportMode, int midiChannel);
    };
}
