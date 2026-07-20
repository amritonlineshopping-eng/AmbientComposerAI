// =============================================================================
//  Theory.cpp — implementation of the music-theory foundation.
// =============================================================================
#include "Theory.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace acai::theory
{
    // -------------------------------------------------------------------------
    // Scale interval tables (semitones from tonic).
    // -------------------------------------------------------------------------
    const std::vector<int>& scaleIntervals (ScaleType s)
    {
        static const std::vector<int> major        { 0, 2, 4, 5, 7, 9, 11 };
        static const std::vector<int> naturalMinor { 0, 2, 3, 5, 7, 8, 10 };
        static const std::vector<int> harmonicMin  { 0, 2, 3, 5, 7, 8, 11 };
        static const std::vector<int> melodicMin   { 0, 2, 3, 5, 7, 9, 11 };
        static const std::vector<int> dorian       { 0, 2, 3, 5, 7, 9, 10 };
        static const std::vector<int> phrygian     { 0, 1, 3, 5, 7, 8, 10 };
        static const std::vector<int> lydian       { 0, 2, 4, 6, 7, 9, 11 };
        static const std::vector<int> mixolydian   { 0, 2, 4, 5, 7, 9, 10 };
        static const std::vector<int> locrian      { 0, 1, 3, 5, 6, 8, 10 };
        static const std::vector<int> pentMajor    { 0, 2, 4, 7, 9 };
        static const std::vector<int> pentMinor    { 0, 3, 5, 7, 10 };

        switch (s)
        {
            case ScaleType::Major:            return major;
            case ScaleType::Minor:            return naturalMinor;   // Minor == natural minor
            case ScaleType::HarmonicMinor:    return harmonicMin;
            case ScaleType::MelodicMinor:     return melodicMin;
            case ScaleType::Dorian:           return dorian;
            case ScaleType::Phrygian:         return phrygian;
            case ScaleType::Lydian:           return lydian;
            case ScaleType::Mixolydian:       return mixolydian;
            case ScaleType::Aeolian:          return naturalMinor;   // Aeolian == natural minor
            case ScaleType::Locrian:          return locrian;
            case ScaleType::PentatonicMajor:  return pentMajor;
            case ScaleType::PentatonicMinor:  return pentMinor;
            default:                          return naturalMinor;
        }
    }

    std::string scaleName (ScaleType s)
    {
        switch (s)
        {
            case ScaleType::Major:            return "Major";
            case ScaleType::Minor:            return "Minor";
            case ScaleType::HarmonicMinor:    return "Harmonic Minor";
            case ScaleType::MelodicMinor:     return "Melodic Minor";
            case ScaleType::Dorian:           return "Dorian";
            case ScaleType::Phrygian:         return "Phrygian";
            case ScaleType::Lydian:           return "Lydian";
            case ScaleType::Mixolydian:       return "Mixolydian";
            case ScaleType::Aeolian:          return "Aeolian";
            case ScaleType::Locrian:          return "Locrian";
            case ScaleType::PentatonicMajor:  return "Pentatonic Major";
            case ScaleType::PentatonicMinor:  return "Pentatonic Minor";
            default:                          return "Minor";
        }
    }

    bool isMinorFlavoured (ScaleType s)
    {
        switch (s)
        {
            case ScaleType::Major:
            case ScaleType::Lydian:
            case ScaleType::Mixolydian:
            case ScaleType::PentatonicMajor:
                return false;
            default:
                return true;   // minor 3rd above tonic
        }
    }

    ScaleType chordScaleFor (ScaleType s)
    {
        if (s == ScaleType::PentatonicMajor) return ScaleType::Major;
        if (s == ScaleType::PentatonicMinor) return ScaleType::Aeolian;
        return s;
    }

    // -------------------------------------------------------------------------
    // Note naming.
    // -------------------------------------------------------------------------
    std::string pitchClassName (int pc, bool preferFlats)
    {
        pc = ((pc % 12) + 12) % 12;
        static const char* sharps[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
        static const char* flats[]  = { "C","Db","D","Eb","E","F","Gb","G","Ab","A","Bb","B" };
        return preferFlats ? flats[pc] : sharps[pc];
    }

    std::string midiNoteName (int midiNote, bool preferFlats)
    {
        midiNote = clampMidi (midiNote);
        const int pc = midiNote % 12;
        const int octave = midiNote / 12 - 1;      // MIDI 60 = C4
        return pitchClassName (pc, preferFlats) + std::to_string (octave);
    }

    bool keyPrefersFlats (int keyRootPc, bool minorFlavoured)
    {
        keyRootPc = ((keyRootPc % 12) + 12) % 12;
        // Flat-side keys by convention. Handled separately for major/minor so
        // enharmonic tonics (e.g. D#/Eb) spell the way musicians expect.
        if (! minorFlavoured)
        {
            // Major keys spelt with flats: F, Bb, Eb, Ab, Db, Gb.
            static const bool flatMajor[12] =
            //   C     C#/Db  D     D#/Eb  E     F     F#/Gb  G     G#/Ab  A     A#/Bb  B
            {   false, true,  false, true, false, true, true, false, true, false, true, false };
            return flatMajor[keyRootPc];
        }
        // Minor keys spelt with flats: D minor, G minor, C minor, F minor,
        // Bb minor, Eb minor.
        static const bool flatMinor[12] =
        //   C     C#/Db  D     D#/Eb  E     F     F#/Gb  G     G#/Ab  A     A#/Bb  B
        {   true,  false, true, true,  false, true, false, true, true, false, true, false };
        return flatMinor[keyRootPc];
    }

    // -------------------------------------------------------------------------
    // Utilities.
    // -------------------------------------------------------------------------
    int clampMidi (int note) { return std::clamp (note, 0, 127); }

    int transpose (int midiNote, int semitones) { return clampMidi (midiNote + semitones); }

    std::vector<int> scalePitchClasses (int keyRootPc, ScaleType s)
    {
        keyRootPc = ((keyRootPc % 12) + 12) % 12;
        std::vector<int> out;
        for (int iv : scaleIntervals (s))
            out.push_back ((keyRootPc + iv) % 12);
        return out;
    }

    bool isInScale (int midiNote, int keyRootPc, ScaleType s)
    {
        const int pc = ((midiNote % 12) + 12) % 12;
        for (int p : scalePitchClasses (keyRootPc, s))
            if (p == pc) return true;
        return false;
    }

    int nearestScaleTone (int midiNote, int keyRootPc, ScaleType s)
    {
        if (isInScale (midiNote, keyRootPc, s)) return clampMidi (midiNote);
        for (int d = 1; d <= 6; ++d)
        {
            if (isInScale (midiNote - d, keyRootPc, s)) return clampMidi (midiNote - d); // gravity: down first
            if (isInScale (midiNote + d, keyRootPc, s)) return clampMidi (midiNote + d);
        }
        return clampMidi (midiNote);
    }

    int nearestChordTone (int midiNote, const std::vector<int>& chordPcs)
    {
        if (chordPcs.empty()) return clampMidi (midiNote);
        auto isTone = [&chordPcs] (int n)
        {
            const int pc = ((n % 12) + 12) % 12;
            return std::find (chordPcs.begin(), chordPcs.end(), pc) != chordPcs.end();
        };
        if (isTone (midiNote)) return clampMidi (midiNote);
        for (int d = 1; d <= 6; ++d)
        {
            if (isTone (midiNote - d)) return clampMidi (midiNote - d);
            if (isTone (midiNote + d)) return clampMidi (midiNote + d);
        }
        return clampMidi (midiNote);
    }

    int intervalSemitones (int a, int b) { return std::abs (a - b); }

    // -------------------------------------------------------------------------
    // Chords.
    // -------------------------------------------------------------------------
    std::vector<int> ChordSpec::pitchClasses() const
    {
        std::vector<int> pcs;
        for (int off : semitonesFromRoot)
            pcs.push_back (((rootPc + off) % 12 + 12) % 12);
        std::sort (pcs.begin(), pcs.end());
        pcs.erase (std::unique (pcs.begin(), pcs.end()), pcs.end());
        return pcs;
    }

    namespace
    {
        // Scale tone `steps` above `degree`, returning semitone offset above the
        // chord root and whether it required wrapping an octave. Works entirely
        // within the diatonic scale so stacked 3rds stay in key.
        int scaleToneOffsetAbove (ScaleType scale, int degree, int steps)
        {
            const auto& iv = scaleIntervals (scale);
            const int n = (int) iv.size();
            const int rootIv = iv[degree % n];
            const int targetIndex = degree + steps;
            const int octaves = targetIndex / n;
            const int wrapped = ((targetIndex % n) + n) % n;
            int offset = iv[wrapped] - rootIv + 12 * octaves;
            while (offset < 0) offset += 12;
            return offset;
        }

        std::string qualitySuffix (const ChordSpec& c)
        {
            // The 7th's quality is decided by the ACTUAL stacked interval, not
            // by the triad's third — a major triad carrying a diatonic minor 7th
            // is a dominant seventh ("7"/"9"), not a maj7/maj9.
            auto hasSemi = [&c] (int semi)
            {
                for (int s : c.semitonesFromRoot)
                    if (((s % 12) + 12) % 12 == semi) return true;
                return false;
            };
            const bool majorSeventh = hasSemi (11);   // 11 = major 7th, 10 = minor 7th

            switch (c.extension)
            {
                case ChordExtension::None:    return c.minorQuality ? "m"    : "";
                case ChordExtension::Seventh:
                    if (c.minorQuality) return majorSeventh ? "mMaj7" : "m7";
                    return majorSeventh ? "maj7" : "7";
                case ChordExtension::Ninth:
                    if (c.minorQuality) return majorSeventh ? "mMaj9" : "m9";
                    return majorSeventh ? "maj9" : "9";
                case ChordExtension::Add9:    return c.minorQuality ? "m add9" : "add9";
                case ChordExtension::Sus2:    return "sus2";
                case ChordExtension::Sus4:    return "sus4";
                case ChordExtension::Sixth:   return c.minorQuality ? "m6"   : "6";
                case ChordExtension::SixNine: return c.minorQuality ? "m6/9" : "6/9";
            }
            return {};
        }

        void rebuildSymbol (ChordSpec& c, bool preferFlats)
        {
            c.symbol = pitchClassName (c.rootPc, preferFlats) + qualitySuffix (c);
        }
    }

    ChordSpec diatonicTriad (int keyRootPc, ScaleType scale, int degree)
    {
        scale = chordScaleFor (scale);
        keyRootPc = ((keyRootPc % 12) + 12) % 12;
        const auto& iv = scaleIntervals (scale);
        const int n = (int) iv.size();
        degree = ((degree % n) + n) % n;

        ChordSpec c;
        c.degree = degree;
        c.rootPc = (keyRootPc + iv[degree]) % 12;

        const int third = scaleToneOffsetAbove (scale, degree, 2);
        const int fifth = scaleToneOffsetAbove (scale, degree, 4);
        c.semitonesFromRoot = { 0, third, fifth };
        c.minorQuality = (third == 3);   // minor or diminished third

        const bool preferFlats = keyPrefersFlats (keyRootPc, isMinorFlavoured (scale));
        rebuildSymbol (c, preferFlats);
        return c;
    }

    void applyExtension (ChordSpec& c, int keyRootPc, ScaleType scale, ChordExtension ext)
    {
        scale = chordScaleFor (scale);
        c.extension = ext;

        // Rebuild the shape from the triad each time so repeated calls are safe.
        const int third = c.semitonesFromRoot.size() > 1 ? c.semitonesFromRoot[1] : (c.minorQuality ? 3 : 4);
        const int fifth = c.semitonesFromRoot.size() > 2 ? c.semitonesFromRoot[2] : 7;

        // Diatonic 7th/9th offsets relative to THIS chord's root, derived from
        // its scale degree so colour tones stay in key.
        const int seventh = scaleToneOffsetAbove (scale, c.degree, 6);
        const int ninth   = scaleToneOffsetAbove (scale, c.degree, 8);  // 9th == 2 above octave

        switch (ext)
        {
            case ChordExtension::None:
                c.semitonesFromRoot = { 0, third, fifth };
                break;
            case ChordExtension::Seventh:
                c.semitonesFromRoot = { 0, third, fifth, seventh };
                break;
            case ChordExtension::Ninth:
                c.semitonesFromRoot = { 0, third, fifth, seventh, ninth };
                break;
            case ChordExtension::Add9:
                c.semitonesFromRoot = { 0, third, fifth, ninth };
                break;
            case ChordExtension::Sus2:
            {
                const int second = scaleToneOffsetAbove (scale, c.degree, 1);
                c.semitonesFromRoot = { 0, second, fifth };
                c.minorQuality = false;   // sus has no third
                break;
            }
            case ChordExtension::Sus4:
            {
                const int fourth = scaleToneOffsetAbove (scale, c.degree, 3);
                c.semitonesFromRoot = { 0, fourth, fifth };
                c.minorQuality = false;
                break;
            }
            case ChordExtension::Sixth:
            {
                const int sixth = scaleToneOffsetAbove (scale, c.degree, 5);
                c.semitonesFromRoot = { 0, third, fifth, sixth };
                break;
            }
            case ChordExtension::SixNine:
            {
                const int sixth = scaleToneOffsetAbove (scale, c.degree, 5);
                c.semitonesFromRoot = { 0, third, fifth, sixth, ninth };
                break;
            }
        }

        const bool preferFlats = keyPrefersFlats (keyRootPc, isMinorFlavoured (scale));
        rebuildSymbol (c, preferFlats);
    }

    // -------------------------------------------------------------------------
    // Roman numerals.
    // -------------------------------------------------------------------------
    std::optional<RomanChord> parseRoman (std::string_view sv)
    {
        RomanChord rc;
        size_t i = 0;
        if (i < sv.size() && (sv[i] == 'b' || sv[i] == '#'))
        {
            rc.flat = (sv[i] == 'b');
            ++i;
        }

        // Read the numeral (I..VII / i..vii).
        std::string num;
        while (i < sv.size() &&
               (sv[i]=='I'||sv[i]=='V'||sv[i]=='i'||sv[i]=='v'))
            num += sv[i++];
        if (num.empty()) return std::nullopt;

        std::string upper;
        for (char c : num) upper += (char) std::toupper ((unsigned char) c);
        rc.lowerCase = std::islower ((unsigned char) num[0]) != 0;

        int degree = -1;
        if      (upper == "I")   degree = 0;
        else if (upper == "II")  degree = 1;
        else if (upper == "III") degree = 2;
        else if (upper == "IV")  degree = 3;
        else if (upper == "V")   degree = 4;
        else if (upper == "VI")  degree = 5;
        else if (upper == "VII") degree = 6;
        else return std::nullopt;
        rc.degree = degree;

        // Optional quality suffix.
        std::string suffix (sv.substr (i));
        if      (suffix == "maj7") rc.extension = ChordExtension::Seventh;
        else if (suffix == "m7" || suffix == "7") rc.extension = ChordExtension::Seventh;
        else if (suffix == "9" || suffix == "m9" || suffix == "maj9") rc.extension = ChordExtension::Ninth;
        else if (suffix == "add9") rc.extension = ChordExtension::Add9;
        else if (suffix == "sus2") rc.extension = ChordExtension::Sus2;
        else if (suffix == "sus4") rc.extension = ChordExtension::Sus4;
        else if (suffix == "6")    rc.extension = ChordExtension::Sixth;
        else if (suffix == "6/9" || suffix == "69") rc.extension = ChordExtension::SixNine;
        else rc.extension = ChordExtension::None;

        return rc;
    }

    ChordSpec resolveRoman (std::string_view roman, int keyRootPc, ScaleType scale)
    {
        const auto parsed = parseRoman (roman);
        const ScaleType cScale = chordScaleFor (scale);
        keyRootPc = ((keyRootPc % 12) + 12) % 12;

        if (! parsed)
            return diatonicTriad (keyRootPc, cScale, 0);

        const RomanChord rc = *parsed;

        ChordSpec c;
        c.roman = std::string (roman);

        if (rc.flat)
        {
            // Borrowed flat-degree chord. "bVII"/"bVI" name fixed pitch classes
            // relative to the tonic (b7 = tonic+10, b6 = tonic+8), i.e. a
            // semitone below the PARALLEL-MAJOR degree — NOT below the current
            // scale's degree, whose 6th/7th are already lowered in minor modes.
            // Using the current scale there would over-flatten the root.
            static const int majorIv[7] = { 0, 2, 4, 5, 7, 9, 11 };
            const int parallelMajorRootPc = (keyRootPc + majorIv[rc.degree % 7]) % 12;
            c.rootPc = ((parallelMajorRootPc - 1) % 12 + 12) % 12;
            c.degree = rc.degree;
            c.flattened = true;
            c.minorQuality = rc.lowerCase;
            const int third = rc.lowerCase ? 3 : 4;
            c.semitonesFromRoot = { 0, third, 7 };
        }
        else
        {
            c = diatonicTriad (keyRootPc, cScale, rc.degree);
            // Honour an explicit case override (e.g. Picardy major I in minor).
            if (! rc.lowerCase && c.minorQuality)
            {
                c.minorQuality = false;
                c.semitonesFromRoot = { 0, 4, 7 };
            }
            else if (rc.lowerCase && ! c.minorQuality)
            {
                c.minorQuality = true;
                c.semitonesFromRoot = { 0, 3, 7 };
            }
        }

        if (rc.extension != ChordExtension::None)
        {
            // For borrowed chords the diatonic-degree colour still reads well;
            // reuse applyExtension which stacks scale tones for non-flat chords
            // and falls back gracefully for flat ones.
            if (! c.flattened)
                applyExtension (c, keyRootPc, cScale, rc.extension);
            else
            {
                // Simple idiomatic colour for borrowed chords.
                if (rc.extension == ChordExtension::Seventh)
                    c.semitonesFromRoot.push_back (c.minorQuality ? 10 : 11);
                else if (rc.extension == ChordExtension::Add9)
                    c.semitonesFromRoot.push_back (14);
                c.extension = rc.extension;
            }
        }

        const bool preferFlats = keyPrefersFlats (keyRootPc, isMinorFlavoured (scale));
        rebuildSymbol (c, preferFlats);
        return c;
    }
}
