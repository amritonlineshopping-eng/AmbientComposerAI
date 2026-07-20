// =============================================================================
//  ScaleEngine.cpp — resolve Key/Scale choices into a MusicalContext.
// =============================================================================
#include "ScaleEngine.h"

namespace acai::scaleEngine
{
    MusicalContext resolve (int keyChoice, int scaleChoice,
                            Mood mood, RandomEngine& rng)
    {
        MusicalContext ctx;

        // Key: 0..11 explicit, 12 = Random (draw from rng so it's seed-stable).
        if (keyChoice >= 0 && keyChoice <= 11)
            ctx.keyRootPc = keyChoice;
        else
            ctx.keyRootPc = rng.nextInt (0, 11);

        // Scale: 0 = Auto (mood-weighted), else ScaleType by (choice - 1).
        if (scaleChoice <= 0)
        {
            ctx.scale = moods::chooseScale (mood, rng);
        }
        else
        {
            const int idx = scaleChoice - 1;
            if (idx >= 0 && idx < (int) theory::ScaleType::Count)
                ctx.scale = (theory::ScaleType) idx;
            else
                ctx.scale = theory::ScaleType::Minor;
        }

        ctx.preferFlats = theory::keyPrefersFlats (ctx.keyRootPc,
                                                   theory::isMinorFlavoured (ctx.scale));
        return ctx;
    }
}
