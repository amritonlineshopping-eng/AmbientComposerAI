// =============================================================================
//  PatternGenerator.cpp — the single generation entry point (spec §16 dataflow).
// =============================================================================
#include "PatternGenerator.h"

#include "ChordEngine.h"
#include "CompositionValidator.h"
#include "Humanizer.h"
#include "MelodyEngine.h"
#include "MoodEngine.h"
#include "ScaleEngine.h"

namespace acai
{
    TimeSignature timeSignatureForIndex (int timeSigIndex)
    {
        switch (timeSigIndex)
        {
            case 1:  return { 3, 4 };
            case 2:  return { 6, 8 };
            default: return { 4, 4 };
        }
    }

    namespace
    {
        Mood moodFromIndex (int idx)
        {
            if (idx < 0 || idx >= (int) Mood::Count) idx = 0;
            return (Mood) idx;
        }

        ChordEngineSettings chordSettings (const GenerationSettings& s, const TimeSignature& ts)
        {
            ChordEngineSettings c;
            c.chordOctave    = s.chordOctave;
            c.voicingWidth   = s.voicingWidth;
            c.octaveDoubling = s.octaveDoubling;
            c.sustainOverlap = s.sustainOverlap;
            c.barsPerChord   = s.barsPerChord;
            c.timeSig        = ts;
            return c;
        }

        MelodyEngineSettings melodySettings (const GenerationSettings& s, const TimeSignature& ts)
        {
            MelodyEngineSettings m;
            m.style        = (MelodyStyle) s.melodyStyle;
            m.rhythmStyle  = (RhythmStyle) s.rhythmStyle;
            m.complexity   = s.complexity;
            m.melodyOctave = s.melodyOctave;
            m.barsPerChord = s.barsPerChord;
            m.timeSig      = ts;
            return m;
        }
    }

    GeneratedPattern generatePattern (const GenerationSettings& settings,
                                      Seeds seeds,
                                      GenerateTarget target,
                                      const GeneratedPattern* previous)
    {
        GeneratedPattern pattern;
        pattern.seeds        = seeds;
        pattern.moodIndex    = settings.moodIndex;
        pattern.bpm          = settings.bpm;
        pattern.barsPerChord = settings.barsPerChord;
        pattern.timeSig      = timeSignatureForIndex (settings.timeSigIndex);

        const Mood mood = moodFromIndex (settings.moodIndex);
        const MoodProfile& moodProfile = moods::profileFor (mood);

        // --- resolve key/scale (deterministic per chord seed) ----------------
        // MelodyOnly keeps the previous chords AND the previous musical context.
        MusicalContext ctx;
        if (target == GenerateTarget::MelodyOnly && previous != nullptr && ! previous->chords.empty())
        {
            ctx.keyRootPc = previous->keyRootPc;
            ctx.scale     = previous->scale;
            ctx.preferFlats = theory::keyPrefersFlats (ctx.keyRootPc,
                                                       theory::isMinorFlavoured (ctx.scale));
            pattern.chords     = previous->chords;
            pattern.chordNotes = previous->chordNotes;
            pattern.lengthTicks = previous->lengthTicks;
        }
        else
        {
            RandomEngine keyRng = RandomEngine (seeds.chord).derive (0x5CA1E);
            ctx = scaleEngine::resolve (settings.keyChoice, settings.scaleChoice, mood, keyRng);

            // --- chords ------------------------------------------------------
            RandomEngine chordRng (seeds.chord);
            const auto chordResult = ChordEngine::generate (ctx, moodProfile,
                                                            chordSettings (settings, pattern.timeSig),
                                                            chordRng);
            pattern.chords      = chordResult.chords;
            pattern.chordNotes  = chordResult.notes;
            pattern.lengthTicks = chordResult.lengthTicks;
        }

        pattern.keyRootPc = ctx.keyRootPc;
        pattern.scale     = ctx.scale;

        // --- melody ----------------------------------------------------------
        // (Re-roll only guards against a degenerate empty melody; the validator
        //  runs later, after humanisation, per the spec's dataflow.)
        if (target != GenerateTarget::ChordsOnly || previous == nullptr)
        {
            for (int attempt = 0; attempt < 4; ++attempt)
            {
                RandomEngine melodyRng = (attempt == 0)
                    ? RandomEngine (seeds.melody)
                    : RandomEngine (seeds.melody).derive (0xA77 + (uint64_t) attempt);

                pattern.melodyNotes = MelodyEngine::generate (
                    pattern.chords, ctx, moodProfile,
                    melodySettings (settings, pattern.timeSig), melodyRng);

                if (! pattern.melodyNotes.empty()) break;
            }
        }
        else if (previous != nullptr)
        {
            // ChordsOnly: keep the previous melody; the validator re-fits it to
            // the NEW chords below so nothing clashes.
            pattern.melodyNotes = previous->melodyNotes;
        }

        // --- humanize (both tracks; pads gentler than lead) ------------------
        {
            RandomEngine humRng (seeds.humanize);
            HumanizeSettings hs;
            hs.velocityAmount = settings.velHumanize;
            hs.timingAmount   = settings.timeHumanize;
            hs.gateAmount     = settings.gateHumanize;

            // pads: half the timing/gate motion so the bed stays steady
            HumanizeSettings padHs = hs;
            padHs.timingAmount *= 0.4;
            padHs.gateAmount   *= 0.3;

            RandomEngine padRng  = humRng.derive (0xDAD);
            RandomEngine leadRng = humRng.derive (0x1EAD);
            Humanizer::apply (pattern.chordNotes,  padHs,  padRng,  pattern.lengthTicks);
            Humanizer::apply (pattern.melodyNotes, hs,     leadRng, pattern.lengthTicks);
        }

        // --- validate (FINAL pass, after humanisation: spec §16 dataflow) ----
        // Humanisation can nudge notes across chord boundaries, so the validator
        // is the last word on pitch: it removes clashes, caps leaps and enforces
        // the "in-scale OR sounding-chord-tone OR flagged-passing" invariant.
        {
            RandomEngine valRng = RandomEngine (seeds.melody).derive (0x7A11D);
            CompositionValidator::validateAndFix (pattern, ctx, moodProfile, valRng);
        }

        // Final cleanup: resolve same-pitch overlaps so every track is valid
        // MIDI (no dangling note-offs) and the preview never hangs a note.
        // (After the validator, since its pitch fixes can create new overlaps.)
        sanitizeSamePitchOverlaps (pattern.chordNotes);
        sanitizeSamePitchOverlaps (pattern.melodyNotes);

        return pattern;
    }
}
