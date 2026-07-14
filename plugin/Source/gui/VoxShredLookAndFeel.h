#pragma once

#include <JuceHeader.h>

namespace voxshred
{

/** Colour palette shared across the UI, one accent per functional module. */
namespace Palette
{
    static const juce::Colour background      { 0xff0d0d12 };
    static const juce::Colour panel           { 0xff17171f };
    static const juce::Colour formantAccent   { 0xffff2e63 }; // hot pink/red — formant engine
    static const juce::Colour rateAccent      { 0xff2ee6ff }; // cyan — tempo/rate module
    static const juce::Colour characterAccent { 0xffff9f1c }; // amber — saturation/character
    static const juce::Colour mixAccent       { 0xff7dff6b };  // green — global mix
    static const juce::Colour textColour      { 0xffe8e8f0 };
}

/**
    Custom LookAndFeel drawing dark, glowing rotary knobs instead of the
    stock flat JUCE sliders. Each knob accepts a per-module accent colour via
    the slider's colour ID `juce::Slider::rotarySliderFillColourId`.
*/
class VoxShredLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxShredLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    juce::Font getLabelFont (juce::Label&) override;
    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                        int buttonX, int buttonY, int buttonW, int buttonH,
                        juce::ComboBox& box) override;
};

} // namespace voxshred
