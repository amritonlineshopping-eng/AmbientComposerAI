// =============================================================================
//  MelodyEngine.cpp
//
//  Constraint-based, never-random-sounding melody generation (spec §8):
//   * a short motif is built, then repeated across the 4 chord regions with
//     variation (transpose to fit each chord, alter an interval, extend/omit)
//   * strong beats target chord tones of the current chord; weak beats allow
//     stepwise passing / neighbour tones from the scale
//   * stepwise motion is preferred; occasional expressive leaps recover by
//     stepping back the opposite way
//   * rests give the line room to breathe (mood restProbability, more at phrase
//     ends); the final phrase resolves onto a chord tone (cadence)
//   * Complexity (0..1) scales passing-tone frequency, subdivision, interval
//     size, syncopation and overall density (spec §8-complexity)
// =============================================================================
#include "MelodyEngine.h"

#include <algorithm>
#include <cmath>

namespace acai
{
    using theory::ScaleType;

    namespace
    {
        // All in-scale MIDI notes 0..127, ascending — the "ladder" the melody
        // walks so scale-step motion is trivial and always in key.
        std::vector<int> buildScaleLadder (int keyRootPc, ScaleType scale)
        {
            std::vector<int> ladder;
            for (int n = 0; n <= 127; ++n)
                if (theory::isInScale (n, keyRootPc, scale))
                    ladder.push_back (n);
            return ladder;
        }

        int nearestLadderIndex (const std::vector<int>& ladder, int midi)
        {
            if (ladder.empty()) return 0;
            int bestIdx = 0, bestDist = 1 << 30;
            for (int i = 0; i < (int) ladder.size(); ++i)
            {
                const int d = std::abs (ladder[i] - midi);
                if (d < bestDist) { bestDist = d; bestIdx = i; }
                else if (ladder[i] > midi && d > bestDist) break;   // ascending, past nearest
            }
            return bestIdx;
        }

        int moveInScale (const std::vector<int>& ladder, int fromMidi, int steps)
        {
            if (ladder.empty()) return fromMidi;
            const int idx = nearestLadderIndex (ladder, fromMidi);
            const int idx2 = std::clamp (idx + steps, 0, (int) ladder.size() - 1);
            return ladder[idx2];
        }

        // One motif cell: a scale-step delta from the previous note (contour),
        // whether it should land on a chord tone, and its relative strength.
        struct MotifCell
        {
            int  stepDelta = 0;   ///< scale steps from previous note
            bool rest = false;
            bool wantChordTone = false;
        };

        // Velocity across a phrase according to the mood's curve.
        int phraseVelocity (const MoodProfile& mood, double phasePos /*0..1*/, RandomEngine& rng)
        {
            double shape = 0.0;
            switch (mood.velocityCurve)
            {
                case VelocityCurve::Flat:  shape = 0.0; break;
                case VelocityCurve::Swell: shape = std::sin (phasePos * 3.14159265) - 0.5; break; // rise then fall
                case VelocityCurve::Fade:  shape = 0.5 - phasePos; break;                          // gentle decay
            }
            const int base = mood.velocityBase + (int) std::lround (shape * mood.velocityRange);
            const int jitter = rng.nextInt (-mood.velocityRange / 4, mood.velocityRange / 4);
            return std::clamp (base + jitter, 20, 122);
        }

        // The felt strong-beat test within a chord region.
        bool isStrong (int localTick, const TimeSignature& ts)
        {
            const int beat = ts.strongBeatEveryTicks();
            return beat > 0 && (localTick % beat) == 0;
        }
    }

    std::vector<Note> MelodyEngine::generate (const std::vector<VoicedChord>& chords,
                                              const MusicalContext& ctx,
                                              const MoodProfile& mood,
                                              const MelodyEngineSettings& settings,
                                              RandomEngine& rng)
    {
        std::vector<Note> out;
        if (chords.empty()) return out;

        const auto ladder = buildScaleLadder (ctx.keyRootPc, ctx.scale);
        if (ladder.empty()) return out;

        const int barTicks   = settings.timeSig.barLengthTicks();
        const int chordTicks = barTicks * settings.barsPerChord;

        // Melody register: base around C{melodyOctave}, biased by mood register.
        const int registerShift = (int) std::lround (mood.registerBias * 12.0);
        const int melodyBase = theory::clampMidi (
            12 * (settings.melodyOctave + 1) + ctx.keyRootPc + registerShift);
        const int rangeLo = theory::clampMidi (melodyBase - 5);
        const int rangeHi = theory::clampMidi (melodyBase + 14);   // ~ an octave + a little

        // ---- rhythmic grid for one phrase -----------------------------------
        // Base note value from the rhythm style; complexity subdivides & adds
        // syncopation; density/rest decisions come later per note.
        auto baseUnit = [&] () -> int
        {
            switch ((RhythmStyle) settings.rhythmStyle)
            {
                case RhythmStyle::Whole:      return 4 * kPPQ;
                case RhythmStyle::Half:       return 2 * kPPQ;
                case RhythmStyle::Quarter:    return kPPQ;
                case RhythmStyle::Eighth:     return kPPQ / 2;
                case RhythmStyle::Sixteenth:  return kPPQ / 4;
                case RhythmStyle::Mixed:      return kPPQ;
                case RhythmStyle::FreeAmbient:return kPPQ / 2;
                default:                      return kPPQ;
            }
        }();

        // Build candidate onsets across a phrase (chordTicks long).
        struct Slot { int onset; int dur; bool strong; };
        auto buildSlots = [&] (RandomEngine& r) -> std::vector<Slot>
        {
            std::vector<Slot> slots;
            const bool freeStyle = ((RhythmStyle) settings.rhythmStyle == RhythmStyle::FreeAmbient
                                 || (RhythmStyle) settings.rhythmStyle == RhythmStyle::Mixed);
            int t = 0;
            while (t < chordTicks)
            {
                int dur = baseUnit;

                // Complexity: subdivide this slot into finer notes sometimes.
                if (r.chance (settings.complexity * 0.6) && dur >= kPPQ / 2)
                    dur /= 2;

                // Mixed / Free: vary durations for a rubato, spacious feel.
                if (freeStyle)
                {
                    const int roll = r.nextInt (0, 3);
                    if (roll == 0)      dur = baseUnit * 2;
                    else if (roll == 1) dur = baseUnit;
                    else if (roll == 2) dur = baseUnit;
                    else                dur = std::max (kPPQ / 4, baseUnit / 2);
                }

                dur = std::min (dur, chordTicks - t);
                if (dur <= 0) break;

                bool strong = isStrong (t, settings.timeSig);

                // Syncopation: nudge a weak onset forward by half a unit.
                slots.push_back ({ t, dur, strong });
                t += dur;
            }

            // Apply syncopation as a post-pass on weak slots (keeps grid clean).
            if (settings.complexity > 0.35)
                for (auto& s : slots)
                    if (! s.strong && r.chance (settings.complexity * 0.4)
                        && s.dur >= kPPQ / 2)
                    {
                        const int shift = kPPQ / 4;
                        if (s.onset + shift + s.dur <= chordTicks) s.onset += shift;
                    }
            return slots;
        };

        // ---- motif contour (generated once, varied per phrase) ---------------
        // Density drives how many cells are notes vs rests; contour tendency
        // shapes the stepDelta sequence; leaps recover by stepping back.
        auto styleDensity = [&] ()
        {
            double d = mood.noteDensity;
            switch (settings.style)
            {
                case MelodyStyle::Minimal:  d *= 0.5; break;
                case MelodyStyle::Flowing:  d = std::min (1.0, d * 1.4); break;
                case MelodyStyle::Arpeggiated: d = std::min (1.0, d * 1.3); break;
                default: break;
            }
            return std::clamp (d + settings.complexity * 0.25, 0.08, 1.0);
        }();

        // Build the reference slot layout (phrase 0) so all phrases share pulse.
        auto motifRng = rng.derive (0x11);
        const std::vector<Slot> phraseSlots = buildSlots (motifRng);

        // Generate a contour: one MotifCell per slot.
        std::vector<MotifCell> motif;
        motif.reserve (phraseSlots.size());
        int lastStep = 0;
        const int maxLeap = 2 + (int) std::lround (settings.complexity * 3.0);  // in scale steps
        for (size_t i = 0; i < phraseSlots.size(); ++i)
        {
            MotifCell cell;
            const bool strong = phraseSlots[i].strong;
            cell.wantChordTone = strong;

            // rest decision — more likely on weak cells and at the phrase tail
            double restP = mood.restProbability;
            if (! strong) restP += 0.08;
            if (i + 1 == phraseSlots.size()) restP += 0.15;            // breathe at phrase end
            restP *= (1.2 - styleDensity);                            // denser => fewer rests
            if (settings.style == MelodyStyle::Flowing) restP *= 0.5;
            if (i == 0) restP = 0.0;                                  // always start the phrase
            cell.rest = motifRng.chance (std::clamp (restP, 0.0, 0.85));

            // contour step
            if (cell.rest) { cell.stepDelta = 0; motif.push_back (cell); continue; }

            int step = 0;
            const bool doLeap = motifRng.chance (mood.leapProbability + settings.complexity * 0.1);
            if (std::abs (lastStep) >= 3)
            {
                // leap recovery: step back opposite the last leap
                step = (lastStep > 0 ? -1 : 1) * motifRng.nextInt (1, 2);
            }
            else if (doLeap)
            {
                const int mag = motifRng.nextInt (3, std::max (3, maxLeap));
                step = motifRng.chance (0.5) ? mag : -mag;
            }
            else
            {
                // stepwise, direction shaped by the mood contour
                int dir = 0;
                switch (mood.contour)
                {
                    case Contour::Ascending:  dir = motifRng.chance (0.72) ? 1 : -1; break;
                    case Contour::Descending: dir = motifRng.chance (0.72) ? -1 : 1; break;
                    case Contour::Arch:       dir = (i < phraseSlots.size()/2) ? 1 : -1;
                                              if (motifRng.chance (0.25)) dir = -dir; break;
                    case Contour::Wave:       dir = motifRng.chance (0.5) ? 1 : -1; break;
                    case Contour::Floating:   dir = motifRng.chance (0.5) ? 1 : -1;
                                              if (motifRng.chance (0.4)) step = 0; break;
                }
                if (step == 0 && dir != 0) step = dir;
            }
            cell.stepDelta = step;
            lastStep = step;
            motif.push_back (cell);
        }

        // ---- render the motif across the 4 chord regions --------------------
        auto renderRng = rng.derive (0x22);
        int currentPitch = -1;   // seeded from the first chord

        for (size_t region = 0; region < chords.size(); ++region)
        {
            const auto& chord = chords[region];
            const auto chordPcs = chord.spec.pitchClasses();
            const int regionStart = (int) region * chordTicks;

            // Per-phrase variation (spec §8.1): from phrase 1 on, alter the
            // motif so it repeats-with-variation rather than copy-pasting.
            const bool vary = (region > 0);
            const int transposeSteps = vary ? renderRng.nextInt (-1, 1) : 0;
            const int alterIndex = (vary && ! motif.empty())
                                   ? renderRng.nextInt (0, (int) motif.size() - 1) : -1;

            // Anchor the phrase on a chord tone near the register (or continue
            // from the previous pitch for legato continuity).
            if (currentPitch < 0)
                currentPitch = theory::nearestChordTone (melodyBase, chordPcs);

            const bool isLastRegion = (region + 1 == chords.size());

            for (size_t i = 0; i < motif.size() && i < phraseSlots.size(); ++i)
            {
                MotifCell cell = motif[i];
                if ((int) i == alterIndex && ! cell.rest)
                    cell.stepDelta += renderRng.chance (0.5) ? 1 : -1;   // alter one interval

                const Slot& slot = phraseSlots[i];
                const int onset = regionStart + slot.onset;

                if (cell.rest) continue;

                // propose next pitch by walking the scale ladder
                int step = cell.stepDelta + (i == 0 ? transposeSteps : 0);
                int proposed = moveInScale (ladder, currentPitch, step);

                // strong beats -> chord tone; weak beats -> nearest scale tone
                bool chromaticPassing = false;
                if (cell.wantChordTone)
                {
                    proposed = theory::nearestChordTone (proposed, chordPcs);
                }
                else
                {
                    // weak beat: keep as scale tone; occasionally (high
                    // complexity) allow a chromatic passing tone that sits
                    // between the previous and next scale tones.
                    proposed = theory::nearestScaleTone (proposed, ctx.keyRootPc, ctx.scale);
                    if (renderRng.chance (settings.complexity * 0.15))
                    {
                        const int between = (currentPitch + proposed) / 2;
                        if (! theory::isInScale (between, ctx.keyRootPc, ctx.scale)
                            && std::abs (between - currentPitch) <= 2)
                        {
                            proposed = between;
                            chromaticPassing = true;
                        }
                    }
                }

                // keep within range; if we hit a wall, reflect back inward
                if (proposed < rangeLo) proposed = moveInScale (ladder, rangeLo, renderRng.nextInt (0, 2));
                if (proposed > rangeHi) proposed = moveInScale (ladder, rangeHi, -renderRng.nextInt (0, 2));
                proposed = theory::clampMidi (proposed);

                Note n;
                n.pitch = proposed;
                n.startTick = onset;
                // gate length from mood, relative to the slot
                n.lengthTick = std::max (kPPQ / 8, (int) std::lround (slot.dur * mood.gateLength));
                const double phasePos = (double) (onset) / (double) std::max (1, chordTicks * 4);
                n.velocity = phraseVelocity (mood, phasePos, renderRng);
                n.source = NoteSource::melody;
                n.isChromaticPassing = chromaticPassing;
                out.push_back (n);

                currentPitch = proposed;
            }

            // ---- cadence on the final region --------------------------------
            if (isLastRegion)
            {
                // Ensure the last sounding note resolves to a strong chord tone
                // (root/3rd/5th) of the final chord, and let it ring.
                if (! out.empty())
                {
                    // choose root, 3rd or 5th of the final chord near current pitch
                    std::vector<int> strongTones;
                    for (size_t k = 0; k < std::min<size_t> (3, chord.spec.semitonesFromRoot.size()); ++k)
                        strongTones.push_back ((chord.spec.rootPc + chord.spec.semitonesFromRoot[k]) % 12);
                    if (strongTones.empty()) strongTones = chordPcs;

                    Note& last = out.back();
                    last.pitch = theory::nearestChordTone (last.pitch, strongTones);
                    last.isChromaticPassing = false;
                    // let the final note breathe to (near) the loop end
                    const int loopEnd = chordTicks * 4;
                    last.lengthTick = std::max (last.lengthTick,
                                                std::min (kPPQ * 2, loopEnd - last.startTick));
                }
            }
        }

        // Arpeggiated style: overlay/replace with broken-chord figures if chosen.
        if (settings.style == MelodyStyle::Arpeggiated)
        {
            out.clear();
            auto arpRng = rng.derive (0x33);
            const int unit = std::max (kPPQ / 4, baseUnit / 2);
            for (size_t region = 0; region < chords.size(); ++region)
            {
                const auto& chord = chords[region];
                auto pcs = chord.spec.pitchClasses();
                std::sort (pcs.begin(), pcs.end());
                const int regionStart = (int) region * chordTicks;
                int idx = 0;
                for (int t = 0; t < chordTicks; t += unit)
                {
                    if (arpRng.chance (mood.restProbability * 0.6)) { ++idx; continue; }
                    const int pc = pcs[(size_t) (idx % (int) pcs.size())];
                    int pitch = theory::nearestChordTone (melodyBase, { pc });
                    // ascend across the octave as the arp climbs
                    pitch += 12 * ((idx / (int) pcs.size()) % 2);
                    pitch = std::clamp (pitch, rangeLo, rangeHi + 5);
                    Note n;
                    n.pitch = theory::clampMidi (pitch);
                    n.startTick = regionStart + t;
                    n.lengthTick = std::max (kPPQ / 8, (int) std::lround (unit * mood.gateLength));
                    n.velocity = phraseVelocity (mood, (double) n.startTick / (chordTicks*4.0), arpRng);
                    n.source = NoteSource::melody;
                    out.push_back (n);
                    ++idx;
                }
            }
        }

        return out;
    }
}
