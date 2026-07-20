// =============================================================================
//  MoodEngine.h
//  Data-driven mood system. One MoodProfile per mood; every other engine
//  reads its behaviour from the active profile — no scattered per-mood ifs.
// =============================================================================
#pragma once

#include <utility>
#include <vector>

#include "../Theory/Theory.h"
#include "RandomEngine.h"

namespace acai
{
    /// Order MUST match ParamChoices::moods in Parameters.h.
    enum class Mood : int
    {
        Sad = 0, Lonely, Emotional, Happy, Hopeful,
        Bright, Dark, Cold, Dreamy, Floating,
        Ethereal, Nostalgic, Cinematic, Peaceful, Warm,
        Count
    };

    enum class VelocityCurve : int
    {
        Flat,      ///< even dynamics
        Swell,     ///< crescendo–decrescendo across each phrase
        Fade       ///< gentle decrescendo toward phrase ends
    };

    enum class Contour : int
    {
        Ascending, Descending, Arch, Wave, Floating
    };

    struct MoodProfile
    {
        /// Scale preferences for Auto Scale mode: (scale, weight) pairs.
        std::vector<std::pair<theory::ScaleType, double>> weightedScales;

        /// 0 = strongly minor harmony, 1 = strongly major. Steers which
        /// progression-template family the ChordEngine draws from.
        double majorBias = 0.5;

        /// How often chords get colour tones (maj7/9/add9/sus...), 0..1.
        double extensionUsage = 0.6;

        /// Suggested tempo range (informational; shown as a hint in the UI).
        double bpmLo = 60.0, bpmHi = 85.0;

        /// Melody note count driver, 0 = very sparse .. 1 = busy.
        double noteDensity = 0.4;

        int velocityBase  = 74;      ///< centre velocity
        int velocityRange = 20;      ///< spread around the centre
        VelocityCurve velocityCurve = VelocityCurve::Swell;

        /// Note length as a fraction of its rhythmic slot: 0.3 staccato-ish,
        /// 1.0 legato, >1.0 ring-over (ambient default is long).
        double gateLength = 0.95;

        Contour contour = Contour::Arch;

        /// Melody register bias in octaves, -1.0 (dark/low) .. +1.0 (airy/high).
        double registerBias = 0.0;

        /// One weight per RhythmStyle (7 entries, order of ParamChoices::rhythmStyles).
        /// Used when the mood wants to bias "Mixed"/"Free Ambient" placement.
        std::vector<double> rhythmStyleWeights { 1, 1, 1, 1, 0.2, 1, 1.5 };

        /// Probability of a rest in an available melody slot, 0..1.
        double restProbability = 0.35;

        /// Probability that a melody move is an expressive leap, 0..1.
        double leapProbability = 0.12;

        /// How often the harmony borrows outside the scale (bVII, bVI, iv...).
        double borrowedChordProb = 0.15;
    };

    namespace moods
    {
        /// The 15-entry profile table (defined in MoodEngine.cpp).
        const MoodProfile& profileFor (Mood);

        /// Weighted scale pick for Auto Scale mode; deterministic per rng state.
        theory::ScaleType chooseScale (Mood, RandomEngine&);

        const char* name (Mood);
    }
}
