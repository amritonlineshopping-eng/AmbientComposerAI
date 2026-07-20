// =============================================================================
//  Humanizer.cpp — velocity / timing / gate variation, safely bounded.
// =============================================================================
#include "Humanizer.h"

#include <algorithm>
#include <cmath>

namespace acai
{
    void Humanizer::apply (std::vector<Note>& notes,
                           const HumanizeSettings& s,
                           RandomEngine& rng,
                           int loopLengthTicks)
    {
        if (notes.empty()) return;

        // Map 0..1 amounts to musical magnitudes.
        //  * velocity: up to +/-24 at full.
        //  * timing:   up to +/-1/16-note (kPPQ/4 ticks) at full.
        //  * gate:     up to +/-35% of length at full.
        const double velStd  = s.velocityAmount * 24.0;
        const int    timeMax = (int) (s.timingAmount * (kPPQ / 4.0));
        const double gateAmt = s.gateAmount * 0.35;

        for (auto& n : notes)
        {
            // --- velocity ----------------------------------------------------
            if (velStd > 0.0)
            {
                const int jitter = (int) std::lround (rng.nextGaussian (velStd));
                n.velocity = std::clamp (n.velocity + jitter, 1, 127);
            }

            // --- timing ------------------------------------------------------
            if (timeMax > 0)
            {
                const int nudge = rng.nextInt (-timeMax, timeMax);
                int newStart = n.startTick + nudge;
                // Never before the loop start; never past the final tick.
                newStart = std::clamp (newStart, 0, std::max (0, loopLengthTicks - 1));
                // Keep the note-off inside the loop-adjacent window; length is
                // preserved here (gate step handles length).
                n.startTick = newStart;
            }

            // --- gate --------------------------------------------------------
            if (gateAmt > 0.0)
            {
                const double factor = 1.0 + rng.nextGaussian (gateAmt);
                int newLen = (int) std::lround (n.lengthTick * std::clamp (factor, 0.4, 1.8));
                // Musical minimum: a 32nd note.
                newLen = std::max (newLen, kPPQ / 8);
                n.lengthTick = newLen;
            }
        }
    }
}
