#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace voxshred;

VoxShredAudioProcessor::VoxShredAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    shiftAmountParam = apvts.getRawParameterValue (ParamIDs::shiftAmount);
    depthParam       = apvts.getRawParameterValue (ParamIDs::depth);
    rateParam        = apvts.getRawParameterValue (ParamIDs::rate);
    mixParam         = apvts.getRawParameterValue (ParamIDs::mix);
    characterParam   = apvts.getRawParameterValue (ParamIDs::character);
}

juce::AudioProcessorValueTreeState::ParameterLayout VoxShredAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::shiftAmount, 1 }, "Shift Amount",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("st")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::depth, 1 }, "Depth",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { ParamIDs::rate, 1 }, "Rate", getSyncDivisionNames(), 2));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::mix, 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::character, 1 }, "Character",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.3f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    return { params.begin(), params.end() };
}

void VoxShredAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 1;

    for (auto& shifter : formantShifters)
    {
        shifter.prepare (spec);
        shifter.reset();
    }

    for (auto& sat : saturators)
    {
        sat.prepare (spec);
        sat.reset();
    }

    lfo.prepare (sampleRate);
    lfo.reset();
}

void VoxShredAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VoxShredAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet();
}
#endif

void VoxShredAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples  = buffer.getNumSamples();

    for (auto i = numChannels; i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, numSamples);

    const auto shiftSemitones = shiftAmountParam->load();
    const auto depth          = depthParam->load();
    const auto rateChoice     = static_cast<voxshred::SyncDivision> (juce::jlimit (0,
        (int) voxshred::SyncDivision::numDivisions - 1, (int) rateParam->load()));
    const auto mix            = mixParam->load();
    const auto character      = characterParam->load();

    lfo.setDivision (rateChoice);
    lfo.setDepth (depth);

    double bpm = 120.0;
    double ppq = 0.0;
    bool haveHostPosition = false;

    if (auto* hostPlayHead = getPlayHead())
    {
        if (auto position = hostPlayHead->getPosition())
        {
            if (position->getBpm().hasValue())
                bpm = *position->getBpm();

            if (position->getPpqPosition().hasValue())
            {
                ppq = *position->getPpqPosition();
                haveHostPosition = true;
            }
        }
    }

    for (auto& shifter : formantShifters)
        shifter.setShiftSemitones (shiftSemitones);

    for (auto& sat : saturators)
        sat.setAmount (character);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float modulation;
        if (haveHostPosition)
        {
            const double samplesPerBeat = (currentSampleRate * 60.0) / juce::jmax (1.0, bpm);
            const double ppqOffset = (double) sample / juce::jmax (1.0, samplesPerBeat);
            modulation = lfo.updateFromHostPosition (bpm, ppq + ppqOffset);
        }
        else
        {
            modulation = lfo.updateFreeRunning (bpm, 1.0);
        }

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto& shifter = formantShifters[(size_t) juce::jmin (channel, 1)];
            auto& sat     = saturators[(size_t) juce::jmin (channel, 1)];

            auto* channelData = buffer.getWritePointer (channel);
            const auto dry = channelData[sample];

            // Rhythmic formant jumps: modulate the effective shift amount by
            // the tempo-synced LFO, scaled by depth.
            shifter.setShiftSemitones (shiftSemitones + modulation * 12.0f);

            auto wet = shifter.processSample (channel, dry);
            wet = sat.processSample (wet);

            channelData[sample] = dry * (1.0f - mix) + wet * mix;
        }
    }
}

juce::AudioProcessorEditor* VoxShredAudioProcessor::createEditor()
{
    return new VoxShredAudioProcessorEditor (*this);
}

void VoxShredAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void VoxShredAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxShredAudioProcessor();
}
