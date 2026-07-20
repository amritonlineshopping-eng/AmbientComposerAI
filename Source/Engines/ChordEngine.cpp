// =============================================================================
//  ChordEngine.cpp
//
//  Pipeline (see spec §6):
//    1. pick a degree-based progression template (weighted by mood)
//    2. vary it: rotation, diatonic-third substitution, borrowed chords
//    3. colour with ambient extensions (maj7/min7/9/add9/sus/6/6-9)
//    4. nearest-note voice leading + width + octave doubling + register
//    5. realize into held pad notes with sustain/overlap
//
//  Templates are SCALE DEGREES (0-6), so the chord quality is always the
//  diatonic quality of the current scale — harmony can never fall out of key
//  except where a borrowed chord is deliberately (and flag-ably) introduced.
// =============================================================================
#include "ChordEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

namespace acai
{
    using theory::ChordSpec;
    using theory::ChordExtension;
    using theory::ScaleType;

    namespace
    {
        struct Template { std::array<int,4> degrees; double minorAffinity; };

        // Degree-based ambient progression library. minorAffinity biases
        // selection: 1.0 = feels best in minor, 0.0 = feels best in major.
        const std::vector<Template>& templates()
        {
            static const std::vector<Template> t = {
                { { 0, 4, 5, 3 }, 0.3 },   // I–V–vi–IV  (i–v–VI–IV in minor)
                { { 5, 3, 0, 4 }, 0.4 },   // vi–IV–I–V
                { { 0, 2, 5, 3 }, 0.35 },  // I–iii–vi–IV
                { { 1, 4, 0, 5 }, 0.3 },   // ii–V–I–vi
                { { 0, 5, 2, 6 }, 0.85 },  // i–VI–III–VII
                { { 0, 6, 5, 6 }, 0.9 },   // i–VII–VI–VII
                { { 5, 0, 4, 3 }, 0.35 },  // vi–I–V–IV
                { { 0, 3, 5, 4 }, 0.75 },  // i–iv–VI–V
                { { 0, 2, 6, 5 }, 0.8 },   // i–III–VII–VI
                { { 0, 5, 6, 0 }, 0.7 },   // i–VI–VII–i (loop-y, ambient)
                { { 3, 0, 4, 5 }, 0.5 },   // IV–I–V–vi
                { { 0, 4, 3, 0 }, 0.6 },   // i–v–iv–i
            };
            return t;
        }

        // Which extension a chord receives, honouring the mood's extensionUsage
        // and whether the chord is major/minor-quality in this scale.
        ChordExtension pickExtension (bool minorQuality, double extensionUsage,
                                      bool isTonicOrFinal, RandomEngine& rng)
        {
            if (! rng.chance (extensionUsage))
                return ChordExtension::None;

            // Ambient-friendly weights. Order:
            //   Seventh, Ninth, Add9, Sus2, Sus4, Sixth, SixNine
            std::vector<double> w;
            if (minorQuality)
                w = { 3.0, 2.0, 2.5, 0.6, 0.5, 0.8, 0.4 };   // min7/min9/add9 favoured
            else
                w = { 2.5, 1.5, 2.5, 1.0, 0.6, 1.5, 1.2 };   // maj7/add9/6/6-9 favoured

            // Tonic / final chords love add9 and 6/9 for a settled, open sound.
            if (isTonicOrFinal)
            {
                w[2] += 1.5;   // add9
                w[6] += 1.0;   // 6/9
            }

            const int pick = rng.pickWeighted (w);
            static const ChordExtension map[] = {
                ChordExtension::Seventh, ChordExtension::Ninth, ChordExtension::Add9,
                ChordExtension::Sus2,    ChordExtension::Sus4,  ChordExtension::Sixth,
                ChordExtension::SixNine
            };
            return map[std::clamp (pick, 0, 6)];
        }

        // Nearest MIDI note >= floor with the given pitch class.
        int nearestPcAtOrAbove (int floorMidi, int pc)
        {
            pc = ((pc % 12) + 12) % 12;
            int n = floorMidi + ((pc - floorMidi) % 12 + 12) % 12;
            return n;
        }

        // Nearest MIDI note to `target` with the given pitch class.
        int nearestPcTo (int target, int pc)
        {
            pc = ((pc % 12) + 12) % 12;
            const int base = target - (((target % 12) - pc + 12) % 12);
            const int cand1 = base;            // <= target
            const int cand2 = base + 12;       // >= target
            return (std::abs (cand1 - target) <= std::abs (cand2 - target)) ? cand1 : cand2;
        }

        // Build the initial (chord 0) voicing as an ascending stack, then apply
        // voicing width. Returns sorted MIDI notes.
        std::vector<int> initialVoicing (const ChordSpec& spec, int rootTargetMidi, int width)
        {
            std::vector<int> voices;
            int root = nearestPcTo (rootTargetMidi, spec.rootPc);
            voices.push_back (root);
            int prev = root;
            for (size_t i = 1; i < spec.semitonesFromRoot.size(); ++i)
            {
                const int pc = (spec.rootPc + spec.semitonesFromRoot[i]) % 12;
                int n = nearestPcAtOrAbove (prev + 1, pc);
                voices.push_back (n);
                prev = n;
            }

            // Width shaping.
            if (width == 1 && voices.size() >= 3)          // Open: drop 2nd from top an octave
            {
                voices[voices.size() - 2] -= 12;
            }
            else if (width == 2 && voices.size() >= 3)      // Spread: fan voices apart
            {
                // Drop every other inner voice to widen the total span.
                for (size_t i = 1; i + 1 < voices.size(); i += 2)
                    voices[i] -= 12;
            }
            std::sort (voices.begin(), voices.end());
            return voices;
        }

        // Nearest-note voice leading: map each previous voice to the closest
        // tone of the new chord, then guarantee every chord tone is present.
        std::vector<int> voiceLeadFrom (const std::vector<int>& prev,
                                        const ChordSpec& spec)
        {
            const auto pcs = spec.pitchClasses();
            std::vector<int> result;
            result.reserve (prev.size() + pcs.size());

            for (int v : prev)
            {
                // nearest midi to v among the chord's pitch classes
                int best = v; int bestDist = 1 << 30;
                for (int pc : pcs)
                {
                    const int cand = nearestPcTo (v, pc);
                    const int d = std::abs (cand - v);
                    if (d < bestDist) { bestDist = d; best = cand; }
                }
                result.push_back (best);
            }

            // Ensure each chord tone appears at least once (so 7ths/9ths sound).
            for (int pc : pcs)
            {
                const bool present = std::any_of (result.begin(), result.end(),
                    [pc] (int n) { return ((n % 12) + 12) % 12 == pc; });
                if (! present)
                {
                    // place near the voicing centroid
                    int centroid = 0;
                    for (int n : result) centroid += n;
                    centroid = result.empty() ? 60 : centroid / (int) result.size();
                    result.push_back (nearestPcTo (centroid, pc));
                }
            }

            std::sort (result.begin(), result.end());
            result.erase (std::unique (result.begin(), result.end()), result.end());
            return result;
        }
    }

    ChordResult ChordEngine::generate (const MusicalContext& ctx,
                                       const MoodProfile& mood,
                                       const ChordEngineSettings& settings,
                                       RandomEngine& rng)
    {
        ChordResult result;
        const ScaleType cScale = theory::chordScaleFor (ctx.scale);

        // -- 1. pick a template, weighted by how minor the mood is -------------
        const double minorness = 1.0 - mood.majorBias;
        std::vector<double> tWeights;
        for (const auto& t : templates())
        {
            // weight high when the template's affinity matches the mood
            const double match = 1.0 - std::abs (t.minorAffinity - minorness);
            tWeights.push_back (0.15 + match * match);   // keep a floor so all remain possible
        }
        const Template tmpl = templates()[(size_t) rng.pickWeighted (tWeights)];

        // Optional rotation for extra variety (keeps the same harmonic set).
        std::array<int,4> degrees = tmpl.degrees;
        if (rng.chance (0.35))
        {
            const int shift = rng.nextInt (0, 3);
            std::rotate (degrees.begin(), degrees.begin() + shift, degrees.end());
        }

        // -- 2. build the 4 chords, with substitutions & borrowed chords -------
        std::vector<ChordSpec> specs;
        specs.reserve (4);
        for (int i = 0; i < 4; ++i)
        {
            int degree = degrees[(size_t) i];

            // Diatonic-third substitution (shares 2 tones, still in key).
            if (i != 0 && i != 3 && rng.chance (0.18))
                degree = (degree + (rng.chance (0.5) ? 2 : 5)) % 7;

            ChordSpec spec = theory::diatonicTriad (ctx.keyRootPc, cScale, degree);

            // Borrowed chord: occasionally recolour a non-tonic chord with a
            // flat-degree major (bVII / bVI) — a hallmark of the ambient sound.
            const bool canBorrow = (i == 1 || i == 2 || i == 3);
            if (canBorrow && rng.chance (mood.borrowedChordProb))
            {
                const char* borrowRoman = rng.chance (0.5) ? "bVII" : "bVI";
                ChordSpec borrowed = theory::resolveRoman (borrowRoman, ctx.keyRootPc, cScale);
                spec = borrowed;
            }

            specs.push_back (spec);
        }

        // Picardy-style major tonic on the final chord (in minor moods, rarely).
        if (mood.majorBias < 0.4 && rng.chance (0.12))
        {
            ChordSpec picardy = theory::resolveRoman ("I", ctx.keyRootPc, cScale);
            specs[3] = picardy;
        }

        // -- 3. extensions -----------------------------------------------------
        for (int i = 0; i < 4; ++i)
        {
            const bool isTonicOrFinal = (i == 0 || i == 3);
            const ChordExtension ext = pickExtension (specs[i].minorQuality,
                                                      mood.extensionUsage,
                                                      isTonicOrFinal, rng);
            if (ext != ChordExtension::None && ! specs[i].flattened)
                theory::applyExtension (specs[i], ctx.keyRootPc, cScale, ext);
        }

        // -- 4. voice leading + register + width + doubling --------------------
        const int rootTargetMidi = 12 * (settings.chordOctave + 1) + ctx.keyRootPc;
        const int barTicks = settings.timeSig.barLengthTicks();
        const int chordTicks = barTicks * settings.barsPerChord;

        std::vector<int> prevVoicing;
        for (int i = 0; i < 4; ++i)
        {
            VoicedChord vc;
            vc.spec = specs[i];
            vc.startTick = i * chordTicks;
            vc.lengthTick = chordTicks;

            if (prevVoicing.empty())
                vc.pitches = initialVoicing (specs[i], rootTargetMidi, settings.voicingWidth);
            else
                vc.pitches = voiceLeadFrom (prevVoicing, specs[i]);

            // Keep the pad in a sensible register: if voice leading has drifted,
            // shift the whole voicing by octaves back toward the target.
            if (! vc.pitches.empty())
            {
                int lo = *std::min_element (vc.pitches.begin(), vc.pitches.end());
                while (lo < rootTargetMidi - 7)      { for (auto& n : vc.pitches) n += 12; lo += 12; }
                while (lo > rootTargetMidi + 12)     { for (auto& n : vc.pitches) n -= 12; lo -= 12; }
            }

            // Octave doubling: add the lowest voice an octave down for depth.
            if (settings.octaveDoubling && ! vc.pitches.empty())
            {
                const int low = *std::min_element (vc.pitches.begin(), vc.pitches.end());
                vc.pitches.insert (vc.pitches.begin(), theory::clampMidi (low - 12));
            }

            for (auto& n : vc.pitches) n = theory::clampMidi (n);
            std::sort (vc.pitches.begin(), vc.pitches.end());
            vc.pitches.erase (std::unique (vc.pitches.begin(), vc.pitches.end()), vc.pitches.end());

            prevVoicing = vc.pitches;
            // Doubling voice shouldn't drag the *next* chord's lead voice down,
            // so lead the next chord from the non-doubled set.
            if (settings.octaveDoubling && prevVoicing.size() > 1)
                prevVoicing.erase (prevVoicing.begin());

            result.chords.push_back (std::move (vc));
        }

        // -- 5. realize into held pad notes with sustain/overlap ---------------
        result.lengthTicks = chordTicks * 4;
        const int overlapTicks = (int) (settings.sustainOverlap * chordTicks * 0.75);

        // Gentle velocity per chord (pads sit under the melody).
        const int padVel = std::clamp (mood.velocityBase - 6, 30, 110);

        for (size_t i = 0; i < result.chords.size(); ++i)
        {
            const auto& vc = result.chords[i];
            const bool isLast = (i + 1 == result.chords.size());
            int len = vc.lengthTick + overlapTicks;
            // The final chord may ring to the loop end but not far beyond, so
            // looping stays clean.
            if (isLast) len = vc.lengthTick + std::min (overlapTicks, chordTicks / 2);

            for (size_t v = 0; v < vc.pitches.size(); ++v)
            {
                Note n;
                n.pitch = vc.pitches[v];
                n.startTick = vc.startTick;
                n.lengthTick = len;
                // slightly softer on the doubled bass + top for balance
                n.velocity = std::clamp (padVel - (v == 0 ? 4 : 0), 25, 120);
                n.source = NoteSource::chord;
                result.notes.push_back (n);
            }
        }

        return result;
    }
}
