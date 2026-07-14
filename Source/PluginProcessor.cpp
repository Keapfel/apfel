#include "PluginProcessor.h"
#include "PluginEditor.h"

// Beat subdivision denominators: index 0 = 1/4, …, 4 = 1/64
static constexpr int kDivDenom[] = { 4, 8, 16, 32, 64 };

// ─────────────────────────────────────────────────────────────────────────────
ChaosChopperAudioProcessor::ChaosChopperAudioProcessor()
    : AudioProcessor (BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "State", createParameterLayout())
{
}

ChaosChopperAudioProcessor::~ChaosChopperAudioProcessor() = default;

// ─── Parameter layout ────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
ChaosChopperAudioProcessor::createParameterLayout()
{
    using Float  = juce::AudioParameterFloat;
    using Choice = juce::AudioParameterChoice;
    using Bool   = juce::AudioParameterBool;
    using NR     = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<Bool>   ("chopEnabled",  "Chop On/Off",  true));
    layout.add (std::make_unique<Choice> ("chopDivision", "Division",
                    juce::StringArray { "1/4","1/8","1/16","1/32","1/64" }, 2));
    layout.add (std::make_unique<Float>  ("chaos",     "Chaos",      NR (0.0f,  1.0f),         0.0f));
    layout.add (std::make_unique<Float>  ("pitch",     "Pitch",      NR (-24.0f, 24.0f, 0.5f), 0.0f));
    layout.add (std::make_unique<Float>  ("formant",   "Formant",    NR (-12.0f, 12.0f, 0.5f), 0.0f));
    layout.add (std::make_unique<Float>  ("clipDrive", "Clip Drive", NR (0.0f,  1.0f),         0.0f));
    layout.add (std::make_unique<Float>  ("dryWet",    "Dry/Wet",    NR (0.0f,  1.0f),         1.0f));

    return layout;
}

// ─── Bus layout ──────────────────────────────────────────────────────────────
bool ChaosChopperAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

// ─── Prepare ─────────────────────────────────────────────────────────────────
void ChaosChopperAudioProcessor::prepareToPlay (double sr, int spb)
{
    currentSampleRate = sr;

    rollingBuf.setSize (2, MAX_BUF, false, true, false);
    chopBuf.setSize    (2, MAX_BUF, false, true, false);

    pitchL.prepare (sr);
    pitchR.prepare (sr);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32)spb, 1 };

    for (auto* f : { &formF1L, &formF1R, &formF2L, &formF2R })
    {
        f->prepare (spec);
        f->reset();
        f->setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        f->setResonance (2.5f);
    }
    for (auto* f : { &formF1L, &formF1R }) f->setCutoffFrequency (600.0f);
    for (auto* f : { &formF2L, &formF2R }) f->setCutoffFrequency (1700.0f);

    rollingWritePos   = 0;
    chopReadPos       = 0.0;
    samplesIntoChop   = 0;
    chopUnitSamples   = getDivisionSamples (2, 120.0);
    chopBufReady      = false;

    chaosExtraPitch     = 0.0f;
    chaosReverse        = false;
    chaosOverrideDivIdx = -1;
    chaosHoldUnits      = 0;
    lastFormantValue    = -999.0f;
}

void ChaosChopperAudioProcessor::releaseResources() {}

// ─── Helpers ─────────────────────────────────────────────────────────────────
int ChaosChopperAudioProcessor::getDivisionSamples (int divIdx, double bpm) const
{
    divIdx = juce::jlimit (0, 4, divIdx);
    double beatsPerChop = 4.0 / kDivDenom[divIdx];
    int samples = (int)(beatsPerChop * (60.0 / bpm) * currentSampleRate);
    return juce::jmax (64, juce::jmin (samples, MAX_BUF - 1));
}

double ChaosChopperAudioProcessor::getHostBpm() const
{
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition(); pos.hasValue())
            if (auto bpm = pos->getBpm(); bpm.hasValue())
                return juce::jlimit (20.0, 300.0, *bpm);
    return 120.0;
}

void ChaosChopperAudioProcessor::snapshotToChopBuffer (int numCh, int chopSamples)
{
    // Copy the most recent `chopSamples` input samples (ending one sample
    // before the current write position) into chopBuf[0..chopSamples-1].
    for (int ch = 0; ch < numCh; ++ch)
        for (int i = 0; i < chopSamples; ++i)
        {
            // rollingWritePos-1 is the last written sample; we go back chopSamples from there.
            int srcIdx = (rollingWritePos - 1 - chopSamples + 1 + i + MAX_BUF) % MAX_BUF;
            chopBuf.setSample (ch, i, rollingBuf.getSample (ch, srcIdx));
        }
}

void ChaosChopperAudioProcessor::triggerChaos (float chaosAmt, int baseDivIdx)
{
    if (chaosHoldUnits > 0) { --chaosHoldUnits; return; }

    // Reset chaos state from the previous unit
    chaosExtraPitch     = 0.0f;
    chaosReverse        = false;
    chaosOverrideDivIdx = -1;

    if (chaosAmt <= 0.0f) return;

    // Pitch jump: probability & semitone range both scale with chaos
    if (rng.nextFloat() < chaosAmt * 0.75f)
    {
        int range = juce::jmax (1, (int)(chaosAmt * 24.0f));
        chaosExtraPitch = (float)(rng.nextInt (range * 2 + 1) - range);
    }

    // Reverse playback
    if (rng.nextFloat() < chaosAmt * 0.45f)
        chaosReverse = true;

    // Faster division (subdivide the current beat further)
    if (rng.nextFloat() < chaosAmt * 0.60f)
    {
        int steps = rng.nextInt (3) + 1;                         // 1–3 steps faster
        chaosOverrideDivIdx = juce::jmin (4, baseDivIdx + steps);
        chaosHoldUnits = rng.nextInt (5) + 1;                    // hold for 1–5 chop units
    }
}

void ChaosChopperAudioProcessor::updateFormantFilters (float formantSemitones)
{
    float ratio = std::pow (2.0f, formantSemitones / 12.0f);
    float f1 = juce::jlimit (80.0f, 8000.0f, 600.0f  * ratio);
    float f2 = juce::jlimit (80.0f, 8000.0f, 1700.0f * ratio);
    for (auto* f : { &formF1L, &formF1R }) f->setCutoffFrequency (f1);
    for (auto* f : { &formF2L, &formF2R }) f->setCutoffFrequency (f2);
}

// ─── Process block ────────────────────────────────────────────────────────────
void ChaosChopperAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const double bpm     = getHostBpm();
    const bool   chopOn  = *apvts.getRawParameterValue ("chopEnabled")  > 0.5f;
    const int    divIdx  = (int)*apvts.getRawParameterValue ("chopDivision");
    const float  chaos   = *apvts.getRawParameterValue ("chaos");
    const float  pitch   = *apvts.getRawParameterValue ("pitch");
    const float  formant = *apvts.getRawParameterValue ("formant");
    const float  clipDrv = *apvts.getRawParameterValue ("clipDrive");
    const float  dryWet  = *apvts.getRawParameterValue ("dryWet");

    const int numCh  = juce::jmin (buffer.getNumChannels(), 2);
    const int numSmp = buffer.getNumSamples();

    // Update formant filters only when the knob moves (cheap filter coeff rebuild)
    if (std::abs (formant - lastFormantValue) > 0.05f)
    {
        updateFormantFilters (formant);
        lastFormantValue = formant;
    }

    // Effective division index (may be overridden by chaos)
    int effectiveDivIdx = (chaosOverrideDivIdx >= 0) ? chaosOverrideDivIdx : divIdx;
    int curChopSmps     = getDivisionSamples (effectiveDivIdx, bpm);

    // Sync to DAW timeline so stutter stays on the beat grid
    if (auto* ph = getPlayHead())
    {
        auto pos = ph->getPosition();
        if (pos.hasValue() && pos->getIsPlaying())
            if (auto ts = pos->getTimeInSamples())
                samplesIntoChop = (int)(*ts % (int64_t)curChopSmps);
    }

    for (int s = 0; s < numSmp; ++s)
    {
        // ── Write input sample into rolling history buffer ─────────────────
        for (int ch = 0; ch < numCh; ++ch)
            rollingBuf.setSample (ch, rollingWritePos, buffer.getSample (ch, s));
        rollingWritePos = (rollingWritePos + 1) % MAX_BUF;

        // ── Detect start of a new chop unit ────────────────────────────────
        const bool sizeChanged  = (chopUnitSamples != curChopSmps);
        const bool newChopBound = (samplesIntoChop == 0) || sizeChanged;

        if (newChopBound)
        {
            if (sizeChanged)
            {
                chopUnitSamples = curChopSmps;
                samplesIntoChop = 0;
            }

            snapshotToChopBuffer (numCh, curChopSmps);

            chopReadPos  = chaosReverse ? (double)(curChopSmps - 1) : 0.0;
            chopBufReady = true;

            // Evaluate chaos for the *next* chop unit
            triggerChaos (chaos, divIdx);

            // Recompute chop size in case chaos just changed the override
            effectiveDivIdx = (chaosOverrideDivIdx >= 0) ? chaosOverrideDivIdx : divIdx;
            curChopSmps     = getDivisionSamples (effectiveDivIdx, bpm);
            chopUnitSamples = curChopSmps;
        }

        // ── Compute wet sample(s) ──────────────────────────────────────────
        float wet[2] = { 0.0f, 0.0f };
        const float totalPitch = pitch + chaosExtraPitch;
        const double pitchRatio = std::pow (2.0, (double)totalPitch / 12.0);

        if (chopOn && chopBufReady)
        {
            // Stutter mode: loop the frozen chop buffer with optional pitch
            // (resampling changes playback speed inside the fixed-length window,
            //  producing pitch shifts without changing the chop unit duration).
            int   p0  = (int)chopReadPos;
            float frc = (float)(chopReadPos - p0);
            int   p1  = p0 + 1;
            int   len = chopUnitSamples;

            for (int ch = 0; ch < numCh; ++ch)
            {
                int pp0 = juce::jlimit (0, len - 1, p0);
                int pp1 = juce::jlimit (0, len - 1, p1);
                wet[ch] = chopBuf.getSample (ch, pp0) * (1.0f - frc)
                        + chopBuf.getSample (ch, pp1) * frc;
            }

            // Advance read head (wraps within chop unit for seamless looping)
            if (chaosReverse)
            {
                chopReadPos -= pitchRatio;
                if (chopReadPos < 0.0) chopReadPos += len;
            }
            else
            {
                chopReadPos += pitchRatio;
                if (chopReadPos >= (double)len) chopReadPos -= (double)len;
            }
        }
        else
        {
            // Pass-through mode: apply time-preserving pitch shift via phase vocoder
            float inL = buffer.getSample (0, s);
            float inR = (numCh > 1) ? buffer.getSample (1, s) : inL;

            if (std::abs (pitch) > 0.05f)
            {
                wet[0] = pitchL.processSample (inL, pitch);
                wet[1] = pitchR.processSample (inR, pitch);
            }
            else
            {
                wet[0] = inL;
                wet[1] = inR;
            }
        }

        // ── Formant colouration ────────────────────────────────────────────
        if (std::abs (formant) > 0.1f)
        {
            // Blend original with two bandpass peaks (simulates vocal formant shift)
            float mix = juce::jmin (1.0f, std::abs (formant) / 6.0f) * 0.5f;

            for (int ch = 0; ch < numCh; ++ch)
            {
                float in = wet[ch];
                float f1 = (ch == 0) ? formF1L.processSample (0, in) : formF1R.processSample (0, in);
                float f2 = (ch == 0) ? formF2L.processSample (0, in) : formF2R.processSample (0, in);
                wet[ch]  = in * (1.0f - mix) + (f1 + f2) * mix;
            }
        }

        // ── Hard clipper (soft knee → hard wall) ──────────────────────────
        if (clipDrv > 0.01f)
        {
            float drive = 1.0f + clipDrv * 19.0f; // 1× – 20× gain
            for (int ch = 0; ch < numCh; ++ch)
                wet[ch] = juce::jlimit (-1.0f, 1.0f, wet[ch] * drive) * 0.85f;
        }

        // ── Dry / Wet ──────────────────────────────────────────────────────
        for (int ch = 0; ch < numCh; ++ch)
        {
            float dry = buffer.getSample (ch, s);
            float w   = (numCh > 1) ? wet[ch] : wet[0];
            buffer.setSample (ch, s, dry * (1.0f - dryWet) + w * dryWet);
        }

        // Advance chop counter; wrap at unit boundary
        if (++samplesIntoChop >= chopUnitSamples)
            samplesIntoChop = 0;
    }
}

// ─── State persistence ────────────────────────────────────────────────────────
void ChaosChopperAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void ChaosChopperAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ─── Factory ──────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChaosChopperAudioProcessor();
}
