// =============================================================================
//  MidiExporter.cpp — Type-1 MIDI file construction + temp files for dragging.
// =============================================================================
#include "MidiExporter.h"

#include "../Engines/MoodEngine.h"

namespace acai
{
    namespace
    {
        // Build one named MidiMessageSequence from a note list. `withMeta`
        // prepends tempo + time-signature meta events (only the first written
        // track carries them, per Type-1 convention).
        juce::MidiMessageSequence buildTrack (const std::vector<Note>& notes,
                                              const juce::String& trackName,
                                              int channel, double bpm,
                                              const TimeSignature& ts, bool withMeta)
        {
            juce::MidiMessageSequence seq;

            // Track-name meta (type 0x03) at tick 0.
            {
                auto nameMsg = juce::MidiMessage::textMetaEvent (3, trackName);
                nameMsg.setTimeStamp (0.0);
                seq.addEvent (nameMsg);
            }

            if (withMeta)
            {
                const double usPerQuarter = 60000000.0 / juce::jmax (1.0, bpm);
                auto tempo = juce::MidiMessage::tempoMetaEvent ((int) usPerQuarter);
                tempo.setTimeStamp (0.0);
                seq.addEvent (tempo);

                auto sig = juce::MidiMessage::timeSignatureMetaEvent (ts.numerator, ts.denominator);
                sig.setTimeStamp (0.0);
                seq.addEvent (sig);
            }

            for (const auto& n : notes)
            {
                const int vel = juce::jlimit (1, 127, n.velocity);
                auto on = juce::MidiMessage::noteOn (channel, juce::jlimit (0, 127, n.pitch),
                                                     (juce::uint8) vel);
                on.setTimeStamp ((double) n.startTick);
                seq.addEvent (on);

                auto off = juce::MidiMessage::noteOff (channel, juce::jlimit (0, 127, n.pitch));
                off.setTimeStamp ((double) juce::jmax (n.startTick + 1, n.endTick()));
                seq.addEvent (off);
            }

            seq.updateMatchedPairs();
            seq.sort();
            return seq;
        }
    }

    juce::MidiFile MidiExporter::buildMidiFile (const GeneratedPattern& p,
                                                ExportMode mode, int midiChannel)
    {
        juce::MidiFile file;
        file.setTicksPerQuarterNote (kPPQ);

        const int ch = juce::jlimit (1, 16, midiChannel);

        switch (mode)
        {
            case ExportMode::ChordsOnly:
                file.addTrack (buildTrack (p.chordNotes, "Chords", ch, p.bpm, p.timeSig, true));
                break;
            case ExportMode::MelodyOnly:
                file.addTrack (buildTrack (p.melodyNotes, "Melody", ch, p.bpm, p.timeSig, true));
                break;
            case ExportMode::Combined:
                // First track carries the tempo/timesig meta; both are named.
                file.addTrack (buildTrack (p.chordNotes,  "Chords", ch, p.bpm, p.timeSig, true));
                file.addTrack (buildTrack (p.melodyNotes, "Melody", ch, p.bpm, p.timeSig, false));
                break;
        }
        return file;
    }

    juce::String MidiExporter::suggestedFileName (const GeneratedPattern& p, ExportMode mode)
    {
        const bool minor = theory::isMinorFlavoured (p.scale);
        const bool flats = theory::keyPrefersFlats (p.keyRootPc, minor);
        juce::String keyName (theory::pitchClassName (p.keyRootPc, flats));
        keyName = keyName.replace ("#", "s");   // filesystem-safe

        juce::String moodName (moods::name ((Mood) juce::jlimit (0, (int) Mood::Count - 1, p.moodIndex)));
        juce::String modeName = (mode == ExportMode::ChordsOnly) ? "chords"
                              : (mode == ExportMode::MelodyOnly) ? "melody" : "combined";

        // seed shown as the chord sub-seed hex (recoverable via Paste Seed).
        juce::String seedHex = juce::String::toHexString ((juce::int64) p.seeds.chord).toUpperCase();

        return "AmbientComposer_" + keyName + (minor ? "min" : "maj")
             + "_" + moodName.toLowerCase()
             + "_seed" + seedHex
             + "_" + modeName + ".mid";
    }

    bool MidiExporter::writeToFile (const GeneratedPattern& p, ExportMode mode,
                                    int midiChannel, const juce::File& file)
    {
        auto midi = buildMidiFile (p, mode, midiChannel);
        file.deleteFile();
        if (auto stream = file.createOutputStream())
        {
            const bool ok = midi.writeTo (*stream);
            stream->flush();
            return ok;
        }
        return false;
    }

    juce::File MidiExporter::writeTempFileForDrag (const GeneratedPattern& p,
                                                   ExportMode mode, int midiChannel)
    {
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory)
                       .getChildFile ("AmbientComposerAI");
        dir.createDirectory();

        // Clean stale drag files so the temp folder never accumulates.
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false, "*.mid"))
            if (f.getLastModificationTime() < juce::Time::getCurrentTime() - juce::RelativeTime::hours (1))
                f.deleteFile();

        auto file = dir.getChildFile (suggestedFileName (p, mode));
        if (! writeToFile (p, mode, midiChannel, file))
            return {};
        return file;
    }
}
