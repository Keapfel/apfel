#pragma once

#include <JuceHeader.h>
#include "VoxShredLookAndFeel.h"

class VoxShredAudioProcessor;

namespace voxshred
{

/**
    Live-updating display of the vocal's spectral formant envelope and the
    frequency region currently being emphasised by the shift amount. Redraws
    at a moderate UI framerate by polling a lock-free snapshot published by
    the FormantShifter DSP class.
*/
class FormantCurveDisplay : public juce::Component, private juce::Timer
{
public:
    explicit FormantCurveDisplay (VoxShredAudioProcessor& processorToUse)
        : processor (processorToUse)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour (Palette::panel);
        g.fillRoundedRectangle (bounds, 6.0f);
        g.setColour (Palette::formantAccent.withAlpha (0.5f));
        g.drawRoundedRectangle (bounds.reduced (0.5f), 6.0f, 1.2f);

        g.setColour (Palette::formantAccent.withAlpha (0.7f));
        g.setFont (juce::Font (11.0f, juce::Font::bold));
        g.drawText ("FORMANT CURVE", bounds.reduced (8.0f, 4.0f), juce::Justification::topLeft);

        if (envelope.empty())
            return;

        auto plotArea = bounds.reduced (10.0f, 22.0f);
        juce::Path path;

        for (size_t i = 0; i < envelope.size(); ++i)
        {
            auto normX = (float) i / (float) (envelope.size() - 1);
            auto px = plotArea.getX() + normX * plotArea.getWidth();
            auto py = plotArea.getBottom() - envelope[i] * plotArea.getHeight();

            if (i == 0)
                path.startNewSubPath (px, py);
            else
                path.lineTo (px, py);
        }

        auto glowPath = path;
        g.setColour (Palette::formantAccent.withAlpha (0.25f));
        g.strokePath (glowPath, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved));

        g.setColour (Palette::formantAccent);
        g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
    }

private:
    void timerCallback() override;

    VoxShredAudioProcessor& processor;
    std::vector<float> envelope;
};

} // namespace voxshred
