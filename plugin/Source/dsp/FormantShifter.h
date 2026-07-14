#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace voxshred
{

/**
    Real-time formant shifter that decouples timbre (spectral envelope) from pitch.

    The processor works frame-by-frame using an FFT / overlap-add pipeline:
      1. Analyse the magnitude spectrum of the incoming signal.
      2. Extract a smooth spectral envelope (the "formants") via cepstral
         liftering.
      3. Warp that envelope up/down in frequency by `shiftSemitones`.
      4. Re-synthesise the signal by dividing out the original envelope and
         multiplying back in the warped one, leaving the excitation (pitch)
         untouched.

    This gives the classic "robot / alien / chipmunk-without-the-pitch-change"
    vocal texture used heavily in Montagem-style vocal chopping.
*/
class FormantShifter
{
public:
    FormantShifter() = default;

    void prepare (const juce::dsp::ProcessSpec& spec);
    void reset();

    /** Shift amount in semitones, positive = formants raised, negative = lowered. */
    void setShiftSemitones (float semitones) noexcept { shiftSemitones = semitones; }

    /** Processes a single sample and returns the formant-shifted sample. */
    float processSample (int channel, float inputSample);

    /** Returns a normalised (0-1 x, 0-1 y) snapshot of the current spectral
        envelope, used to drive the UI's formant curve display. */
    const std::vector<float>& getEnvelopeSnapshot() const noexcept { return envelopeSnapshot; }

private:
    static constexpr int fftOrder = 10;               // 1024 point FFT
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int hopSize  = fftSize / 4;       // 75% overlap

    struct ChannelState
    {
        std::vector<float> inputFifo;
        std::vector<float> outputFifo;
        std::vector<float> fftBuffer;
        int fifoIndex = 0;
        int samplesSinceLastHop = 0;
    };

    void processFrame (ChannelState& state);
    void extractEnvelope (const std::vector<float>& magnitudes, std::vector<float>& envelopeOut);
    void warpEnvelope (const std::vector<float>& envelope, std::vector<float>& warpedOut, float ratio);

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) fftSize, juce::dsp::WindowingFunction<float>::hann, false };

    std::vector<ChannelState> channels;
    std::vector<float> envelopeSnapshot;

    double sampleRate = 44100.0;
    float shiftSemitones = 0.0f;
};

} // namespace voxshred
