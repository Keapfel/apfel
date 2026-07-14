#include "PitchShifter.h"

void PitchShifter::prepare (double /*sampleRate*/)
{
    for (int i = 0; i < N; ++i)
        hann[i] = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * i / N));

    // For Hann window at 75 % overlap, sum of overlapping Hann² ≈ 1.5.
    // JUCE FFT does NOT normalise on inverse, so we must divide by N.
    // Combined factor: 1 / (1.5 * N)
    olaScale = 1.0f / (1.5f * (float)N);

    reset();
}

void PitchShifter::reset()
{
    inBuf.fill (0.0f);
    outBuf.fill (0.0f);
    analysisPhase.fill (0.0f);
    synthesisPhase.fill (0.0f);
    fftIn.fill ({});
    fftOut.fill ({});
    inWritePos = 0;
    hopCounter  = 0;
    outReadPos  = 0;
}

float PitchShifter::processSample (float input, float pitchSemitones)
{
    inBuf[inWritePos] = input;
    inWritePos = (inWritePos + 1) & (N * 2 - 1);

    if (++hopCounter >= HOP)
    {
        hopCounter = 0;
        processFrame (std::pow (2.0f, pitchSemitones / 12.0f));
    }

    float out = outBuf[outReadPos];
    outBuf[outReadPos] = 0.0f;
    outReadPos = (outReadPos + 1) % (N * 4);
    return out;
}

void PitchShifter::processFrame (float pitchRatio)
{
    // ── Build windowed analysis frame ──────────────────────────────────────
    for (int i = 0; i < N; ++i)
    {
        int idx    = (inWritePos - N + i + N * 2) & (N * 2 - 1);
        fftIn[i]   = { inBuf[idx] * hann[i], 0.0f };
    }

    // ── Forward FFT ────────────────────────────────────────────────────────
    fft.perform (fftIn.data(), fftOut.data(), false);

    // ── Phase-vocoder analysis (bins 0 … N/2) ─────────────────────────────
    float mag  [N / 2 + 1] {};
    float iFreq[N / 2 + 1] {};

    for (int k = 0; k <= N / 2; ++k)
    {
        float re    = fftOut[k].real();
        float im    = fftOut[k].imag();
        mag[k]      = std::sqrt (re * re + im * im);
        float phase = std::atan2 (im, re);

        // Expected phase advance this hop
        float dPhase = phase - analysisPhase[k]
                     - juce::MathConstants<float>::twoPi * (float)k * HOP / N;

        // Wrap to (−π, π]
        dPhase -= juce::MathConstants<float>::twoPi
               * std::round (dPhase / juce::MathConstants<float>::twoPi);

        iFreq[k]         = juce::MathConstants<float>::twoPi * (float)k / N + dPhase / (float)HOP;
        analysisPhase[k] = phase;
    }

    // ── Bin remapping (pitch shift) ────────────────────────────────────────
    float synMag  [N / 2 + 1] {};
    float synFreq [N / 2 + 1] {};

    for (int k = 0; k <= N / 2; ++k)
    {
        if (mag[k] < 1e-12f) continue;
        int K = (int)std::round ((float)k * pitchRatio);
        if (K >= 0 && K <= N / 2)
        {
            synMag[K]  += mag[k];
            synFreq[K]  = iFreq[k] * pitchRatio;
        }
    }

    // ── Synthesis phase accumulation & spectrum rebuild ────────────────────
    fftIn.fill ({});

    for (int k = 0; k <= N / 2; ++k)
    {
        synthesisPhase[k] += synFreq[k] * (float)HOP;
        float re = synMag[k] * std::cos (synthesisPhase[k]);
        float im = synMag[k] * std::sin (synthesisPhase[k]);
        fftIn[k] = { re, im };

        // Conjugate symmetry for real-valued IFFT result
        if (k > 0 && k < N / 2)
            fftIn[N - k] = { re, -im };
    }

    // ── Inverse FFT ────────────────────────────────────────────────────────
    fft.perform (fftIn.data(), fftOut.data(), true);

    // ── Overlap-add (with Hann synthesis window + normalisation) ──────────
    for (int i = 0; i < N; ++i)
    {
        int idx = (outReadPos + i) % (N * 4);
        outBuf[idx] += fftOut[i].real() * olaScale * hann[i];
    }
}
