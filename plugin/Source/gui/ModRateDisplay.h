#pragma once

#include <JuceHeader.h>
#include "VoxShredLookAndFeel.h"

class VoxShredAudioProcessor;

namespace voxshred
{

/**
    Live-updating visualisation of the tempo-synced modulation rate: a
    rotating phase indicator plus a scrolling history of the LFO's rhythmic
    output, so the user can see the rhythmic wobble/jump pattern they are
    about to hear, in sync with the host transport.
*/
class ModRateDisplay : public juce::Component, private juce::Timer
{
public:
    explicit ModRateDisplay (VoxShredAudioProcessor& processorToUse)
        : processor (processorToUse)
    {
        history.assign ((size_t) historyLength, 0.0f);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (Palette::panel);
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (Palette::rateAccent.withAlpha (0.5f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.2f);

        g.setColour (Palette::rateAccent.withAlpha (0.7f));
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText ("MOD RATE", bounds.reduced (8.0f, 4.0f), juce::Justification::topLeft);

        auto plotArea = bounds.reduced (10.0f, 22.0f);

        juce::Path path;
        for (size_t i = 0; i < history.size(); ++i)
        {
            auto normX = (float) i / (float) (history.size() - 1);
            auto px = plotArea.getX() + normX * plotArea.getWidth();
            auto py = plotArea.getCentreY() - history[i] * (plotArea.getHeight() * 0.5f);

            if (i == 0)
                path.startNewSubPath (px, py);
            else
                path.lineTo (px, py);
        }

        g.setColour (Palette::rateAccent.withAlpha (0.25f));
        g.strokePath (path, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));
        g.setColour (Palette::rateAccent);
        g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));

        // Phase indicator dot travelling around a small ring, giving an
        // at-a-glance sense of where we are in the current sync division.
        auto ringBounds = juce::Rectangle<float> (16.0f, 16.0f)
                              .withCentre ({ plotArea.getRight() - 10.0f, plotArea.getY() + 8.0f });
        g.setColour (Palette::rateAccent.withAlpha (0.4f));
        g.drawEllipse (ringBounds, 1.2f);

        auto angle = phase * juce::MathConstants<float>::twoPi - juce::MathConstants<float>::halfPi;
        auto dotPos = ringBounds.getCentre().getPointOnCircumference (ringBounds.getWidth() * 0.5f, angle);
        g.setColour (Palette::rateAccent);
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (dotPos));
    }

private:
    void timerCallback() override;

    static constexpr int historyLength = 128;

    VoxShredAudioProcessor& processor;
    std::vector<float> history;
    float phase = 0.0f;
};

} // namespace voxshred
