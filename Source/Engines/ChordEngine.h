// =============================================================================
//  ChordEngine.h
//  Generates exactly 4 voice-led, ambient-flavoured chords:
//  template -> substitutions/borrowed chords -> extensions -> nearest-note
//  voice leading -> width/doubling/register -> sustain/overlap realization.
// =============================================================================
#pragma once

#include <vector>

#include "../Model/GeneratedPattern.h"
#include "MoodEngine.h"
#include "RandomEngine.h"
#include "ScaleEngine.h"

namespace acai
{
    struct ChordEngineSettings
    {
        int    chordOctave    = 3;      ///< root register: root near C{chordOctave}
        int    voicingWidth   = 1;      ///< 0 Close, 1 Open, 2 Spread
        bool   octaveDoubling = true;   ///< double the root -12
        double sustainOverlap = 0.4;    ///< 0..1 — ring past the chord boundary
        int    barsPerChord   = 2;
        TimeSignature timeSig;
    };

    struct ChordResult
    {
        std::vector<VoicedChord> chords;   ///< the 4 voiced chords
        std::vector<Note>        notes;    ///< realized pad notes (pre-humanize)
        int lengthTicks = 0;               ///< loop length (4 * barsPerChord bars)
    };

    class ChordEngine
    {
    public:
        /// Deterministic per rng seed. The rng passed in must be a fresh
        /// engine built from Seeds::chord.
        static ChordResult generate (const MusicalContext&,
                                     const MoodProfile&,
                                     const ChordEngineSettings&,
                                     RandomEngine& rng);
    };
}
