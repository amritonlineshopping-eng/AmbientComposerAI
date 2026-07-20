// =============================================================================
//  CompositionValidator.h
//  Post-generation guarantee pass. Fixes what can be fixed gently (nudge to
//  consonance, cap wild leaps, break exact repetition); reports when a
//  generation is beyond repair so the caller can re-roll a sub-seed.
// =============================================================================
#pragma once

#include "../Model/GeneratedPattern.h"
#include "MoodEngine.h"
#include "RandomEngine.h"
#include "ScaleEngine.h"

namespace acai
{
    struct ValidationReport
    {
        bool acceptable      = true;   ///< false -> caller should re-roll
        int  notesNudged     = 0;      ///< out-of-scale / clashing fixes
        int  leapsCapped     = 0;
        bool repetitionFixed = false;  ///< copy-paste melody was varied
    };

    class CompositionValidator
    {
    public:
        /// Checks and repairs `pattern.melodyNotes` against the sounding
        /// chords, in place. Never touches deliberate chromatic passing tones
        /// (Note::isChromaticPassing). Deterministic per rng.
        static ValidationReport validateAndFix (GeneratedPattern& pattern,
                                                const MusicalContext&,
                                                const MoodProfile&,
                                                RandomEngine& rng);
    };
}
