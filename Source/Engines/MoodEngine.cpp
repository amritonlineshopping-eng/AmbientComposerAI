// =============================================================================
//  MoodEngine.cpp — the 15-mood profile table.
//
//  Each profile is tuned so the mood AUDIBLY changes scale, chord colour,
//  contour, density, rhythm, velocity and note length (acceptance criterion).
//  The rhythmStyleWeights vector has 7 entries in ParamChoices::rhythmStyles
//  order: {Whole, Half, Quarter, Eighth, Sixteenth, Mixed, FreeAmbient}.
// =============================================================================
#include "MoodEngine.h"

namespace acai::moods
{
    using theory::ScaleType;

    namespace
    {
        MoodProfile makeTable[(int) Mood::Count];
        bool tableBuilt = false;

        void build()
        {
            using SC = std::pair<ScaleType, double>;

            // -- Sad -----------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Sad];
                m.weightedScales = { {ScaleType::Aeolian, 3}, {ScaleType::HarmonicMinor, 2}, {ScaleType::Dorian, 1} };
                m.majorBias = 0.08; m.extensionUsage = 0.7;
                m.bpmLo = 58; m.bpmHi = 74;
                m.noteDensity = 0.30;
                m.velocityBase = 62; m.velocityRange = 18; m.velocityCurve = VelocityCurve::Swell;
                m.gateLength = 1.05; m.contour = Contour::Descending; m.registerBias = -0.1;
                m.rhythmStyleWeights = { 2, 3, 1.5, 0.4, 0.1, 2, 2.5 };
                m.restProbability = 0.42; m.leapProbability = 0.10; m.borrowedChordProb = 0.20;
            }
            // -- Lonely --------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Lonely];
                m.weightedScales = { {ScaleType::Aeolian, 3}, {ScaleType::Dorian, 1}, {ScaleType::PentatonicMinor, 1} };
                m.majorBias = 0.06; m.extensionUsage = 0.6;
                m.bpmLo = 52; m.bpmHi = 66;
                m.noteDensity = 0.18;
                m.velocityBase = 54; m.velocityRange = 16; m.velocityCurve = VelocityCurve::Fade;
                m.gateLength = 1.15; m.contour = Contour::Descending; m.registerBias = -0.4;
                m.rhythmStyleWeights = { 3, 2.5, 0.8, 0.15, 0.05, 1.5, 3 };
                m.restProbability = 0.55; m.leapProbability = 0.08; m.borrowedChordProb = 0.18;
            }
            // -- Emotional -----------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Emotional];
                m.weightedScales = { {ScaleType::Aeolian, 2}, {ScaleType::HarmonicMinor, 2}, {ScaleType::MelodicMinor, 1} };
                m.majorBias = 0.18; m.extensionUsage = 0.8;
                m.bpmLo = 60; m.bpmHi = 80;
                m.noteDensity = 0.42;
                m.velocityBase = 70; m.velocityRange = 26; m.velocityCurve = VelocityCurve::Swell;
                m.gateLength = 1.0; m.contour = Contour::Arch; m.registerBias = 0.0;
                m.rhythmStyleWeights = { 1, 2, 2, 1, 0.3, 2.5, 2 };
                m.restProbability = 0.34; m.leapProbability = 0.16; m.borrowedChordProb = 0.22;
            }
            // -- Happy ---------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Happy];
                m.weightedScales = { {ScaleType::Major, 3}, {ScaleType::Mixolydian, 1}, {ScaleType::PentatonicMajor, 1} };
                m.majorBias = 0.94; m.extensionUsage = 0.55;
                m.bpmLo = 74; m.bpmHi = 96;
                m.noteDensity = 0.55;
                m.velocityBase = 84; m.velocityRange = 20; m.velocityCurve = VelocityCurve::Flat;
                m.gateLength = 0.85; m.contour = Contour::Ascending; m.registerBias = 0.15;
                m.rhythmStyleWeights = { 0.3, 1, 2, 2.5, 1, 2, 1 };
                m.restProbability = 0.22; m.leapProbability = 0.16; m.borrowedChordProb = 0.06;
            }
            // -- Hopeful -------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Hopeful];
                m.weightedScales = { {ScaleType::Major, 3}, {ScaleType::Lydian, 1}, {ScaleType::Mixolydian, 1} };
                m.majorBias = 0.88; m.extensionUsage = 0.62;
                m.bpmLo = 70; m.bpmHi = 90;
                m.noteDensity = 0.48;
                m.velocityBase = 78; m.velocityRange = 22; m.velocityCurve = VelocityCurve::Swell;
                m.gateLength = 0.95; m.contour = Contour::Ascending; m.registerBias = 0.2;
                m.rhythmStyleWeights = { 0.5, 1.5, 2, 2, 0.6, 2, 1.5 };
                m.restProbability = 0.26; m.leapProbability = 0.18; m.borrowedChordProb = 0.08;
            }
            // -- Bright --------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Bright];
                m.weightedScales = { {ScaleType::Lydian, 3}, {ScaleType::Major, 2}, {ScaleType::PentatonicMajor, 1} };
                m.majorBias = 0.97; m.extensionUsage = 0.6;
                m.bpmLo = 78; m.bpmHi = 100;
                m.noteDensity = 0.6;
                m.velocityBase = 86; m.velocityRange = 18; m.velocityCurve = VelocityCurve::Flat;
                m.gateLength = 0.8; m.contour = Contour::Ascending; m.registerBias = 0.45;
                m.rhythmStyleWeights = { 0.2, 0.8, 2, 3, 1.5, 2, 0.8 };
                m.restProbability = 0.18; m.leapProbability = 0.2; m.borrowedChordProb = 0.05;
            }
            // -- Dark ----------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Dark];
                m.weightedScales = { {ScaleType::Phrygian, 3}, {ScaleType::HarmonicMinor, 2}, {ScaleType::Locrian, 1} };
                m.majorBias = 0.03; m.extensionUsage = 0.45;
                m.bpmLo = 54; m.bpmHi = 70;
                m.noteDensity = 0.22;
                m.velocityBase = 58; m.velocityRange = 16; m.velocityCurve = VelocityCurve::Fade;
                m.gateLength = 1.2; m.contour = Contour::Floating;   // low, static-floating
                m.registerBias = -0.6;
                m.rhythmStyleWeights = { 3, 2.5, 0.6, 0.2, 0.05, 1.2, 2.5 };
                m.restProbability = 0.5; m.leapProbability = 0.07; m.borrowedChordProb = 0.25;
            }
            // -- Cold ----------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Cold];
                m.weightedScales = { {ScaleType::Locrian, 2}, {ScaleType::Phrygian, 2}, {ScaleType::Aeolian, 1} };
                m.majorBias = 0.05; m.extensionUsage = 0.4;
                m.bpmLo = 50; m.bpmHi = 66;
                m.noteDensity = 0.16;
                m.velocityBase = 52; m.velocityRange = 14; m.velocityCurve = VelocityCurve::Fade;
                m.gateLength = 1.25; m.contour = Contour::Floating; m.registerBias = -0.5;
                m.rhythmStyleWeights = { 3.5, 2, 0.5, 0.1, 0.02, 1, 3 };
                m.restProbability = 0.58; m.leapProbability = 0.10; m.borrowedChordProb = 0.22;
            }
            // -- Dreamy --------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Dreamy];
                m.weightedScales = { {ScaleType::Lydian, 2}, {ScaleType::Major, 2}, {ScaleType::Dorian, 1} };
                m.majorBias = 0.7; m.extensionUsage = 0.85;
                m.bpmLo = 62; m.bpmHi = 80;
                m.noteDensity = 0.34;
                m.velocityBase = 66; m.velocityRange = 14; m.velocityCurve = VelocityCurve::Flat;
                m.gateLength = 1.15; m.contour = Contour::Floating; m.registerBias = 0.2;
                m.rhythmStyleWeights = { 1.5, 2, 1.2, 0.6, 0.15, 2.5, 3 };
                m.restProbability = 0.44; m.leapProbability = 0.14; m.borrowedChordProb = 0.12;
            }
            // -- Floating ------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Floating];
                m.weightedScales = { {ScaleType::Lydian, 3}, {ScaleType::Major, 1}, {ScaleType::Mixolydian, 1} };
                m.majorBias = 0.72; m.extensionUsage = 0.9;
                m.bpmLo = 58; m.bpmHi = 74;
                m.noteDensity = 0.24;
                m.velocityBase = 60; m.velocityRange = 12; m.velocityCurve = VelocityCurve::Flat;
                m.gateLength = 1.3; m.contour = Contour::Floating; m.registerBias = 0.35;
                m.rhythmStyleWeights = { 2.5, 2, 0.8, 0.3, 0.05, 2, 3.5 };
                m.restProbability = 0.5; m.leapProbability = 0.16; m.borrowedChordProb = 0.10;
            }
            // -- Ethereal ------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Ethereal];
                m.weightedScales = { {ScaleType::Lydian, 3}, {ScaleType::Major, 1}, {ScaleType::PentatonicMajor, 1} };
                m.majorBias = 0.75; m.extensionUsage = 0.92;
                m.bpmLo = 54; m.bpmHi = 70;
                m.noteDensity = 0.16;
                m.velocityBase = 56; m.velocityRange = 10; m.velocityCurve = VelocityCurve::Flat;
                m.gateLength = 1.4; m.contour = Contour::Floating; m.registerBias = 0.6;
                m.rhythmStyleWeights = { 3, 2, 0.5, 0.15, 0.02, 1.8, 4 };
                m.restProbability = 0.6; m.leapProbability = 0.18; m.borrowedChordProb = 0.08;
            }
            // -- Nostalgic -----------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Nostalgic];
                m.weightedScales = { {ScaleType::Major, 2}, {ScaleType::Mixolydian, 2}, {ScaleType::Dorian, 1} };
                m.majorBias = 0.62; m.extensionUsage = 0.78;
                m.bpmLo = 64; m.bpmHi = 82;
                m.noteDensity = 0.4;
                m.velocityBase = 70; m.velocityRange = 20; m.velocityCurve = VelocityCurve::Swell;
                m.gateLength = 1.0; m.contour = Contour::Arch; m.registerBias = 0.0;
                m.rhythmStyleWeights = { 1, 2, 2, 1, 0.3, 2.5, 2 };
                m.restProbability = 0.32; m.leapProbability = 0.14; m.borrowedChordProb = 0.16;
            }
            // -- Cinematic -----------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Cinematic];
                m.weightedScales = { {ScaleType::Aeolian, 2}, {ScaleType::Dorian, 2}, {ScaleType::HarmonicMinor, 1} };
                m.majorBias = 0.3; m.extensionUsage = 0.7;
                m.bpmLo = 60; m.bpmHi = 84;
                m.noteDensity = 0.46;
                m.velocityBase = 72; m.velocityRange = 30; m.velocityCurve = VelocityCurve::Swell;
                m.gateLength = 1.05; m.contour = Contour::Arch; m.registerBias = 0.1;
                m.rhythmStyleWeights = { 1, 1.5, 2, 1.2, 0.4, 3, 2 };
                m.restProbability = 0.3; m.leapProbability = 0.2; m.borrowedChordProb = 0.24;
            }
            // -- Peaceful ------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Peaceful];
                m.weightedScales = { {ScaleType::Major, 2}, {ScaleType::PentatonicMajor, 3}, {ScaleType::Lydian, 1} };
                m.majorBias = 0.85; m.extensionUsage = 0.55;
                m.bpmLo = 56; m.bpmHi = 72;
                m.noteDensity = 0.2;
                m.velocityBase = 62; m.velocityRange = 12; m.velocityCurve = VelocityCurve::Flat;
                m.gateLength = 1.2; m.contour = Contour::Wave; m.registerBias = 0.1;
                m.rhythmStyleWeights = { 2.5, 2.5, 1, 0.3, 0.05, 1.8, 2.5 };
                m.restProbability = 0.46; m.leapProbability = 0.06; m.borrowedChordProb = 0.05;
            }
            // -- Warm ----------------------------------------------------------
            {
                auto& m = makeTable[(int) Mood::Warm];
                m.weightedScales = { {ScaleType::Major, 2}, {ScaleType::Mixolydian, 2}, {ScaleType::Dorian, 1} };
                m.majorBias = 0.72; m.extensionUsage = 0.8;
                m.bpmLo = 62; m.bpmHi = 80;
                m.noteDensity = 0.36;
                m.velocityBase = 72; m.velocityRange = 16; m.velocityCurve = VelocityCurve::Flat;
                m.gateLength = 1.1; m.contour = Contour::Wave; m.registerBias = -0.15;
                m.rhythmStyleWeights = { 1.5, 2, 1.5, 0.6, 0.15, 2.5, 2.5 };
                m.restProbability = 0.34; m.leapProbability = 0.10; m.borrowedChordProb = 0.10;
            }

            tableBuilt = true;
        }
    }

    const MoodProfile& profileFor (Mood m)
    {
        if (! tableBuilt) build();
        int idx = (int) m;
        if (idx < 0 || idx >= (int) Mood::Count) idx = 0;
        return makeTable[idx];
    }

    ScaleType chooseScale (Mood m, RandomEngine& rng)
    {
        const auto& prof = profileFor (m);
        if (prof.weightedScales.empty()) return ScaleType::Aeolian;
        std::vector<double> weights;
        for (const auto& sc : prof.weightedScales) weights.push_back (sc.second);
        const int pick = rng.pickWeighted (weights);
        return prof.weightedScales[(size_t) pick].first;
    }

    const char* name (Mood m)
    {
        static const char* names[] = {
            "Sad", "Lonely", "Emotional", "Happy", "Hopeful",
            "Bright", "Dark", "Cold", "Dreamy", "Floating",
            "Ethereal", "Nostalgic", "Cinematic", "Peaceful", "Warm"
        };
        int idx = (int) m;
        if (idx < 0 || idx >= (int) Mood::Count) idx = 0;
        return names[idx];
    }
}
