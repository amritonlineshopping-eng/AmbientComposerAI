// =============================================================================
//  PianoRollComponent.cpp — the hero piano-roll view.
// =============================================================================
#include "PianoRollComponent.h"
#include "LookAndFeelDark.h"

#include <cmath>

namespace acai
{
    PianoRollComponent::PianoRollComponent()
    {
        setInterceptsMouseClicks (false, false);
    }

    void PianoRollComponent::setPattern (std::shared_ptr<const GeneratedPattern> pattern)
    {
        pattern_ = std::move (pattern);
        repaint();
    }

    void PianoRollComponent::setPlayheadSource (std::function<double()> tickProvider)
    {
        playheadTicks_ = std::move (tickProvider);
        if (playheadTicks_ && ! isTimerRunning())
            startTimerHz (30);
    }

    void PianoRollComponent::timerCallback()
    {
        if (! playheadTicks_) return;
        const double now = playheadTicks_();
        if (std::abs (now - lastDrawnPlayhead_) > 0.5)
        {
            lastDrawnPlayhead_ = now;
            repaint();
        }
    }

    juce::Rectangle<float> PianoRollComponent::noteRect (const Note& n,
                                                         juce::Rectangle<float> area,
                                                         int loTicks, int hiTicks,
                                                         int loPitch, int hiPitch) const
    {
        const float tSpan = (float) juce::jmax (1, hiTicks - loTicks);
        const float pSpan = (float) juce::jmax (1, hiPitch - loPitch);
        const float x = area.getX() + (n.startTick - loTicks) / tSpan * area.getWidth();
        const float w = juce::jmax (2.0f, n.lengthTick / tSpan * area.getWidth());
        const float rowH = area.getHeight() / (pSpan + 1);
        const float y = area.getBottom() - (n.pitch - loPitch + 1) * rowH;
        return { x, y, w, juce::jmax (2.0f, rowH - 1.0f) };
    }

    void PianoRollComponent::paint (juce::Graphics& g)
    {
        auto area = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (palette::rollBg);
        g.fillRoundedRectangle (area, 8.0f);

        if (! pattern_ || pattern_->empty())
        {
            g.setColour (palette::textDim);
            g.setFont (15.0f);
            g.drawText ("Press Generate to compose", area, juce::Justification::centred);
            return;
        }

        // pitch range across both tracks (with a little padding)
        int loPitch = 127, hiPitch = 0;
        auto scan = [&] (const std::vector<Note>& v)
        {
            for (const auto& n : v) { loPitch = juce::jmin (loPitch, n.pitch); hiPitch = juce::jmax (hiPitch, n.pitch); }
        };
        scan (pattern_->chordNotes);
        scan (pattern_->melodyNotes);
        if (loPitch > hiPitch) { loPitch = 48; hiPitch = 72; }
        loPitch -= 2; hiPitch += 2;

        const int loTicks = 0;
        const int hiTicks = juce::jmax (1, pattern_->lengthTicks);

        auto inner = area.reduced (6.0f);

        // --- grid: bar + beat lines -----------------------------------------
        const int barTicks  = pattern_->timeSig.barLengthTicks();
        const int beatTicks = pattern_->timeSig.strongBeatEveryTicks();
        const float tSpan = (float) hiTicks;
        for (int t = 0; t <= hiTicks; t += beatTicks)
        {
            const float x = inner.getX() + t / tSpan * inner.getWidth();
            const bool isBar = (barTicks > 0 && (t % barTicks) == 0);
            g.setColour (isBar ? palette::rollGridBar : palette::rollGridBeat);
            g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());
        }

        // --- chord notes (drawn under the melody) ---------------------------
        for (const auto& n : pattern_->chordNotes)
        {
            auto r = noteRect (n, inner, loTicks, hiTicks, loPitch, hiPitch);
            g.setColour (palette::chordNote.withAlpha (0.55f + n.velocity / 320.0f));
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (palette::chordNote.brighter (0.25f).withAlpha (0.5f));
            g.drawRoundedRectangle (r, 2.5f, 0.8f);
        }

        // --- melody notes ---------------------------------------------------
        for (const auto& n : pattern_->melodyNotes)
        {
            auto r = noteRect (n, inner, loTicks, hiTicks, loPitch, hiPitch);
            g.setColour (palette::melodyNote.withAlpha (0.7f + n.velocity / 400.0f));
            g.fillRoundedRectangle (r, 2.5f);
            g.setColour (palette::melodyNote.brighter (0.3f));
            g.drawRoundedRectangle (r, 2.5f, 0.9f);
        }

        // --- playhead -------------------------------------------------------
        if (playheadTicks_)
        {
            const double ph = playheadTicks_();
            if (ph >= 0.0)
            {
                const float x = inner.getX() + (float) (ph / tSpan) * inner.getWidth();
                g.setColour (palette::playhead.withAlpha (0.85f));
                g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());
                g.setColour (palette::playhead.withAlpha (0.25f));
                g.fillRect (juce::Rectangle<float> (x - 1.5f, inner.getY(), 3.0f, inner.getHeight()));
            }
        }
    }

    void PianoRollComponent::resized() {}
}
