// =============================================================================
//  CompositionValidator.cpp
//
//  Post-generation guarantee pass over the melody (spec §11). It:
//   * nudges out-of-scale notes back to scale (except deliberate, flagged
//     chromatic passing tones)
//   * resolves harsh clashes (min-2nd / min-9th against a sounding chord tone
//     in a close register) to the nearest consonant chord tone
//   * caps impossible leaps to a musical maximum
//   * breaks exact copy-paste repetition by forcing a small variation
//  Returns acceptable=false only when the melody is essentially empty, so the
//  caller can re-roll.
// =============================================================================
#include "CompositionValidator.h"

#include <algorithm>
#include <cmath>

namespace acai
{
    namespace
    {
        // Which chord sounds at a given tick.
        const VoicedChord* chordAt (const GeneratedPattern& p, int tick)
        {
            const VoicedChord* found = nullptr;
            for (const auto& c : p.chords)
                if (tick >= c.startTick && tick < c.startTick + c.lengthTick)
                    found = &c;
            if (! found && ! p.chords.empty())
                found = &p.chords.back();
            return found;
        }

        // Harsh interval against a sounding chord tone in the SAME octave region:
        // a minor 2nd (1 semitone) or minor 9th (13) is the classic ambient clash.
        // A note that is ITSELF a chord tone is never a clash — landing on the
        // maj7 of a maj7 chord (a semitone below the root) is intended colour.
        bool clashesWithChord (int pitch, const VoicedChord& chord)
        {
            const auto pcs = chord.spec.pitchClasses();
            const int pitchPc = ((pitch % 12) + 12) % 12;
            if (std::find (pcs.begin(), pcs.end(), pitchPc) != pcs.end())
                return false;   // it's a chord tone

            for (int cp : chord.pitches)
            {
                const int d = std::abs (pitch - cp);
                if (d == 1 || d == 13) return true;
            }
            return false;
        }
    }

    ValidationReport CompositionValidator::validateAndFix (GeneratedPattern& pattern,
                                                           const MusicalContext& ctx,
                                                           const MoodProfile& mood,
                                                           RandomEngine& rng)
    {
        ValidationReport report;
        auto& notes = pattern.melodyNotes;
        if (notes.size() < 2)
        {
            report.acceptable = ! notes.empty();
            return report;
        }

        std::sort (notes.begin(), notes.end(),
                   [] (const Note& a, const Note& b) { return a.startTick < b.startTick; });

        // Musical max leap (semitones). A single expressive octave leap is
        // allowed when the mood is leap-happy; otherwise cap tighter.
        const int maxLeap = (mood.leapProbability > 0.16) ? 12 : 9;

        int prevPitch = notes.front().pitch;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            Note& n = notes[i];

            // 1) out-of-scale (unless a deliberate, flagged passing tone)
            if (! n.isChromaticPassing
                && ! theory::isInScale (n.pitch, ctx.keyRootPc, ctx.scale))
            {
                n.pitch = theory::nearestScaleTone (n.pitch, ctx.keyRootPc, ctx.scale);
                ++report.notesNudged;
            }

            // 2) clash with the sounding chord. Passing dissonance is a feature
            //    of good melodic writing, so we only resolve clashes that are
            //    ACCENTED (on a strong beat) or SUSTAINED (a beat or longer) —
            //    a short weak-beat note a step from a chord tone is left as an
            //    expressive passing/neighbour tone. Flagged chromatic passing
            //    tones are always exempt.
            if (! n.isChromaticPassing)
            {
                const int beat = pattern.timeSig.strongBeatEveryTicks();
                const bool onStrongBeat = beat > 0 && (n.startTick % beat) < (kPPQ / 8);
                const bool sustained = n.lengthTick >= beat;
                if (onStrongBeat || sustained)
                {
                    if (const VoicedChord* c = chordAt (pattern, n.startTick))
                    {
                        if (clashesWithChord (n.pitch, *c))
                        {
                            n.pitch = theory::nearestChordTone (n.pitch, c->spec.pitchClasses());
                            ++report.notesNudged;
                        }
                    }
                }
            }

            // 3) impossible leap from the previous note
            if (i > 0)
            {
                const int leap = n.pitch - prevPitch;
                if (std::abs (leap) > maxLeap)
                {
                    const int dir = leap > 0 ? 1 : -1;
                    int target = prevPitch + dir * maxLeap;
                    // snap the capped target back into the scale / chord
                    if (const VoicedChord* c = chordAt (pattern, n.startTick))
                        target = theory::nearestChordTone (target, c->spec.pitchClasses());
                    else
                        target = theory::nearestScaleTone (target, ctx.keyRootPc, ctx.scale);
                    n.pitch = theory::clampMidi (target);
                    ++report.leapsCapped;
                }
            }

            prevPitch = n.pitch;
        }

        // 4) repetitive-loop guard: if every phrase is a byte-identical pitch
        //    sequence, force a small variation into the second half.
        //    (We compare the pitch contour of the first vs second half.)
        if (notes.size() >= 4)
        {
            const size_t half = notes.size() / 2;
            bool identical = true;
            for (size_t i = 0; i < half && (i + half) < notes.size(); ++i)
                if (notes[i].pitch != notes[i + half].pitch) { identical = false; break; }

            if (identical)
            {
                // nudge a couple of notes in the second half by a scale step
                for (size_t i = half; i < notes.size(); i += 3)
                {
                    Note& n = notes[i];
                    const int dir = rng.chance (0.5) ? 1 : -1;
                    int cand = theory::nearestScaleTone (n.pitch + dir * 2, ctx.keyRootPc, ctx.scale);
                    // keep it consonant with the chord
                    if (const VoicedChord* c = chordAt (pattern, n.startTick))
                        if (clashesWithChord (cand, *c))
                            cand = theory::nearestChordTone (cand, c->spec.pitchClasses());
                    n.pitch = theory::clampMidi (cand);
                }
                report.repetitionFixed = true;
            }
        }

        // 5) final consonance guarantee. After the clash/leap fixes a note may
        //    sit on a BORROWED chord's tone (outside the parent scale but still
        //    consonant with the sounding chord). That is allowed. Anything that
        //    is neither in-scale, nor a tone of the sounding chord, nor a
        //    deliberate flagged passing tone is forced back to the scale — so
        //    the invariant "in-scale OR sounding-chord-tone OR flagged" holds.
        for (auto& n : notes)
        {
            if (n.isChromaticPassing) continue;
            if (theory::isInScale (n.pitch, ctx.keyRootPc, ctx.scale)) continue;

            bool chordTone = false;
            if (const VoicedChord* c = chordAt (pattern, n.startTick))
            {
                const auto pcs = c->spec.pitchClasses();
                const int notePc = ((n.pitch % 12) + 12) % 12;
                chordTone = std::find (pcs.begin(), pcs.end(), notePc) != pcs.end();
            }
            if (! chordTone)
            {
                n.pitch = theory::nearestScaleTone (n.pitch, ctx.keyRootPc, ctx.scale);
                ++report.notesNudged;
            }
        }

        report.acceptable = true;
        return report;
    }
}
