// =============================================================================
//  PreviewSynth.h
//  Tiny built-in audition synth — NOT the user's production sound. A soft
//  triangle-ish tone (few gentle partials) with a click-free ADSR, warm and
//  neutral so generated MIDI can be judged musically inside the plugin window.
//  Strictly separated from all generation code.
// =============================================================================
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

namespace acai
{
    /// Accepts every midi note/channel.
    struct PreviewSound : public juce::SynthesiserSound
    {
        bool appliesToNote (int) override    { return true; }
        bool appliesToChannel (int) override { return true; }
    };

    /// One voice: soft additive tone (fundamental + quiet 2nd/3rd partials),
    /// slow attack / long release envelope, per-voice low-pass smoothing so
    /// pads sound rounded. All processing is allocation-free.
    class PreviewVoice : public juce::SynthesiserVoice
    {
    public:
        PreviewVoice() = default;

        bool canPlaySound (juce::SynthesiserSound* s) override
        {
            return dynamic_cast<PreviewSound*> (s) != nullptr;
        }

        void startNote (int midiNoteNumber, float velocity,
                        juce::SynthesiserSound*, int pitchWheel) override;
        void stopNote (float velocity, bool allowTailOff) override;
        void pitchWheelMoved (int) override {}
        void controllerMoved (int, int) override {}
        void renderNextBlock (juce::AudioBuffer<float>&, int startSample,
                              int numSamples) override;

    private:
        double phase_ = 0.0, phaseDelta_ = 0.0;
        float  level_ = 0.0f;
        juce::ADSR adsr_;
        float  lpState_ = 0.0f;      ///< one-pole smoother for rounded tone
    };

    /// The synth itself: 32 voices so 4-chord pads + melody + overlap never
    /// steal audibly. Also exposes a hard "panic" that guarantees silence.
    class PreviewSynth : public juce::Synthesiser
    {
    public:
        PreviewSynth();
        void prepare (double sampleRate);

        /// All-notes-off on every voice, with tail-off (musical stop).
        void softStopAll();

        /// Immediate silence (used on releaseResources / destruction).
        void panic();
    };
}
