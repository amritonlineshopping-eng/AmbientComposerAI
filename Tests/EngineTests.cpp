// =============================================================================
//  EngineTests.cpp
//  Headless verification of the generation core — no GUI, no audio device.
//  Exercises every acceptance criterion that can be checked offline:
//   determinism, per-part reproducibility, session uniqueness, scale/chord
//   conformance, clash-freedom, leap caps, mood differentiation, generation
//   speed (<100ms), and MIDI-file validity (paired notes, tempo, named tracks).
// =============================================================================
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../Source/Engines/PatternGenerator.h"
#include "../Source/Engines/MoodEngine.h"
#include "../Source/Engines/ScaleEngine.h"
#include "../Source/IO/MidiExporter.h"
#include "../Source/Playback/PlaybackSequence.h"

using namespace acai;

namespace
{
    int gFailures = 0;
    int gChecks   = 0;

    void check (bool cond, const std::string& what)
    {
        ++gChecks;
        if (! cond)
        {
            ++gFailures;
            std::printf ("  [FAIL] %s\n", what.c_str());
        }
    }

    GenerationSettings baseSettings (int mood = 8)
    {
        GenerationSettings s;
        s.keyChoice = 9;      // A
        s.scaleChoice = 0;    // Auto
        s.moodIndex = mood;
        s.bpm = 72;
        s.barsPerChord = 2;
        s.timeSigIndex = 0;
        s.chordOctave = 3;
        s.voicingWidth = 1;
        s.octaveDoubling = true;
        s.sustainOverlap = 0.4;
        s.melodyStyle = 0;
        s.rhythmStyle = 6;
        s.complexity = 0.35;
        s.melodyOctave = 5;
        s.velHumanize = 0.3;
        s.timeHumanize = 0.2;
        s.gateHumanize = 0.2;
        return s;
    }

    // -- determinism ----------------------------------------------------------
    void testDeterminism()
    {
        std::printf ("[determinism]\n");
        auto s = baseSettings();
        auto seeds = Seeds::fromMaster (123456789ull);
        auto a = generatePattern (s, seeds, GenerateTarget::Both, nullptr);
        auto b = generatePattern (s, seeds, GenerateTarget::Both, nullptr);
        check (a.chordFingerprint() == b.chordFingerprint(), "same seed -> identical chords");
        check (a.melodyFingerprint() == b.melodyFingerprint(), "same seed -> identical melody");
        check (a.melodyNotes.size() == b.melodyNotes.size(), "same seed -> same melody note count");

        // A different seed should (almost surely) differ.
        auto c = generatePattern (s, Seeds::fromMaster (987654321ull), GenerateTarget::Both, nullptr);
        check (a.melodyFingerprint() != c.melodyFingerprint()
            || a.chordFingerprint() != c.chordFingerprint(), "different seed -> different music");
    }

    // -- per-part reproducibility --------------------------------------------
    void testPartIndependence()
    {
        std::printf ("[part independence]\n");
        auto s = baseSettings();
        auto seeds = Seeds::fromMaster (42ull);
        auto both = generatePattern (s, seeds, GenerateTarget::Both, nullptr);

        // Regenerate melody, keep chords: chord fingerprint must be unchanged.
        auto seeds2 = seeds; seeds2.melody = Seeds::fromMaster (43ull).melody;
        auto melOnly = generatePattern (s, seeds2, GenerateTarget::MelodyOnly, &both);
        check (melOnly.chordFingerprint() == both.chordFingerprint(),
               "regenerate melody keeps chords identical");
        check (melOnly.melodyFingerprint() != both.melodyFingerprint(),
               "regenerate melody changes the melody");

        // Regenerate chords: new chords, melody re-validated but present.
        auto seeds3 = seeds; seeds3.chord = Seeds::fromMaster (44ull).chord;
        auto chOnly = generatePattern (s, seeds3, GenerateTarget::ChordsOnly, &both);
        check (! chOnly.chordNotes.empty(), "regenerate chords produces chords");
        check (chOnly.chords.size() == 4, "always exactly 4 chords");
    }

    // -- uniqueness across a session -----------------------------------------
    void testUniqueness()
    {
        std::printf ("[uniqueness]\n");
        auto s = baseSettings();
        std::set<uint64_t> chordPrints, melodyPrints;
        int dupChords = 0, dupMel = 0;
        for (uint64_t i = 0; i < 200; ++i)
        {
            auto p = generatePattern (s, Seeds::fromMaster (1000 + i * 2654435761ull),
                                      GenerateTarget::Both, nullptr);
            if (! chordPrints.insert (p.chordFingerprint()).second) ++dupChords;
            if (! melodyPrints.insert (p.melodyFingerprint()).second) ++dupMel;
        }
        // With random seeds a handful of chord-progression collisions is fine
        // (finite template set); melodies should be essentially all unique.
        std::printf ("  chord dups=%d/200  melody dups=%d/200\n", dupChords, dupMel);
        check (dupMel <= 2, "melodies effectively unique across 200 generations");
    }

    // -- scale / chord conformance & clash-freedom ---------------------------
    // The guaranteed invariant (spec §11): every melody note is either in the
    // scale, OR a tone of the currently sounding chord (this covers deliberate
    // BORROWED chords, whose tones are consonant but outside the parent scale),
    // OR a deliberate, flagged chromatic passing tone.
    void testConformance()
    {
        std::printf ("[conformance]\n");
        int totalNonConforming = 0, totalHarshClash = 0, totalBigLeaps = 0, totalNotes = 0;

        auto soundingChord = [] (const GeneratedPattern& p, int tick) -> const VoicedChord*
        {
            const VoicedChord* found = p.chords.empty() ? nullptr : &p.chords.back();
            for (const auto& c : p.chords)
                if (tick >= c.startTick && tick < c.startTick + c.lengthTick) found = &c;
            return found;
        };

        for (int mood = 0; mood < (int) Mood::Count; ++mood)
        {
            auto s = baseSettings (mood);
            for (int scaleChoice = 0; scaleChoice <= 12; ++scaleChoice)
            {
                s.scaleChoice = scaleChoice;
                for (uint64_t seed = 0; seed < 8; ++seed)
                {
                    auto p = generatePattern (s, Seeds::fromMaster (seed * 7 + mood),
                                              GenerateTarget::Both, nullptr);

                    int prev = p.melodyNotes.empty() ? 0 : p.melodyNotes.front().pitch;
                    for (const auto& n : p.melodyNotes)
                    {
                        ++totalNotes;
                        bool conforms = n.isChromaticPassing
                                     || theory::isInScale (n.pitch, p.keyRootPc, p.scale);
                        const VoicedChord* c = soundingChord (p, n.startTick);
                        if (! conforms && c != nullptr)
                        {
                            const auto pcs = c->spec.pitchClasses();
                            const int notePc = ((n.pitch % 12) + 12) % 12;
                            conforms = std::find (pcs.begin(), pcs.end(), notePc) != pcs.end();
                        }
                        if (! conforms) ++totalNonConforming;

                        // Genuine harsh clash: an ACCENTED or SUSTAINED non-chord
                        // note a min-2nd/min-9th from a sounding chord tone.
                        // (Short weak-beat passing dissonance is allowed.)
                        if (c != nullptr && ! n.isChromaticPassing)
                        {
                            const auto pcs = c->spec.pitchClasses();
                            const int notePc = ((n.pitch % 12) + 12) % 12;
                            const bool isChordTone =
                                std::find (pcs.begin(), pcs.end(), notePc) != pcs.end();
                            const int beat = p.timeSig.strongBeatEveryTicks();
                            const bool accented = beat > 0 && (n.startTick % beat) < (kPPQ / 8);
                            const bool sustained = n.lengthTick >= beat;
                            if (! isChordTone && (accented || sustained))
                                for (int cp : c->pitches)
                                    if (std::abs (n.pitch - cp) == 1 || std::abs (n.pitch - cp) == 13)
                                        { ++totalHarshClash; break; }
                        }

                        if (std::abs (n.pitch - prev) > 13) ++totalBigLeaps;
                        prev = n.pitch;
                    }
                }
            }
        }
        std::printf ("  notes=%d nonConforming=%d harshClash=%d bigLeaps=%d\n",
                     totalNotes, totalNonConforming, totalHarshClash, totalBigLeaps);
        check (totalNonConforming == 0, "every melody note is in-scale or a sounding-chord tone");
        check (totalHarshClash == 0, "no accented/sustained clashes against the chord");
        check (totalBigLeaps == 0, "no leaps beyond the musical cap");
        check (totalNotes > 0, "melodies actually contain notes");
    }

    // -- mood differentiation -------------------------------------------------
    void testMoodDifferentiation()
    {
        std::printf ("[mood differentiation]\n");
        // Average velocity and note density should clearly differ between a
        // loud/busy mood (Bright) and a soft/sparse one (Lonely).
        auto measure = [] (int mood)
        {
            double vel = 0; int notes = 0;
            for (uint64_t seed = 0; seed < 20; ++seed)
            {
                auto s = baseSettings (mood);
                s.scaleChoice = 0;
                auto p = generatePattern (s, Seeds::fromMaster (seed + mood * 99),
                                          GenerateTarget::Both, nullptr);
                for (const auto& n : p.melodyNotes) { vel += n.velocity; ++notes; }
            }
            return std::pair<double,double> { notes ? vel / notes : 0.0, notes / 20.0 };
        };
        auto [brightVel, brightDensity] = measure ((int) Mood::Bright);
        auto [lonelyVel, lonelyDensity] = measure ((int) Mood::Lonely);
        std::printf ("  Bright: vel=%.1f density=%.1f | Lonely: vel=%.1f density=%.1f\n",
                     brightVel, brightDensity, lonelyVel, lonelyDensity);
        check (brightVel > lonelyVel + 8.0, "Bright is louder than Lonely");
        check (brightDensity > lonelyDensity, "Bright is busier than Lonely");
    }

    // -- complexity effect ----------------------------------------------------
    void testComplexity()
    {
        std::printf ("[complexity]\n");
        auto density = [] (double complexity)
        {
            int notes = 0;
            for (uint64_t seed = 0; seed < 20; ++seed)
            {
                auto s = baseSettings ((int) Mood::Emotional);
                s.rhythmStyle = 5;   // Mixed responds to complexity
                s.complexity = complexity;
                auto p = generatePattern (s, Seeds::fromMaster (seed), GenerateTarget::Both, nullptr);
                notes += (int) p.melodyNotes.size();
            }
            return notes / 20.0;
        };
        const double low = density (0.0), high = density (1.0);
        std::printf ("  density @0%%=%.1f  @100%%=%.1f\n", low, high);
        check (high > low, "higher complexity yields busier melodies");
    }

    // -- generation speed -----------------------------------------------------
    void testSpeed()
    {
        std::printf ("[speed]\n");
        auto s = baseSettings();
        double worstMs = 0;
        for (uint64_t i = 0; i < 100; ++i)
        {
            auto t0 = std::chrono::high_resolution_clock::now();
            auto p = generatePattern (s, Seeds::fromMaster (i), GenerateTarget::Both, nullptr);
            auto t1 = std::chrono::high_resolution_clock::now();
            const double ms = std::chrono::duration<double, std::milli> (t1 - t0).count();
            worstMs = std::max (worstMs, ms);
            (void) p;
        }
        std::printf ("  worst=%.2f ms\n", worstMs);
        check (worstMs < 100.0, "generation completes < 100 ms");
    }

    // -- MIDI validity --------------------------------------------------------
    void testMidiExport()
    {
        std::printf ("[midi export]\n");
        auto s = baseSettings();
        auto p = generatePattern (s, Seeds::fromMaster (7), GenerateTarget::Both, nullptr);

        auto midi = MidiExporter::buildMidiFile (p, ExportMode::Combined, 1);
        check (midi.getNumTracks() == 2, "combined export has 2 tracks");
        check (midi.getTimeFormat() == kPPQ, "ticks-per-quarter is 960");

        // every note-on matches a note-off; find tempo + track-name metas
        bool foundTempo = false, foundName = false;
        int hangingNotes = 0;
        for (int t = 0; t < midi.getNumTracks(); ++t)
        {
            const auto* seq = midi.getTrack (t);
            std::map<int,int> open;
            int ons = 0, offs = 0;
            for (int e = 0; e < seq->getNumEvents(); ++e)
            {
                const auto& m = seq->getEventPointer (e)->message;
                if (m.isTempoMetaEvent()) foundTempo = true;
                if (m.isTrackNameEvent()) foundName = true;
                if (m.isNoteOn())  { open[m.getNoteNumber()]++; ++ons; }
                if (m.isNoteOff()) { open[m.getNoteNumber()]--; ++offs; }
            }
            for (auto& [note, cnt] : open) if (cnt != 0) ++hangingNotes;
            juce::ignoreUnused (ons, offs);
        }
        check (foundTempo, "tempo meta present");
        check (foundName, "track-name meta present");
        check (hangingNotes == 0, "every note-on has a matching note-off");

        // Persistent sample for independent (non-JUCE) validation.
        if (const char* out = std::getenv ("ACAI_MIDI_OUT"))
        {
            juce::File keep { juce::String (out) };
            MidiExporter::writeToFile (p, ExportMode::Combined, 1, keep);
            std::printf ("  wrote sample MIDI -> %s\n", keep.getFullPathName().toRawUTF8());
        }

        // round-trip: write and re-read
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("acai_test.mid");
        check (MidiExporter::writeToFile (p, ExportMode::Combined, 1, tmp), "writes a .mid file");
        juce::MidiFile reread;
        if (auto in = tmp.createInputStream())
            check (reread.readFrom (*in) && reread.getNumTracks() >= 2, "re-reads as valid Type-1 MIDI");
        tmp.deleteFile();
    }

    // -- pattern basics -------------------------------------------------------
    void testPatternBasics()
    {
        std::printf ("[pattern basics]\n");
        auto s = baseSettings();
        auto p = generatePattern (s, Seeds::fromMaster (5), GenerateTarget::Both, nullptr);
        check (p.chords.size() == 4, "exactly 4 chords");
        check (! p.chordNotes.empty(), "chords produce notes");
        check (! p.melodyNotes.empty(), "melody produces notes");
        check (p.lengthTicks == p.timeSig.barLengthTicks() * 2 * 4, "8-bar loop at 2 bars/chord");

        // no negative-time or zero-length notes anywhere (humanizer safety)
        bool timesOk = true;
        for (const auto* list : { &p.chordNotes, &p.melodyNotes })
            for (const auto& n : *list)
                if (n.startTick < 0 || n.lengthTick <= 0) timesOk = false;
        check (timesOk, "no negative-time or zero-length notes");

        std::printf ("  progression: %s\n", p.progressionText().c_str());
        std::printf ("  key=%d scale=%s melody notes=%zu\n",
                     p.keyRootPc, theory::scaleName (p.scale).c_str(), p.melodyNotes.size());
    }

    // -- validate a PlaybackSequence (the preview-playback data) --------------
    bool playbackSequenceValid (const GeneratedPattern& p)
    {
        auto seq = PlaybackSequence::build (p);
        if (! seq) return false;

        // sorted by tick; offs before ons at equal ticks; within loop bounds
        int lastTick = -1; bool lastWasOn = false;
        for (const auto& e : seq->events)
        {
            if (e.tick < 0 || e.tick > seq->lengthTicks) return false;
            if (e.tick < lastTick) return false;
            if (e.tick == lastTick && ! e.isNoteOn && lastWasOn) return false; // off after on at same tick
            lastTick = e.tick; lastWasOn = e.isNoteOn;
        }
        // every note balanced (no hanging note in the preview stream)
        std::map<int,int> open;
        for (const auto& e : seq->events)
            open[e.pitch] += (e.isNoteOn ? 1 : -1);
        for (auto& [pitch, cnt] : open)
            if (cnt != 0) return false;
        return true;
    }

    // -- stress: every style / rhythm / time-sig / extreme, no crash / valid --
    void testStress()
    {
        std::printf ("[stress]\n");
        int combos = 0, badNotes = 0, badSeq = 0, empty = 0;
        const double complexities[] = { 0.0, 0.5, 1.0 };

        for (int melStyle = 0; melStyle < 5; ++melStyle)
        for (int rhythm = 0; rhythm < 7; ++rhythm)
        for (int tsig = 0; tsig < 3; ++tsig)
        for (int bars = 0; bars < 2; ++bars)
        for (double cx : complexities)
        {
            ++combos;
            auto s = baseSettings ((combos * 7) % (int) Mood::Count);
            s.scaleChoice = (combos % 12) + 1;   // exercise every explicit scale
            s.melodyStyle = melStyle;
            s.rhythmStyle = rhythm;
            s.timeSigIndex = tsig;
            s.barsPerChord = bars;               // choice index 0->1 bar, 1->2 bars decoded in readSettings; here raw
            // barsPerChord in GenerationSettings is already the decoded count:
            s.barsPerChord = (bars == 0) ? 1 : 2;
            s.complexity = cx;
            // push humanize to the max to stress the safety clamps
            s.velHumanize = 1.0; s.timeHumanize = 1.0; s.gateHumanize = 1.0;

            auto p = generatePattern (s, Seeds::fromMaster ((uint64_t) combos * 2654435761ull),
                                      GenerateTarget::Both, nullptr);

            if (p.chordNotes.empty() && p.melodyNotes.empty()) ++empty;

            bool notesOk = true;
            for (const auto* list : { &p.chordNotes, &p.melodyNotes })
                for (const auto& n : *list)
                    if (n.startTick < 0 || n.lengthTick <= 0 || n.pitch < 0 || n.pitch > 127
                        || n.velocity < 1 || n.velocity > 127)
                        notesOk = false;
            if (! notesOk) ++badNotes;

            if (! playbackSequenceValid (p)) ++badSeq;

            // MIDI export must stay valid for every combo too
            auto midi = MidiExporter::buildMidiFile (p, ExportMode::Combined, (combos % 16) + 1);
            for (int t = 0; t < midi.getNumTracks(); ++t)
            {
                std::map<int,int> mopen;
                const auto* mseq = midi.getTrack (t);
                for (int e = 0; e < mseq->getNumEvents(); ++e)
                {
                    const auto& m = mseq->getEventPointer (e)->message;
                    if (m.isNoteOn())  mopen[m.getNoteNumber()]++;
                    if (m.isNoteOff()) mopen[m.getNoteNumber()]--;
                }
                for (auto& [note, cnt] : mopen) if (cnt != 0) ++badSeq;
            }
        }
        std::printf ("  %d combos: emptyMelody=%d badNotes=%d badSeq/MIDI=%d\n",
                     combos, empty, badNotes, badSeq);
        check (badNotes == 0, "no invalid notes across all style/rhythm/timesig/extreme combos");
        check (badSeq == 0, "playback sequence + MIDI valid (no hangs) across all combos");
        check (combos == 5 * 7 * 3 * 2 * 3, "stress matrix fully covered");
    }

    // -- music-theory spot checks (regression guards for review findings) ----
    void testTheory()
    {
        using namespace acai::theory;
        std::printf ("[theory]\n");

        // Borrowed chords bVII / bVI must resolve to the parallel-major-derived
        // roots (b7 = tonic+10, b6 = tonic+8) in EVERY key and scale — including
        // minor modes whose own 6th/7th are already lowered.
        int badBorrow = 0;
        for (int key = 0; key < 12; ++key)
            for (ScaleType sc : { ScaleType::Major, ScaleType::Aeolian, ScaleType::Dorian,
                                  ScaleType::Phrygian, ScaleType::HarmonicMinor })
            {
                auto bVII = resolveRoman ("bVII", key, sc);
                auto bVI  = resolveRoman ("bVI",  key, sc);
                if (bVII.rootPc != (key + 10) % 12) ++badBorrow;
                if (bVI.rootPc  != (key + 8)  % 12) ++badBorrow;
            }
        check (badBorrow == 0, "bVII/bVI borrowed roots correct in every key & scale");

        // Concrete: bVII in C minor must be Bb major (not A major).
        auto cMinBVII = resolveRoman ("bVII", 0, ScaleType::Aeolian);
        check (cMinBVII.rootPc == 10, "bVII in C minor = Bb (pc 10)");
        auto pcs = cMinBVII.pitchClasses();   // Bb, D, F = {10, 2, 5}
        const bool hasBb = std::find (pcs.begin(), pcs.end(), 10) != pcs.end();
        const bool hasD  = std::find (pcs.begin(), pcs.end(), 2)  != pcs.end();
        const bool hasF  = std::find (pcs.begin(), pcs.end(), 5)  != pcs.end();
        check (hasBb && hasD && hasF, "bVII in C minor spells Bb-D-F");

        // Seventh-chord labelling: a dominant (major triad + minor 7th) is "7",
        // a tonic (major triad + major 7th) is "maj7".
        auto V7 = diatonicTriad (0, ScaleType::Major, 4);      // G major
        applyExtension (V7, 0, ScaleType::Major, ChordExtension::Seventh);
        check (V7.symbol == "G7", "V7 in C major is labelled G7 (dominant), not Gmaj7");

        auto Imaj7 = diatonicTriad (0, ScaleType::Major, 0);   // C major
        applyExtension (Imaj7, 0, ScaleType::Major, ChordExtension::Seventh);
        check (Imaj7.symbol == "Cmaj7", "I7 in C major is labelled Cmaj7");

        std::printf ("  bVII(Cmin)=%s  V7(C)=%s  Imaj7(C)=%s\n",
                     cMinBVII.symbol.c_str(), V7.symbol.c_str(), Imaj7.symbol.c_str());
    }

    // -- seed string round-trip ----------------------------------------------
    void testSeedParsing()
    {
        std::printf ("[seed parsing]\n");
        auto seeds = Seeds::fromMaster (0xDEADBEEFull);
        auto str = seeds.toString();
        auto parsed = Seeds::parse (str);
        check (parsed.has_value(), "seed string parses");
        check (parsed && *parsed == seeds, "seed round-trips through its string");
        check (Seeds::parse ("12345").has_value(), "decimal master seed parses");
        check (Seeds::parse ("0xABCD").has_value(), "hex master seed parses");
        check (! Seeds::parse ("not a seed!!").has_value(), "garbage rejected");
    }
}

int main()
{
    std::printf ("=== Ambient Composer AI — engine tests ===\n");
    testPatternBasics();
    testDeterminism();
    testPartIndependence();
    testUniqueness();
    testConformance();
    testMoodDifferentiation();
    testComplexity();
    testSpeed();
    testMidiExport();
    testStress();
    testTheory();
    testSeedParsing();

    std::printf ("\n%d/%d checks passed, %d failed\n",
                 gChecks - gFailures, gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
