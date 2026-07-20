// =============================================================================
//  GeneratedPattern.h
//  The complete result of one generation: 4 voiced chords, realized chord
//  notes, melody notes, and every piece of context needed to reproduce or
//  export it. Pure C++ (no JUCE); (de)serialization lives in IO/PatternState.
// =============================================================================
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "Note.h"
#include "../Theory/Theory.h"
#include "../Engines/RandomEngine.h"

namespace acai
{
    /// One chord of the progression after voice leading: the abstract spec
    /// plus the concrete voiced MIDI pitches and its position in the loop.
    struct VoicedChord
    {
        theory::ChordSpec spec;
        std::vector<int>  pitches;      ///< voiced MIDI notes, ascending
        int startTick  = 0;
        int lengthTick = 0;             ///< nominal length (before sustain/overlap)
    };

    /// Time signature info shared by generation, playback and export.
    struct TimeSignature
    {
        int numerator   = 4;   ///< beats per bar as written (4/4, 3/4, 6/8)
        int denominator = 4;

        /// Length of one bar in ticks (denominator 4 -> beat = quarter = kPPQ,
        /// denominator 8 -> beat = eighth = kPPQ/2).
        int barLengthTicks() const noexcept
        {
            return numerator * (kPPQ * 4 / denominator);
        }

        /// The grid slot (in ticks) treated as one "beat" for strong/weak logic.
        /// In 6/8 the felt pulse is the dotted quarter (2 groups of 3 eighths).
        int strongBeatEveryTicks() const noexcept
        {
            if (denominator == 8) return 3 * (kPPQ / 2);   // dotted quarter
            return kPPQ;                                   // quarter
        }
    };

    struct GeneratedPattern
    {
        // --- the music -------------------------------------------------------
        std::vector<VoicedChord> chords;      ///< exactly 4 after generation
        std::vector<Note>        chordNotes;  ///< realized pad notes (post-humanize)
        std::vector<Note>        melodyNotes; ///< realized lead notes (post-humanize)

        /// Future tracks (bass, counter-melody, arp...) — see IGenerator.h.
        std::map<std::string, std::vector<Note>> extraTracks;

        // --- reproducibility & context --------------------------------------
        Seeds  seeds;
        int    keyRootPc   = 0;
        theory::ScaleType scale = theory::ScaleType::Minor;
        int    moodIndex   = 0;
        double bpm         = 72.0;
        TimeSignature timeSig;
        int    barsPerChord = 2;
        int    lengthTicks  = 0;              ///< total loop length

        // --- convenience -----------------------------------------------------
        bool empty() const noexcept { return chordNotes.empty() && melodyNotes.empty(); }

        int totalBars() const noexcept
        {
            const int bar = timeSig.barLengthTicks();
            return bar > 0 ? (lengthTicks + bar - 1) / bar : 0;
        }

        /// FNV-1a over the progression identity (degree/quality/extension/
        /// inversion order) — used by the session-uniqueness check.
        uint64_t chordFingerprint() const
        {
            uint64_t h = 1469598103934665603ull;
            auto mix = [&h] (uint64_t v) { h ^= v; h *= 1099511628211ull; };
            for (const auto& c : chords)
            {
                mix ((uint64_t) (c.spec.rootPc + 1));
                mix ((uint64_t) (c.spec.minorQuality ? 2 : 3));
                mix ((uint64_t) c.spec.extension + 10);
                for (int p : c.pitches) mix ((uint64_t) (p + 200));
            }
            return h;
        }

        /// FNV-1a over melody pitches + onset grid — uniqueness check.
        uint64_t melodyFingerprint() const
        {
            uint64_t h = 1469598103934665603ull;
            auto mix = [&h] (uint64_t v) { h ^= v; h *= 1099511628211ull; };
            for (const auto& n : melodyNotes)
            {
                mix ((uint64_t) (n.pitch + 1));
                mix ((uint64_t) (n.startTick / (kPPQ / 8) + 3));  // 32nd-note grid
            }
            return h;
        }

        /// Human-readable progression, e.g. "Am9 – Fmaj7 – Cadd9 – G6".
        std::string progressionText() const
        {
            std::string out;
            for (size_t i = 0; i < chords.size(); ++i)
            {
                if (i > 0) out += "  –  ";
                out += chords[i].spec.symbol;
            }
            return out;
        }
    };
}
