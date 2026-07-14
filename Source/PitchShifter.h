#pragma once
#include <JuceHeader.h>
#include <array>
#include <cmath>

/**
 * Phase-vocoder based pitch shifter.
 *
 * Shifts pitch without changing duration (time-preserving).
 * Uses 1024-point FFT with 75 % overlap (hop = 256).
 * Latency: ~1024 samples (≈ 23 ms at 44100 Hz).
 */
class PitchShifter
{
public:
    static constexpr int FFT_ORDER = 10;
    static constexpr int N         = 1 << FFT_ORDER; // 1024
    static constexpr int HOP       = N / 4;            // 256  (75 % overlap → 4 frames)

    void  prepare (double sampleRate);
    void  reset();
    float processSample (float input, float pitchSemitones);

private:
    void processFrame (float pitchRatio);

    juce::dsp::FFT fft { FFT_ORDER };

    std::array<float, N>     hann {};

    // Circular input history
    std::array<float, N * 2> inBuf {};
    int inWritePos = 0;
    int hopCounter = 0;

    // Overlap-add output accumulator (4× frame size for safety)
    std::array<float, N * 4> outBuf {};
    int outReadPos = 0;

    // Per-bin phase accumulators
    std::array<float, N / 2 + 1> analysisPhase  {};
    std::array<float, N / 2 + 1> synthesisPhase {};

    // FFT working buffers
    std::array<juce::dsp::Complex<float>, N> fftIn  {};
    std::array<juce::dsp::Complex<float>, N> fftOut {};

    // Combined OLA + FFT normalization factor (pre-computed in prepare())
    float olaScale = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchShifter)
};
