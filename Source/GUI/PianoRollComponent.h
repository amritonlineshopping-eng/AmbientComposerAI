// =============================================================================
//  PianoRollComponent.h
//  The hero element: draws the generated pattern (chords vs melody in distinct
//  colours), bar/beat grid, key guides, and a smoothly moving playhead.
//  Fits the whole loop; repaints via a light 30 Hz timer only while playing.
// =============================================================================
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <functional>
#include <memory>

#include "../Model/GeneratedPattern.h"

namespace acai
{
    class PianoRollComponent : public juce::Component,
                               private juce::Timer
    {
    public:
        PianoRollComponent();
        ~PianoRollComponent() override = default;

        /// Message thread only. The component keeps its own shared_ptr so the
        /// pattern can't vanish mid-paint.
        void setPattern (std::shared_ptr<const GeneratedPattern> pattern);

        /// Playhead data source (ticks; < 0 means "not playing").
        /// Typically wired to the PreviewPlayer's atomics.
        void setPlayheadSource (std::function<double()> tickProvider);

        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;
        juce::Rectangle<float> noteRect (const Note&, juce::Rectangle<float> area,
                                         int loTicks, int hiTicks,
                                         int loPitch, int hiPitch) const;

        std::shared_ptr<const GeneratedPattern> pattern_;
        std::function<double()> playheadTicks_;
        double lastDrawnPlayhead_ = -1.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PianoRollComponent)
    };
}
