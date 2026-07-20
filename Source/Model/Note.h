// =============================================================================
//  Note.h
//  The atomic musical event. Pure C++ (no JUCE) so the whole generation core
//  can be unit-tested headlessly.
//
//  Time is measured in MIDI ticks at a fixed resolution of 960 PPQ
//  (ticks per quarter note) — the same resolution written into exported files.
// =============================================================================
#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace acai
{
    /// Ticks per quarter note used across the entire plugin (generation,
    /// preview playback and MIDI export all share this grid).
    inline constexpr int kPPQ = 960;

    enum class NoteSource : uint8_t
    {
        chord,   ///< belongs to the pad / chord track
        melody   ///< belongs to the lead / melody track
    };

    struct Note
    {
        int        pitch      = 60;                 ///< MIDI note number 0..127
        int        startTick  = 0;                  ///< onset, in ticks (>= 0)
        int        lengthTick = kPPQ;               ///< duration, in ticks (> 0)
        int        velocity   = 90;                 ///< MIDI velocity 1..127
        NoteSource source     = NoteSource::melody;

        /// Marks a deliberate chromatic passing tone the melody engine created
        /// on purpose. The CompositionValidator lets these through (they are
        /// resolved by construction) while nudging accidental out-of-scale notes.
        bool isChromaticPassing = false;

        int endTick() const noexcept { return startTick + lengthTick; }
    };

    // -------------------------------------------------------------------------
    // Same-pitch overlap sanitiser.
    //
    // Voice leading keeps common tones, so with sustain/overlap the SAME pitch
    // can sound across two chords at once. MIDI cannot represent two note-ons of
    // one pitch without a note-off between them (JUCE would silently insert one,
    // leaving a dangling note-off). We resolve it musically: the earlier note is
    // truncated to end where the next same-pitch note begins (a clean common-tone
    // re-articulation). Notes that share both pitch AND onset are merged.
    //
    // Run this per track as the final step, after humanisation (which can shift
    // starts/lengths). It never creates negative or zero-length notes.
    // -------------------------------------------------------------------------
    inline void sanitizeSamePitchOverlaps (std::vector<Note>& notes)
    {
        if (notes.size() < 2) return;

        std::sort (notes.begin(), notes.end(), [] (const Note& a, const Note& b)
        {
            if (a.pitch != b.pitch)         return a.pitch < b.pitch;
            if (a.startTick != b.startTick) return a.startTick < b.startTick;
            return a.lengthTick > b.lengthTick;
        });

        std::vector<Note> out;
        out.reserve (notes.size());
        for (auto& n : notes)
        {
            if (! out.empty()
                && out.back().pitch == n.pitch
                && n.startTick <= out.back().startTick)
            {
                // identical onset (or earlier after sort ties) -> keep the
                // louder / longer one already stored, drop this duplicate.
                if (n.velocity > out.back().velocity) out.back() = n;
                continue;
            }
            if (! out.empty()
                && out.back().pitch == n.pitch
                && out.back().endTick() > n.startTick)
            {
                // overlap -> truncate the earlier note to abut this one.
                out.back().lengthTick = std::max (1, n.startTick - out.back().startTick);
            }
            out.push_back (n);
        }

        // Restore chronological order for downstream consumers.
        std::sort (out.begin(), out.end(), [] (const Note& a, const Note& b)
        {
            if (a.startTick != b.startTick) return a.startTick < b.startTick;
            return a.pitch < b.pitch;
        });
        notes.swap (out);
    }
}
