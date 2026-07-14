#include "PluginEditor.h"

using namespace voxshred;

namespace
{
    juce::String rateValueToText (const juce::Slider&, double value)
    {
        auto names = getSyncDivisionNames();
        auto index = juce::jlimit (0, names.size() - 1, (int) std::round (value));
        return names[index];
    }
}

VoxShredAudioProcessorEditor::VoxShredAudioProcessorEditor (VoxShredAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      shiftKnob (p.apvts, voxshred::ParamIDs::shiftAmount, "SHIFT AMOUNT", Palette::formantAccent),
      depthKnob (p.apvts, voxshred::ParamIDs::depth, "DEPTH", Palette::rateAccent),
      rateKnob (p.apvts, voxshred::ParamIDs::rate, "RATE", Palette::rateAccent, rateValueToText),
      mixKnob (p.apvts, voxshred::ParamIDs::mix, "MIX", Palette::mixAccent),
      characterKnob (p.apvts, voxshred::ParamIDs::character, "CHARACTER", Palette::characterAccent),
      formantDisplay (p),
      modRateDisplay (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("VOXSHRED", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (26.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("VOCAL-FORMANT-MODULATOR  //  MALUS AUDIO", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (11.0f, juce::Font::plain));
    subtitleLabel.setColour (juce::Label::textColourId, Palette::rateAccent);
    subtitleLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (subtitleLabel);

    auto setupSectionLabel = [this] (juce::Label& label, const juce::String& text, juce::Colour colour)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::Font (12.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, colour);
        label.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (label);
    };

    setupSectionLabel (formantSectionLabel, "FORMANT ENGINE", Palette::formantAccent);
    setupSectionLabel (rateSectionLabel, "RHYTHM / SYNC", Palette::rateAccent);
    setupSectionLabel (characterSectionLabel, "CHARACTER", Palette::characterAccent);
    setupSectionLabel (mixSectionLabel, "MIX", Palette::mixAccent);

    addAndMakeVisible (shiftKnob);
    addAndMakeVisible (depthKnob);
    addAndMakeVisible (rateKnob);
    addAndMakeVisible (mixKnob);
    addAndMakeVisible (characterKnob);
    addAndMakeVisible (formantDisplay);
    addAndMakeVisible (modRateDisplay);

    setResizable (false, false);
    setSize (620, 420);
}

VoxShredAudioProcessorEditor::~VoxShredAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void VoxShredAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::background);

    juce::ColourGradient bgGradient (Palette::background.brighter (0.04f), 0, 0,
                                       Palette::background.darker (0.3f), 0, (float) getHeight(), false);
    g.setGradientFill (bgGradient);
    g.fillRect (getLocalBounds());

    g.setColour (Palette::formantAccent.withAlpha (0.5f));
    g.drawLine (20.0f, 62.0f, (float) getWidth() - 20.0f, 62.0f, 1.0f);
}

void VoxShredAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (16);

    auto header = bounds.removeFromTop (54);
    titleLabel.setBounds (header.removeFromTop (32));
    subtitleLabel.setBounds (header);

    bounds.removeFromTop (12);

    auto displaysRow = bounds.removeFromTop (110);
    formantDisplay.setBounds (displaysRow.removeFromLeft (displaysRow.getWidth() / 2).reduced (6, 0));
    modRateDisplay.setBounds (displaysRow.reduced (6, 0));

    bounds.removeFromTop (10);

    auto knobRow = bounds.removeFromTop (150);
    auto knobWidth = knobRow.getWidth() / 5;

    auto formantArea = knobRow.removeFromLeft (knobWidth);
    formantSectionLabel.setBounds (formantArea.removeFromTop (16));
    shiftKnob.setBounds (formantArea.reduced (4));

    auto depthArea = knobRow.removeFromLeft (knobWidth);
    rateSectionLabel.setBounds (depthArea.removeFromTop (16));
    depthKnob.setBounds (depthArea.reduced (4));

    auto rateArea = knobRow.removeFromLeft (knobWidth);
    rateArea.removeFromTop (16);
    rateKnob.setBounds (rateArea.reduced (4));

    auto characterArea = knobRow.removeFromLeft (knobWidth);
    characterSectionLabel.setBounds (characterArea.removeFromTop (16));
    characterKnob.setBounds (characterArea.reduced (4));

    auto mixArea = knobRow;
    mixSectionLabel.setBounds (mixArea.removeFromTop (16));
    mixKnob.setBounds (mixArea.reduced (4));
}
