// =============================================================================
//  PatternGenerator.h
//  The full generation pipeline as ONE pure, deterministic function:
//
//    settings + Seeds  ->  MoodProfile -> key/scale -> ChordEngine
//                      ->  MelodyEngine -> Humanizer -> CompositionValidator
//                      ->  GeneratedPattern
//
//  Both the plugin processor and the headless test-suite call this — there is
//  no other entry point to generation, so what the tests verify is exactly
//  what the plugin ships. Runs on the message thread (never the audio thread);
//  completes in well under 100 ms.
// =============================================================================
#pragma once

#include "../Model/GeneratedPattern.h"
#include "RandomEngine.h"

namespace acai
{
    /// Snapshot of every parameter that influences generation. Plain ints/
    /// doubles mirroring the APVTS values (see Parameters.h for choice orders).
    struct GenerationSettings
    {
        int    keyChoice    = 0;     ///< 0..11 key, 12 = Random
        int    scaleChoice  = 0;     ///< 0 = Auto, 1.. = ScaleType index + 1
        int    moodIndex    = 8;     ///< Dreamy
        double bpm          = 72.0;
        int    barsPerChord = 2;     ///< 1 or 2 (already decoded from choice)
        int    timeSigIndex = 0;     ///< 0 = 4/4, 1 = 3/4, 2 = 6/8

        int    chordOctave    = 3;
        int    voicingWidth   = 1;   ///< 0 Close, 1 Open, 2 Spread
        bool   octaveDoubling = true;
        double sustainOverlap = 0.4; ///< 0..1

        int    melodyStyle  = 0;
        int    rhythmStyle  = 6;
        double complexity   = 0.35;  ///< 0..1
        int    melodyOctave = 5;

        double velHumanize  = 0.3;   ///< 0..1
        double timeHumanize = 0.2;
        double gateHumanize = 0.2;
    };

    enum class GenerateTarget : int
    {
        Both = 0,     ///< new chords + new melody
        ChordsOnly,   ///< new chords; melody re-fitted from its unchanged seed
        MelodyOnly    ///< chords kept verbatim from `previous`; new melody
    };

    /// Decode a timeSigIndex into the TimeSignature model.
    TimeSignature timeSignatureForIndex (int timeSigIndex);

    /// Run the pipeline.
    ///  * `seeds` fully determines the result for given settings (same seeds +
    ///    same settings -> byte-identical pattern).
    ///  * For MelodyOnly, `previous` must hold the pattern whose chords (and
    ///    musical context) are kept; its chord notes are reused untouched.
    ///  * If the CompositionValidator rejects a melody as unfixable, the
    ///    melody re-rolls internally on a derived sub-seed (bounded attempts)
    ///    — still fully deterministic for the same inputs.
    GeneratedPattern generatePattern (const GenerationSettings& settings,
                                      Seeds seeds,
                                      GenerateTarget target,
                                      const GeneratedPattern* previous);
}
