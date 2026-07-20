// =============================================================================
//  PreviewSynth.cpp — the small built-in audition synth (NOT the user's sound).
// =============================================================================
#include "PreviewSynth.h"

#include <cmath>

namespace acai
{
    void PreviewVoice::startNote (int midiNoteNumber, float velocity,
                                  juce::SynthesiserSound*, int)
    {
        phase_ = 0.0;
        const double freq = juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        phaseDelta_ = freq / getSampleRate();
        level_ = juce::jlimit (0.05f, 1.0f, velocity) * 0.22f;   // headroom for many voices

        // Soft, pad-like envelope: slow attack, long release => click-free.
        juce::ADSR::Parameters p;
        p.attack  = 0.02f;
        p.decay   = 0.15f;
        p.sustain = 0.85f;
        p.release = 0.55f;
        adsr_.setParameters (p);
        adsr_.setSampleRate (getSampleRate());
        adsr_.noteOn();
        lpState_ = 0.0f;
    }

    void PreviewVoice::stopNote (float, bool allowTailOff)
    {
        if (allowTailOff)
        {
            adsr_.noteOff();
        }
        else
        {
            adsr_.reset();
            clearCurrentNote();
        }
    }

    void PreviewVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                        int startSample, int numSamples)
    {
        if (! adsr_.isActive())
            return;

        // Gentle low-pass coefficient (rounds off the triangle-ish partials).
        const float lpCoeff = 0.35f;

        while (--numSamples >= 0)
        {
            const double t = phase_;
            // Additive: fundamental + quiet 2nd + fainter 3rd => warm, soft.
            float s = (float) (std::sin (t * juce::MathConstants<double>::twoPi)
                     + 0.18 * std::sin (t * 2.0 * juce::MathConstants<double>::twoPi)
                     + 0.08 * std::sin (t * 3.0 * juce::MathConstants<double>::twoPi));
            s *= 0.7f;

            // one-pole low pass for a rounded tone
            lpState_ += lpCoeff * (s - lpState_);
            const float env = adsr_.getNextSample();
            const float value = lpState_ * env * level_;

            for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
                outputBuffer.addSample (ch, startSample, value);

            ++startSample;
            phase_ += phaseDelta_;
            if (phase_ >= 1.0) phase_ -= 1.0;

            if (! adsr_.isActive())
            {
                clearCurrentNote();
                break;
            }
        }
    }

    // -------------------------------------------------------------------------
    PreviewSynth::PreviewSynth()
    {
        for (int i = 0; i < 32; ++i)
            addVoice (new PreviewVoice());
        addSound (new PreviewSound());
        setNoteStealingEnabled (true);
    }

    void PreviewSynth::prepare (double sampleRate)
    {
        setCurrentPlaybackSampleRate (sampleRate);
    }

    void PreviewSynth::softStopAll()
    {
        // musical release on all channels
        for (int ch = 1; ch <= 16; ++ch)
            allNotesOff (ch, true);
    }

    void PreviewSynth::panic()
    {
        for (int ch = 1; ch <= 16; ++ch)
            allNotesOff (ch, false);
    }
}
