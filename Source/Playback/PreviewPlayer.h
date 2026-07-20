// =============================================================================
//  PreviewPlayer.h
//  Real-time-safe preview transport. Owns playback position and feeds the
//  PreviewSynth from the current PlaybackSequence inside processBlock.
//
//  Threading contract:
//   * Message thread: setSequence() / play() / stop() / setLooping() / setBpm().
//   * Audio thread:   process() only. Reads the sequence via a SpinLock
//     try-lock + shared_ptr copy (refcount bump only, no allocation).
//   * GUI thread:     playheadTicks() / isPlaying() atomics for the playhead.
//
//  RT-safety of the sequence hand-off:
//   * The message thread keeps the last few sequences alive in `retainRing_`,
//     so when the audio thread's transient shared_ptr copy is destroyed it is
//     never the last owner — deallocation always happens on the message thread.
//   * The synth is ONLY ever touched on the audio thread. A sequence swap
//     raises `sequenceSwapped_`; process() consumes it and does the
//     all-notes-off itself, so no synth lock is taken cross-thread (no priority
//     inversion of the audio callback).
// =============================================================================
#pragma once

#include <array>
#include <atomic>
#include <memory>

#include <juce_audio_utils/juce_audio_utils.h>

#include "PlaybackSequence.h"
#include "PreviewSynth.h"

namespace acai
{
    class PreviewPlayer
    {
    public:
        PreviewPlayer() = default;

        // --- message thread --------------------------------------------------
        void prepare (double sampleRate, int maxBlockSize);
        void releaseResources();

        /// Swap in a new sequence. Safe during playback; sounding notes get a
        /// soft all-notes-off so nothing hangs across the swap.
        void setSequence (std::shared_ptr<const PlaybackSequence> seq);

        void play();                      ///< (re)start from the loop top
        void stop();                      ///< stop + all-notes-off
        void setLooping (bool shouldLoop) { looping_.store (shouldLoop); }
        void setBpm (double bpm)          { bpm_.store (bpm); }

        // --- GUI thread ------------------------------------------------------
        bool   isPlaying() const     { return playing_.load(); }
        bool   isLooping() const     { return looping_.load(); }
        double playheadTicks() const { return playheadTicks_.load(); }

        // --- audio thread ----------------------------------------------------
        /// Renders preview audio into `buffer` (additively mixes the synth).
        void process (juce::AudioBuffer<float>& buffer);

    private:
        void fireEventsInRange (double fromTick, double toTick, int sampleOffsetBase,
                                double samplesPerTick,
                                const PlaybackSequence& seq,
                                juce::MidiBuffer& midiScratch);

        PreviewSynth synth_;
        juce::MidiBuffer midiScratch_;      ///< preallocated, audio thread only

        std::shared_ptr<const PlaybackSequence> sequence_;   // guarded by lock_
        juce::SpinLock lock_;

        // Message-thread-owned keep-alives so the audio thread's transient copy
        // is never the last owner (frees happen here, on the message thread).
        std::array<std::shared_ptr<const PlaybackSequence>, 4> retainRing_;
        int retainWrite_ = 0;

        std::atomic<bool>   playing_  { false };
        std::atomic<bool>   looping_  { true };
        std::atomic<bool>   restartRequested_ { false };
        std::atomic<bool>   stopRequested_    { false };
        std::atomic<bool>   sequenceSwapped_  { false };
        std::atomic<double> bpm_      { 72.0 };
        std::atomic<double> playheadTicks_ { 0.0 };

        double sampleRate_   = 44100.0;
        int    lastBlockSize_ = 512;        ///< current block size (for offset clamping)
        double posTicks_     = 0.0;         ///< audio-thread-local position
    };
}
