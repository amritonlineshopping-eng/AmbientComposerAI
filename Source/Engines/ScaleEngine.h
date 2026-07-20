// =============================================================================
//  ScaleEngine.h
//  Resolves the user's Key / Scale parameter choices (including "Random" key
//  and "Auto" scale) into a concrete MusicalContext, deterministically per rng.
// =============================================================================
#pragma once

#include "../Theory/Theory.h"
#include "MoodEngine.h"
#include "RandomEngine.h"

namespace acai
{
    /// The resolved musical frame every engine generates inside.
    struct MusicalContext
    {
        int keyRootPc = 0;                                   ///< 0..11, C = 0
        theory::ScaleType scale = theory::ScaleType::Minor;
        bool preferFlats = false;                            ///< for note spelling
    };

    namespace scaleEngine
    {
        /// keyChoice: index into ParamChoices::keys (12 = Random).
        /// scaleChoice: index into ParamChoices::scales (0 = Auto; 1.. maps to
        /// ScaleType by index-1). Random/Auto draw from `rng` (chord sub-seed
        /// domain) so the choice is reproducible per seed.
        MusicalContext resolve (int keyChoice, int scaleChoice,
                                Mood mood, RandomEngine& rng);
    }
}
