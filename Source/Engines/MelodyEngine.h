// =============================================================================
//  MelodyEngine.h
//  Constraint-based melody generation over a chord progression:
//  motif -> repetition with variation -> chord-tone targeting on strong beats,
//  passing/neighbor tones on weak beats -> leap recovery -> cadential ending.
// =============================================================================
#pragma once

#include <vector>

#include "../Model/GeneratedPattern.h"
#include "MoodEngine.h"
#include "RandomEngine.h"
#include "ScaleEngine.h"

namespace acai
{
    /// Order matches ParamChoices::melodyStyles.
    enum class MelodyStyle : int
    {
        Motif = 0,        ///< short motif repeated with variation (default)
        Flowing,          ///< continuous stepwise lines, fewer rests
        Minimal,          ///< very sparse long tones, maximum space
        Arpeggiated,      ///< chord-tone broken figures
        CallAndResponse   ///< alternating statement / answer phrases
    };

    /// Order matches ParamChoices::rhythmStyles.
    enum class RhythmStyle : int
    {
        Whole = 0, Half, Quarter, Eighth, Sixteenth, Mixed, FreeAmbient
    };

    struct MelodyEngineSettings
    {
        MelodyStyle style       = MelodyStyle::Motif;
        RhythmStyle rhythmStyle = RhythmStyle::FreeAmbient;
        double      complexity  = 0.35;   ///< 0..1 (see §8-complexity mapping)
        int         melodyOctave = 5;     ///< base register: around C{melodyOctave}
        int         barsPerChord = 2;
        TimeSignature timeSig;
    };

    class MelodyEngine
    {
    public:
        /// Deterministic per rng seed (fresh engine from Seeds::melody).
        /// The returned notes are pre-humanize, quantized to the tick grid.
        static std::vector<Note> generate (const std::vector<VoicedChord>& chords,
                                           const MusicalContext&,
                                           const MoodProfile&,
                                           const MelodyEngineSettings&,
                                           RandomEngine& rng);
    };
}
