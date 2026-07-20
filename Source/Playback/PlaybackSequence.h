// =============================================================================
//  PlaybackSequence.h
//  A GeneratedPattern flattened into a time-sorted note-on/off event list —
//  the ONLY representation the audio thread ever touches. Built on the message
//  thread, published via shared_ptr swap, read lock-free-ish during playback.
//  Pure C++ (no JUCE).
// =============================================================================
#pragma once

#include <algorithm>
#include <memory>
#include <vector>

#include "../Model/GeneratedPattern.h"

namespace acai
{
    struct PlaybackEvent
    {
        int  tick     = 0;
        int  pitch    = 60;
        int  velocity = 0;        ///< 0 => note-off
        bool isNoteOn = false;
        NoteSource source = NoteSource::melody;
    };

    struct PlaybackSequence
    {
        std::vector<PlaybackEvent> events;   ///< sorted by tick (offs before ons at same tick)
        int lengthTicks = 0;

        /// Flatten a pattern (both tracks) into a sorted event list.
        /// Note-offs are clamped to the loop end so looping can never leave a
        /// note hanging past the wrap point.
        static std::shared_ptr<const PlaybackSequence> build (const GeneratedPattern& p)
        {
            auto seq = std::make_shared<PlaybackSequence>();
            seq->lengthTicks = std::max (1, p.lengthTicks);

            auto addNotes = [&seq] (const std::vector<Note>& notes)
            {
                for (const auto& n : notes)
                {
                    const int start = std::clamp (n.startTick, 0, seq->lengthTicks - 1);
                    const int end   = std::clamp (n.endTick(), start + 1, seq->lengthTicks);
                    seq->events.push_back ({ start, n.pitch, std::clamp (n.velocity, 1, 127),
                                             true,  n.source });
                    seq->events.push_back ({ end,   n.pitch, 0, false, n.source });
                }
            };
            addNotes (p.chordNotes);
            addNotes (p.melodyNotes);
            for (const auto& [name, notes] : p.extraTracks)
                addNotes (notes);

            std::stable_sort (seq->events.begin(), seq->events.end(),
                [] (const PlaybackEvent& a, const PlaybackEvent& b)
                {
                    if (a.tick != b.tick) return a.tick < b.tick;
                    return ! a.isNoteOn && b.isNoteOn;   // offs first at equal ticks
                });
            return seq;
        }
    };
}
