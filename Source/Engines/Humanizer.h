// =============================================================================
//  Humanizer.h
//  Final pass over generated notes: velocity jitter, timing nudge, gate
//  variation. Never creates negative start times, zero/negative lengths,
//  or velocities outside 1..127.
// =============================================================================
#pragma once

#include <vector>

#include "../Model/Note.h"
#include "RandomEngine.h"

namespace acai
{
    struct HumanizeSettings
    {
        double velocityAmount = 0.3;   ///< 0..1 (param % / 100)
        double timingAmount   = 0.2;   ///< 0..1
        double gateAmount     = 0.2;   ///< 0..1
    };

    class Humanizer
    {
    public:
        /// Applies all three variations in place. `loopLengthTicks` bounds the
        /// timing nudge so nothing starts before tick 0 or beyond the loop.
        /// Chord notes get gentler treatment than melody notes (pads should
        /// drift less), controlled by the caller via separate invocations.
        static void apply (std::vector<Note>& notes,
                           const HumanizeSettings&,
                           RandomEngine& rng,
                           int loopLengthTicks);
    };
}
