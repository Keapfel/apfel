#include "VoxShredLookAndFeel.h"

namespace voxshred
{

VoxShredLookAndFeel::VoxShredLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, Palette::background);
    setColour (juce::Label::textColourId, Palette::textColour);
    setColour (juce::ComboBox::backgroundColourId, Palette::panel);
    setColour (juce::ComboBox::textColourId, Palette::textColour);
    setColour (juce::ComboBox::outlineColourId, Palette::rateAccent);
    setColour (juce::PopupMenu::backgroundColourId, Palette::panel);
    setColour (juce::PopupMenu::textColourId, Palette::textColour);
}

void VoxShredLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPosProportional, float rotaryStartAngle,
                                             float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (6.0f);
    auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto centre = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // Outer glow — several soft, decreasing-alpha rings behind the knob body.
    for (int i = 6; i >= 1; --i)
    {
        auto glowAlpha = (0.05f * (float) i) * (0.4f + 0.6f * sliderPosProportional);
        g.setColour (accent.withAlpha (glowAlpha));
        g.fillEllipse (bounds.expanded ((float) i * 1.6f));
    }

    // Knob body.
    auto bodyBounds = bounds.reduced (radius * 0.12f);
    juce::ColourGradient bodyGradient (Palette::panel.brighter (0.15f), centre.x, bodyBounds.getY(),
                                        Palette::background, centre.x, bodyBounds.getBottom(), false);
    g.setGradientFill (bodyGradient);
    g.fillEllipse (bodyBounds);

    g.setColour (accent.withAlpha (0.9f));
    g.drawEllipse (bodyBounds, 1.8f);

    // Value arc.
    juce::Path arc;
    auto arcRadius = radius - 2.0f;
    arc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (Palette::panel.brighter (0.05f));
    g.strokePath (arc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path valueArc;
    valueArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (valueArc, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer.
    juce::Path pointer;
    auto pointerLength = radius * 0.68f;
    auto pointerThickness = 3.0f;
    pointer.addRoundedRectangle (-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.6f, 1.5f);
    pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre));

    g.setColour (juce::Colours::white);
    g.fillPath (pointer);

    // Centre glow dot.
    g.setColour (accent);
    g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (centre));
}

juce::Font VoxShredLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (13.0f, juce::Font::bold);
}

void VoxShredLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                         int, int, int, int, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0, 0, (float) width, (float) height).reduced (1.0f);
    g.setColour (Palette::panel);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (bounds, 4.0f, 1.4f);

    juce::Path arrow;
    auto arrowZone = bounds.removeFromRight (18.0f).reduced (5.0f);
    arrow.addTriangle (arrowZone.getX(), arrowZone.getY(),
                        arrowZone.getRight(), arrowZone.getY(),
                        arrowZone.getCentreX(), arrowZone.getBottom());
    g.setColour (Palette::textColour);
    g.fillPath (arrow);
}

} // namespace voxshred
