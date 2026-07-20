// =============================================================================
//  PatternState.cpp — GeneratedPattern <-> ValueTree.
//
//  Notes are stored as compact CSV strings (one per track / chord) to keep the
//  saved state small. Everything needed to reproduce the exact music AND to
//  keep-chords-regenerate-melody after a reload is preserved: seeds, key/scale,
//  mood, tempo, time-signature, the voiced chords and both note tracks.
// =============================================================================
#include "PatternState.h"

namespace acai::patternState
{
    namespace
    {
        juce::String notesToString (const std::vector<Note>& notes)
        {
            juce::String s;
            for (const auto& n : notes)
                s << n.pitch << ',' << n.startTick << ',' << n.lengthTick << ','
                  << n.velocity << ',' << (int) n.source << ','
                  << (n.isChromaticPassing ? 1 : 0) << ';';
            return s;
        }

        std::vector<Note> notesFromString (const juce::String& s)
        {
            std::vector<Note> notes;
            auto records = juce::StringArray::fromTokens (s, ";", "");
            for (const auto& rec : records)
            {
                if (rec.trim().isEmpty()) continue;
                auto f = juce::StringArray::fromTokens (rec, ",", "");
                if (f.size() < 6) continue;
                Note n;
                n.pitch      = f[0].getIntValue();
                n.startTick  = f[1].getIntValue();
                n.lengthTick = f[2].getIntValue();
                n.velocity   = f[3].getIntValue();
                n.source     = (NoteSource) f[4].getIntValue();
                n.isChromaticPassing = f[5].getIntValue() != 0;
                notes.push_back (n);
            }
            return notes;
        }

        juce::String intsToString (const std::vector<int>& v)
        {
            juce::String s;
            for (int x : v) s << x << ',';
            return s;
        }

        std::vector<int> intsFromString (const juce::String& s)
        {
            std::vector<int> v;
            for (const auto& tok : juce::StringArray::fromTokens (s, ",", ""))
                if (tok.trim().isNotEmpty()) v.push_back (tok.getIntValue());
            return v;
        }
    }

    juce::ValueTree toValueTree (const GeneratedPattern& p)
    {
        juce::ValueTree t (kPatternType);
        t.setProperty ("seedChord",    (juce::int64) p.seeds.chord,    nullptr);
        t.setProperty ("seedMelody",   (juce::int64) p.seeds.melody,   nullptr);
        t.setProperty ("seedHumanize", (juce::int64) p.seeds.humanize, nullptr);
        t.setProperty ("keyRootPc",    p.keyRootPc, nullptr);
        t.setProperty ("scale",        (int) p.scale, nullptr);
        t.setProperty ("moodIndex",    p.moodIndex, nullptr);
        t.setProperty ("bpm",          p.bpm, nullptr);
        t.setProperty ("tsNum",        p.timeSig.numerator, nullptr);
        t.setProperty ("tsDen",        p.timeSig.denominator, nullptr);
        t.setProperty ("barsPerChord", p.barsPerChord, nullptr);
        t.setProperty ("lengthTicks",  p.lengthTicks, nullptr);
        t.setProperty ("chordNotes",   notesToString (p.chordNotes), nullptr);
        t.setProperty ("melodyNotes",  notesToString (p.melodyNotes), nullptr);

        juce::ValueTree chords ("CHORDS");
        for (const auto& vc : p.chords)
        {
            juce::ValueTree c ("CHORD");
            c.setProperty ("symbol",     juce::String (vc.spec.symbol), nullptr);
            c.setProperty ("roman",      juce::String (vc.spec.roman), nullptr);
            c.setProperty ("rootPc",     vc.spec.rootPc, nullptr);
            c.setProperty ("degree",     vc.spec.degree, nullptr);
            c.setProperty ("minorQ",     vc.spec.minorQuality ? 1 : 0, nullptr);
            c.setProperty ("flattened",  vc.spec.flattened ? 1 : 0, nullptr);
            c.setProperty ("ext",        (int) vc.spec.extension, nullptr);
            c.setProperty ("semis",      intsToString (vc.spec.semitonesFromRoot), nullptr);
            c.setProperty ("pitches",    intsToString (vc.pitches), nullptr);
            c.setProperty ("startTick",  vc.startTick, nullptr);
            c.setProperty ("lengthTick", vc.lengthTick, nullptr);
            chords.appendChild (c, nullptr);
        }
        t.appendChild (chords, nullptr);
        return t;
    }

    GeneratedPattern fromValueTree (const juce::ValueTree& t)
    {
        GeneratedPattern p;
        if (! t.hasType (kPatternType)) return p;

        p.seeds.chord    = (uint64_t) (juce::int64) t.getProperty ("seedChord",    0);
        p.seeds.melody   = (uint64_t) (juce::int64) t.getProperty ("seedMelody",   0);
        p.seeds.humanize = (uint64_t) (juce::int64) t.getProperty ("seedHumanize", 0);
        p.keyRootPc      = t.getProperty ("keyRootPc", 0);
        p.scale          = (theory::ScaleType) (int) t.getProperty ("scale", (int) theory::ScaleType::Minor);
        p.moodIndex      = t.getProperty ("moodIndex", 0);
        p.bpm            = t.getProperty ("bpm", 72.0);
        p.timeSig.numerator   = t.getProperty ("tsNum", 4);
        p.timeSig.denominator = t.getProperty ("tsDen", 4);
        p.barsPerChord   = t.getProperty ("barsPerChord", 2);
        p.lengthTicks    = t.getProperty ("lengthTicks", 0);
        p.chordNotes     = notesFromString (t.getProperty ("chordNotes", "").toString());
        p.melodyNotes    = notesFromString (t.getProperty ("melodyNotes", "").toString());

        auto chords = t.getChildWithName ("CHORDS");
        for (int i = 0; i < chords.getNumChildren(); ++i)
        {
            auto c = chords.getChild (i);
            VoicedChord vc;
            vc.spec.symbol       = c.getProperty ("symbol", "").toString().toStdString();
            vc.spec.roman        = c.getProperty ("roman", "").toString().toStdString();
            vc.spec.rootPc       = c.getProperty ("rootPc", 0);
            vc.spec.degree       = c.getProperty ("degree", 0);
            vc.spec.minorQuality = (int) c.getProperty ("minorQ", 0) != 0;
            vc.spec.flattened    = (int) c.getProperty ("flattened", 0) != 0;
            vc.spec.extension    = (theory::ChordExtension) (int) c.getProperty ("ext", 0);
            vc.spec.semitonesFromRoot = intsFromString (c.getProperty ("semis", "0,4,7").toString());
            vc.pitches           = intsFromString (c.getProperty ("pitches", "").toString());
            vc.startTick         = c.getProperty ("startTick", 0);
            vc.lengthTick        = c.getProperty ("lengthTick", 0);
            p.chords.push_back (std::move (vc));
        }
        return p;
    }
}
