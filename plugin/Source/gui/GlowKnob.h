#pragma once

#include <JuceHeader.h>
#include "VoxShredLookAndFeel.h"

namespace voxshred
{

/**
    A labelled rotary knob bound to an AudioProcessorValueTreeState parameter,
    rendered with the glowing custom LookAndFeel and coloured according to
    its owning module.
*/
class GlowKnob : public juce::Component
{
public:
    GlowKnob (juce::AudioProcessorValueTreeState& state, const juce::String& parameterID,
              const juce::String& displayName, juce::Colour accentColour,
              std::function<juce::String (const juce::Slider&, double)> textFromValue = nullptr)
        : attachment (state, parameterID, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accentColour);
        slider.setColour (juce::Slider::textBoxTextColourId, Palette::textColour);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setLookAndFeel (&lookAndFeel);

        if (textFromValue != nullptr)
            slider.textFromValueFunction = [&slider = slider, textFromValue] (double value)
            {
                return textFromValue (slider, value);
            };

        addAndMakeVisible (slider);

        label.setText (displayName, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, accentColour.brighter (0.3f));
        addAndMakeVisible (label);
    }

    ~GlowKnob() override { slider.setLookAndFeel (nullptr); }

    void resized() override
    {
        auto bounds = getLocalBounds();
        label.setBounds (bounds.removeFromTop (18));
        slider.setBounds (bounds);
    }

private:
    VoxShredLookAndFeel lookAndFeel;
    juce::Slider slider;
    juce::Label label;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
};

} // namespace voxshred
