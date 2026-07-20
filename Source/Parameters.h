// =============================================================================
//  Parameters.h
//  Single source of truth for every APVTS parameter: IDs, choice lists,
//  ranges and defaults. Every engine and every GUI control reads from here so
//  parameter IDs can never drift apart between the processor and the editor.
// =============================================================================
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    // --- musical context -----------------------------------------------------
    inline constexpr auto key            = "key";            // 12 keys + Random
    inline constexpr auto scale          = "scale";          // Auto + 12 scales
    inline constexpr auto mood           = "mood";           // 15 moods
    inline constexpr auto bpm            = "bpm";            // 40..140
    inline constexpr auto barsPerChord   = "barsPerChord";   // 1 or 2
    inline constexpr auto timeSig        = "timeSig";        // 4/4, 3/4, 6/8

    // --- chord engine --------------------------------------------------------
    inline constexpr auto chordOctave    = "chordOctave";    // register of the pad
    inline constexpr auto voicingWidth   = "voicingWidth";   // Close / Open / Spread
    inline constexpr auto octaveDoubling = "octaveDoubling"; // root doubled -12
    inline constexpr auto sustainOverlap = "sustainOverlap"; // legato ring-over %

    // --- melody engine -------------------------------------------------------
    inline constexpr auto melodyStyle    = "melodyStyle";
    inline constexpr auto rhythmStyle    = "rhythmStyle";
    inline constexpr auto complexity     = "complexity";     // 0..100 %
    inline constexpr auto melodyOctave   = "melodyOctave";

    // --- humanizer -----------------------------------------------------------
    inline constexpr auto velHumanize    = "velHumanize";    // velocity jitter %
    inline constexpr auto timeHumanize   = "timeHumanize";   // timing jitter %
    inline constexpr auto gateHumanize   = "gateHumanize";   // length jitter %

    // --- output / workflow ---------------------------------------------------
    inline constexpr auto midiChannel    = "midiChannel";    // 1..16
    inline constexpr auto autoPlay       = "autoPlay";       // preview on generate
    inline constexpr auto seedLock       = "seedLock";       // reuse seed on generate
}

namespace ParamChoices
{
    inline const juce::StringArray keys {
        "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb",
        "G", "G#/Ab", "A", "A#/Bb", "B", "Random"
    };
    inline constexpr int keyRandomIndex = 12;

    inline const juce::StringArray scales {
        "Auto", "Major", "Minor", "Harmonic Minor", "Melodic Minor",
        "Dorian", "Phrygian", "Lydian", "Mixolydian", "Aeolian",
        "Locrian", "Pentatonic Major", "Pentatonic Minor"
    };
    inline constexpr int scaleAutoIndex = 0;

    inline const juce::StringArray moods {
        "Sad", "Lonely", "Emotional", "Happy", "Hopeful",
        "Bright", "Dark", "Cold", "Dreamy", "Floating",
        "Ethereal", "Nostalgic", "Cinematic", "Peaceful", "Warm"
    };

    inline const juce::StringArray melodyStyles {
        "Motif", "Flowing", "Minimal", "Arpeggiated", "Call & Response"
    };

    inline const juce::StringArray rhythmStyles {
        "Whole Notes", "Half Notes", "Quarter Notes", "Eighth Notes",
        "Sixteenth Notes", "Mixed", "Free Ambient"
    };

    inline const juce::StringArray voicingWidths { "Close", "Open", "Spread" };

    inline const juce::StringArray barsPerChordChoices { "1 Bar", "2 Bars" };

    inline const juce::StringArray timeSigs { "4/4", "3/4", "6/8" };
}

// -----------------------------------------------------------------------------
// The full parameter layout. Defaults are tuned for the ambient aesthetic:
// slow tempo, open voicings, long overlapping pads, gentle humanization.
// -----------------------------------------------------------------------------
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    auto choice = [&] (const char* id, const char* name,
                       const StringArray& items, int def)
    {
        params.push_back (std::make_unique<AudioParameterChoice> (
            ParameterID { id, 1 }, name, items, def));
    };
    auto floatP = [&] (const char* id, const char* name,
                       float lo, float hi, float step, float def,
                       const String& suffix = {})
    {
        params.push_back (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name,
            NormalisableRange<float> (lo, hi, step), def,
            AudioParameterFloatAttributes().withLabel (suffix)));
    };
    auto intP = [&] (const char* id, const char* name, int lo, int hi, int def)
    {
        params.push_back (std::make_unique<AudioParameterInt> (
            ParameterID { id, 1 }, name, lo, hi, def));
    };
    auto boolP = [&] (const char* id, const char* name, bool def)
    {
        params.push_back (std::make_unique<AudioParameterBool> (
            ParameterID { id, 1 }, name, def));
    };

    // musical context
    choice (ParamIDs::key,          "Key",            ParamChoices::keys, 0);
    choice (ParamIDs::scale,        "Scale",          ParamChoices::scales, ParamChoices::scaleAutoIndex);
    choice (ParamIDs::mood,         "Mood",           ParamChoices::moods, 8);       // Dreamy
    floatP (ParamIDs::bpm,          "BPM",            40.0f, 140.0f, 0.1f, 72.0f);
    choice (ParamIDs::barsPerChord, "Bars Per Chord", ParamChoices::barsPerChordChoices, 1); // 2 bars
    choice (ParamIDs::timeSig,      "Time Signature", ParamChoices::timeSigs, 0);    // 4/4

    // chord engine
    intP   (ParamIDs::chordOctave,    "Chord Octave",    1, 5, 3);                   // root ~C3
    choice (ParamIDs::voicingWidth,   "Voicing Width",   ParamChoices::voicingWidths, 1); // Open
    boolP  (ParamIDs::octaveDoubling, "Octave Doubling", true);
    floatP (ParamIDs::sustainOverlap, "Sustain/Overlap", 0.0f, 100.0f, 1.0f, 40.0f, "%");

    // melody engine
    choice (ParamIDs::melodyStyle,  "Melody Style",  ParamChoices::melodyStyles, 0); // Motif
    choice (ParamIDs::rhythmStyle,  "Rhythm Style",  ParamChoices::rhythmStyles, 6); // Free Ambient
    floatP (ParamIDs::complexity,   "Complexity",    0.0f, 100.0f, 1.0f, 35.0f, "%");
    intP   (ParamIDs::melodyOctave, "Melody Octave", 3, 6, 5);

    // humanizer
    floatP (ParamIDs::velHumanize,  "Velocity Variation", 0.0f, 100.0f, 1.0f, 30.0f, "%");
    floatP (ParamIDs::timeHumanize, "Timing Variation",   0.0f, 100.0f, 1.0f, 20.0f, "%");
    floatP (ParamIDs::gateHumanize, "Gate Variation",     0.0f, 100.0f, 1.0f, 20.0f, "%");

    // output / workflow
    intP  (ParamIDs::midiChannel, "MIDI Channel", 1, 16, 1);
    boolP (ParamIDs::autoPlay,    "Auto-Play On Generate", true);
    boolP (ParamIDs::seedLock,    "Lock Seed", false);

    return { params.begin(), params.end() };
}
