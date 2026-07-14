#include "FormantShifter.h"

namespace voxshred
{

void FormantShifter::prepare (const juce::dsp::ProcessSpec& spec)
{
    sampleRate = spec.sampleRate;

    channels.assign ((size_t) spec.numChannels, ChannelState());
    for (auto& ch : channels)
    {
        ch.inputFifo.assign (fftSize, 0.0f);
        ch.outputFifo.assign (fftSize, 0.0f);
        ch.fftBuffer.assign ((size_t) fftSize * 2, 0.0f);
        ch.fifoIndex = 0;
        ch.samplesSinceLastHop = 0;
    }

    envelopeSnapshot.assign (fftSize / 2, 0.0f);
}

void FormantShifter::reset()
{
    for (auto& ch : channels)
    {
        std::fill (ch.inputFifo.begin(), ch.inputFifo.end(), 0.0f);
        std::fill (ch.outputFifo.begin(), ch.outputFifo.end(), 0.0f);
        std::fill (ch.fftBuffer.begin(), ch.fftBuffer.end(), 0.0f);
        ch.fifoIndex = 0;
        ch.samplesSinceLastHop = 0;
    }
}

float FormantShifter::processSample (int channel, float inputSample)
{
    if (channel < 0 || (size_t) channel >= channels.size())
        return inputSample;

    auto& state = channels[(size_t) channel];

    // Read the finished output sample before overwriting it with new accumulation.
    float outSample = state.outputFifo[(size_t) state.fifoIndex];
    state.outputFifo[(size_t) state.fifoIndex] = 0.0f;

    state.inputFifo[(size_t) state.fifoIndex] = inputSample;
    state.fifoIndex++;
    state.samplesSinceLastHop++;

    if (state.fifoIndex >= fftSize)
        state.fifoIndex = 0;

    if (state.samplesSinceLastHop >= hopSize)
    {
        state.samplesSinceLastHop = 0;
        processFrame (state);
    }

    return outSample;
}

void FormantShifter::processFrame (ChannelState& state)
{
    // Copy the fifo (which is a circular buffer starting at fifoIndex) into a
    // linear analysis buffer, oldest sample first.
    std::vector<float> analysis ((size_t) fftSize, 0.0f);
    for (int i = 0; i < fftSize; ++i)
    {
        auto idx = (size_t) ((state.fifoIndex + i) % fftSize);
        analysis[(size_t) i] = state.inputFifo[idx];
    }

    window.multiplyWithWindowingTable (analysis.data(), (size_t) fftSize);

    std::fill (state.fftBuffer.begin(), state.fftBuffer.end(), 0.0f);
    std::copy (analysis.begin(), analysis.end(), state.fftBuffer.begin());

    fft.performRealOnlyForwardTransform (state.fftBuffer.data());

    const int numBins = fftSize / 2;
    std::vector<float> magnitude ((size_t) numBins, 0.0f);
    std::vector<float> phase ((size_t) numBins, 0.0f);

    for (int bin = 0; bin < numBins; ++bin)
    {
        auto re = state.fftBuffer[(size_t) bin * 2];
        auto im = state.fftBuffer[(size_t) bin * 2 + 1];
        magnitude[(size_t) bin] = std::sqrt (re * re + im * im);
        phase[(size_t) bin] = std::atan2 (im, re);
    }

    std::vector<float> envelope ((size_t) numBins, 0.0f);
    extractEnvelope (magnitude, envelope);

    // Ratio > 1 => formants pushed up in frequency, < 1 => pushed down.
    const float ratio = std::pow (2.0f, shiftSemitones / 12.0f);
    std::vector<float> warpedEnvelope ((size_t) numBins, 0.0f);
    warpEnvelope (envelope, warpedEnvelope, ratio);

    for (int bin = 0; bin < numBins; ++bin)
    {
        auto original = juce::jmax (envelope[(size_t) bin], 1.0e-6f);
        auto gain = warpedEnvelope[(size_t) bin] / original;
        gain = juce::jlimit (0.0f, 8.0f, gain);

        auto newMag = magnitude[(size_t) bin] * gain;
        state.fftBuffer[(size_t) bin * 2]     = newMag * std::cos (phase[(size_t) bin]);
        state.fftBuffer[(size_t) bin * 2 + 1] = newMag * std::sin (phase[(size_t) bin]);
    }

    fft.performRealOnlyInverseTransform (state.fftBuffer.data());

    // Empirical normalisation for a Hann-windowed, 75%-overlap OLA.
    constexpr float scale = (float) hopSize / (float) fftSize * 2.0f;

    for (int i = 0; i < fftSize; ++i)
    {
        auto idx = (size_t) ((state.fifoIndex + i) % fftSize);
        state.outputFifo[idx] += state.fftBuffer[(size_t) i] * scale;
    }

    // Publish a normalised snapshot of the (unwarped) envelope for the UI.
    float maxEnv = 1.0e-6f;
    for (auto v : envelope)
        maxEnv = juce::jmax (maxEnv, v);

    if (envelopeSnapshot.size() != envelope.size())
        envelopeSnapshot.assign (envelope.size(), 0.0f);

    for (size_t i = 0; i < envelope.size(); ++i)
        envelopeSnapshot[i] = envelope[i] / maxEnv;
}

void FormantShifter::extractEnvelope (const std::vector<float>& magnitudes, std::vector<float>& envelopeOut)
{
    // Cepstral smoothing: log-magnitude -> low-pass in the "quefrency" domain
    // via a simple moving average approximates cepstral liftering without a
    // second full FFT, which keeps this cheap enough for real-time use.
    const int numBins = (int) magnitudes.size();
    envelopeOut.assign ((size_t) numBins, 0.0f);

    std::vector<float> logMag ((size_t) numBins, 0.0f);
    for (int i = 0; i < numBins; ++i)
        logMag[(size_t) i] = std::log (juce::jmax (magnitudes[(size_t) i], 1.0e-6f));

    const int smoothingRadius = juce::jmax (2, numBins / 40);
    for (int i = 0; i < numBins; ++i)
    {
        float sum = 0.0f;
        int count = 0;
        for (int k = -smoothingRadius; k <= smoothingRadius; ++k)
        {
            auto idx = i + k;
            if (idx >= 0 && idx < numBins)
            {
                sum += logMag[(size_t) idx];
                ++count;
            }
        }
        envelopeOut[(size_t) i] = std::exp (sum / (float) juce::jmax (1, count));
    }
}

void FormantShifter::warpEnvelope (const std::vector<float>& envelope, std::vector<float>& warpedOut, float ratio)
{
    const int numBins = (int) envelope.size();
    warpedOut.assign ((size_t) numBins, 0.0f);

    if (ratio <= 0.0f)
        ratio = 1.0f;

    for (int bin = 0; bin < numBins; ++bin)
    {
        // Sample the original envelope at the frequency this bin should be
        // pulled from in order to move the formant structure by `ratio`.
        float sourceBin = (float) bin / ratio;

        if (sourceBin < 0.0f || sourceBin >= (float) (numBins - 1))
        {
            warpedOut[(size_t) bin] = envelope[(size_t) juce::jlimit (0, numBins - 1, bin)];
            continue;
        }

        auto lowIndex = (int) sourceBin;
        auto frac = sourceBin - (float) lowIndex;
        auto highIndex = juce::jmin (lowIndex + 1, numBins - 1);

        warpedOut[(size_t) bin] = juce::jmap (frac, envelope[(size_t) lowIndex], envelope[(size_t) highIndex]);
    }
}

} // namespace voxshred
