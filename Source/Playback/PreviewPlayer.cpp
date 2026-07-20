// =============================================================================
//  PreviewPlayer.cpp — RT-safe preview transport.
//
//  The audio thread (process) never allocates and never blocks: it try-locks
//  to copy the current sequence's shared_ptr (a refcount bump), and if it can't
//  get the lock it simply renders the synth tail for this block. Message-thread
//  actions (play/stop/setSequence) communicate via atomics + a guarded pointer.
// =============================================================================
#include "PreviewPlayer.h"

namespace acai
{
    void PreviewPlayer::prepare (double sampleRate, int maxBlockSize)
    {
        sampleRate_ = sampleRate;
        synth_.prepare (sampleRate);
        midiScratch_.ensureSize (2048);
        juce::ignoreUnused (maxBlockSize);
    }

    void PreviewPlayer::releaseResources()
    {
        stop();
        synth_.panic();
    }

    void PreviewPlayer::setSequence (std::shared_ptr<const PlaybackSequence> seq)
    {
        // Keep this sequence (and the last few) alive on the message thread so
        // the audio thread's transient copy can never be the last owner.
        retainRing_[(size_t) (retainWrite_++ % (int) retainRing_.size())] = seq;

        {
            const juce::SpinLock::ScopedLockType sl (lock_);
            sequence_ = std::move (seq);
        }
        // Release the old notes on the AUDIO thread (not here) to avoid taking
        // the synth's lock cross-thread — process() consumes this flag.
        sequenceSwapped_.store (true);
    }

    void PreviewPlayer::play()
    {
        restartRequested_.store (true);
        playing_.store (true);
    }

    void PreviewPlayer::stop()
    {
        stopRequested_.store (true);
        playing_.store (false);
    }

    void PreviewPlayer::fireEventsInRange (double fromTick, double toTick,
                                           int sampleOffsetBase,
                                           double samplesPerTick,
                                           const PlaybackSequence& seq,
                                           juce::MidiBuffer& midiScratch)
    {
        // Emit every event whose tick is in [fromTick, toTick). The sample
        // offset is measured from `sampleOffsetBase` (non-zero for the part of
        // a block that comes AFTER a loop wrap), so post-wrap notes land at the
        // correct time rather than at the block start.
        for (const auto& ev : seq.events)
        {
            if (ev.tick >= fromTick && ev.tick < toTick)
            {
                int sampleOffset = sampleOffsetBase + (int) ((ev.tick - fromTick) * samplesPerTick);
                sampleOffset = juce::jlimit (0, lastBlockSize_ - 1, sampleOffset);
                if (ev.isNoteOn)
                    midiScratch.addEvent (juce::MidiMessage::noteOn (1, ev.pitch,
                                          (juce::uint8) ev.velocity), sampleOffset);
                else
                    midiScratch.addEvent (juce::MidiMessage::noteOff (1, ev.pitch), sampleOffset);
            }
        }
    }

    void PreviewPlayer::process (juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        lastBlockSize_ = juce::jmax (1, numSamples);
        midiScratch_.clear();

        // Handle transport requests raised on the message thread.
        if (stopRequested_.exchange (false))
        {
            synth_.softStopAll();
            posTicks_ = 0.0;
            playheadTicks_.store (0.0);
        }
        if (restartRequested_.exchange (false))
        {
            synth_.softStopAll();
            posTicks_ = 0.0;
        }
        if (sequenceSwapped_.exchange (false))
        {
            // A new sequence was published: release anything still sounding from
            // the old one, cleanly, here on the audio thread.
            synth_.softStopAll();
        }

        std::shared_ptr<const PlaybackSequence> seq;
        {
            const juce::SpinLock::ScopedTryLockType sl (lock_);
            if (sl.isLocked())
                seq = sequence_;      // refcount bump only
        }

        if (playing_.load() && seq && seq->lengthTicks > 0 && numSamples > 0)
        {
            const double bpm = bpm_.load();
            const double ticksPerSecond = (bpm / 60.0) * (double) kPPQ;
            const double ticksPerSample = ticksPerSecond / sampleRate_;
            const double samplesPerTick = ticksPerSample > 0.0 ? 1.0 / ticksPerSample : 0.0;

            const double startTick = posTicks_;
            double endTick = startTick + ticksPerSample * numSamples;
            const double loopLen = (double) seq->lengthTicks;

            if (endTick < loopLen || ! looping_.load())
            {
                fireEventsInRange (startTick, endTick, 0, samplesPerTick, *seq, midiScratch_);
                posTicks_ = endTick;
                if (! looping_.load() && posTicks_ >= loopLen)
                {
                    // reached the end of a one-shot: stop cleanly
                    synth_.softStopAll();
                    playing_.store (false);
                    posTicks_ = 0.0;
                }
            }
            else
            {
                // wrap: fire up to the loop end, all-notes-off at the wrap
                // sample, then continue from tick 0 with the CORRECT base offset.
                fireEventsInRange (startTick, loopLen, 0, samplesPerTick, *seq, midiScratch_);
                const int wrapOffset = juce::jlimit (0, numSamples - 1,
                                                     (int) ((loopLen - startTick) * samplesPerTick));
                // ensure nothing hangs across the wrap (inserted BEFORE the
                // post-wrap note-ons at the same sample, so tick-0 notes survive)
                for (int p = 0; p < 128; ++p)
                    midiScratch_.addEvent (juce::MidiMessage::noteOff (1, p), wrapOffset);
                const double remainder = endTick - loopLen;
                fireEventsInRange (0.0, remainder, wrapOffset, samplesPerTick, *seq, midiScratch_);
                posTicks_ = remainder;
            }

            playheadTicks_.store (posTicks_);
        }

        // Render the synth (adds to whatever is in the buffer; host bus is
        // otherwise silent).
        synth_.renderNextBlock (buffer, midiScratch_, 0, numSamples);
    }
}
