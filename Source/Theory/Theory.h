// =============================================================================
//  Theory.h
//  Music-theory foundation: scales, note naming, chord construction and the
//  roman-numeral resolver. Pure C++ (no JUCE) — headlessly testable.
//
//  Conventions used throughout:
//   * "pc"  = pitch class 0..11 (C = 0)
//   * MIDI note numbers use the C4 = 60 convention (so C3 = 48)
//   * Scale degrees are 0-based (degree 0 = tonic)
// =============================================================================
#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace acai::theory
{
    // -------------------------------------------------------------------------
    // Scales
    // -------------------------------------------------------------------------
    // Note: "Minor" IS natural minor IS Aeolian. All three names are exposed in
    // the UI (users expect them) but they share one interval table — the enum
    // keeps distinct entries only so parameter indices match the UI list.
    enum class ScaleType : int
    {
        Major = 0, Minor, HarmonicMinor, MelodicMinor,
        Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian,
        PentatonicMajor, PentatonicMinor,
        Count
    };

    /// Semitone offsets from the tonic (7 entries; 5 for pentatonics).
    const std::vector<int>& scaleIntervals (ScaleType);

    /// Human-readable name ("Harmonic Minor" etc.).
    std::string scaleName (ScaleType);

    /// True for scales whose tonic triad is minor (drives display + defaults).
    bool isMinorFlavoured (ScaleType);

    /// Pentatonics can't stack diatonic triads sensibly; chords are built from
    /// the parent 7-note scale instead (Pent. Major -> Major, Pent. Minor -> Aeolian).
    ScaleType chordScaleFor (ScaleType);

    // -------------------------------------------------------------------------
    // Note naming
    // -------------------------------------------------------------------------
    std::string pitchClassName (int pc, bool preferFlats);
    std::string midiNoteName (int midiNote, bool preferFlats);   // e.g. "C#4"

    /// Keys that are conventionally spelt with flats (F, Bb, Eb, Ab, Db, Gb).
    bool keyPrefersFlats (int keyRootPc, bool minorFlavoured);

    // -------------------------------------------------------------------------
    // Chords
    // -------------------------------------------------------------------------
    // Ambient palette only: triads coloured with maj7/min7/9/add9/sus/6 — the
    // ChordEngine deliberately never requests dense jazz stacks.
    enum class ChordExtension : int
    {
        None = 0, Seventh,      // maj7 on major triads, min7 on minor (diatonic 7th)
        Ninth,                  // diatonic 7th + 9th
        Add9,                   // triad + 9th (no 7th)
        Sus2, Sus4,             // 3rd replaced
        Sixth,                  // triad + 6th
        SixNine                 // triad + 6th + 9th (no 7th)
    };

    struct ChordSpec
    {
        int  degree      = 0;      ///< 0-based scale degree of the root
        int  rootPc      = 0;      ///< absolute pitch class of the root
        bool flattened   = false;  ///< borrowed flat-degree root (bVII, bVI...)
        bool minorQuality = false; ///< minor third above the root?
        ChordExtension extension = ChordExtension::None;

        /// Complete chord shape as semitone offsets from the root, ascending,
        /// starting with 0. E.g. Cmaj9 = {0, 4, 7, 11, 14}.
        std::vector<int> semitonesFromRoot { 0, 4, 7 };

        std::string symbol;        ///< display symbol, e.g. "Am9", "Fmaj7"
        std::string roman;         ///< roman numeral it came from, e.g. "vi9"

        /// Pitch classes of every chord tone (root + offsets, mod 12).
        std::vector<int> pitchClasses() const;
    };

    /// Diatonic triad on `degree` of the given key/scale (stacks scale 3rds).
    ChordSpec diatonicTriad (int keyRootPc, ScaleType scale, int degree);

    /// Colour a chord by stacking scale tones (chromatic only where the idiom
    /// demands, e.g. maj7 colour on a borrowed chord). Rebuilds the symbol.
    void applyExtension (ChordSpec&, int keyRootPc, ScaleType scale, ChordExtension);

    // -------------------------------------------------------------------------
    // Roman numerals
    // -------------------------------------------------------------------------
    // Grammar: optional 'b'/'♭' prefix (borrowed flat degree), numeral I..VII
    // (upper = major quality, lower = minor), optional quality suffix that maps
    // onto ChordExtension: "maj7" "m7" "7" "9" "add9" "sus2" "sus4" "6" "6/9".
    // Diminished degrees (vii° in major, ii° in minor) come out of the diatonic
    // stack automatically.
    struct RomanChord
    {
        int  degree     = 0;      ///< 0..6
        bool flat       = false;
        bool lowerCase  = false;  ///< minor quality requested
        ChordExtension extension = ChordExtension::None;
    };

    std::optional<RomanChord> parseRoman (std::string_view);

    /// Full resolver: numeral + key + scale -> concrete ChordSpec.
    /// Borrowed chords (bVII, bVI, iv-in-major, Picardy I-in-minor) resolve to
    /// their idiomatic qualities even where the diatonic stack would differ.
    ChordSpec resolveRoman (std::string_view roman, int keyRootPc, ScaleType scale);

    // -------------------------------------------------------------------------
    // Utilities
    // -------------------------------------------------------------------------
    int clampMidi (int note);                       ///< clamp to 0..127
    int transpose (int midiNote, int semitones);    ///< transpose + clamp

    /// Absolute pitch classes of the scale.
    std::vector<int> scalePitchClasses (int keyRootPc, ScaleType);

    bool isInScale (int midiNote, int keyRootPc, ScaleType);

    /// Nearest note (in semitones) to `midiNote` that lies in the scale.
    /// Ties break downward (gravity suits ambient lines).
    int nearestScaleTone (int midiNote, int keyRootPc, ScaleType);

    /// Nearest note to `midiNote` whose pitch class is in `chordPcs`.
    int nearestChordTone (int midiNote, const std::vector<int>& chordPcs);

    /// Absolute interval size in semitones.
    int intervalSemitones (int a, int b);
}
