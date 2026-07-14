# VOXSHRED

**Vocal-Formant-Modulator** by **Malus Audio**

A VST3 / AU / Standalone effect plugin that turns a vocal recording into a
pulsing, tempo-locked synthetic instrument — built specifically for
Montagem-style vocal chopping, without the manual chopping and automation.

## Concept

VOXSHRED decouples a vocal's formants (timbre) from its pitch and rhythmically
modulates them using a sequencer/LFO that is hard-locked to the host's tempo
and transport position. The result ranges from subtle robotic colouration to
full-on alien, rhythmically stuttering vocal textures that sit perfectly on
the beat grid of a Montagem-style arrangement.

## Modules

- **Formant Engine** (pink/red) — real-time FFT-based formant shifter that
  warps the spectral envelope independently of pitch. `Shift Amount` sets the
  static formant offset in semitones.
- **Rhythm / Sync** (cyan) — a tempo-synced LFO/sequencer with `Depth` and a
  `Rate` knob offering `1/1, 1/2, 1/4, 1/8, 1/16, 1/32` sync divisions, driving
  rhythmic formant jumps/wobbles that stay locked to the DAW playhead.
- **Character** (amber) — a tanh-based saturation stage that adds grit and
  glue so the processed vocal cuts through a dense mix.
- **Mix** (green) — a large dry/wet knob blending the raw and processed
  signal.

The UI also includes two live-updating displays: a **formant curve** view of
the current spectral envelope, and a **mod rate** view showing the tempo-
synced modulation shape and its current phase.

## Building

This is a [JUCE](https://juce.com/) plugin built with CMake. JUCE itself is
fetched automatically via `FetchContent` — no submodules to initialise.

```sh
cmake -B build -G Ninja plugin
cmake --build build --target VoxShred_VST3       # or VoxShred_AU / VoxShred_Standalone
```

On Linux, building requires the usual JUCE GUI dependencies
(`libx11-dev`, `libxrandr-dev`, `libxcursor-dev`, `libxinerama-dev`,
`libxext-dev`, `libfreetype6-dev`, `libfontconfig1-dev`, `libasound2-dev`,
`libgtk-3-dev`, `libcurl4-openssl-dev`).

`AU` builds only run on macOS; on other platforms that target is skipped
automatically by JUCE's CMake support.
