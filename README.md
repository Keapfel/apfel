# Chaos Chopper – VST3 Plugin

**Chaos Chopper** ist ein VST3-Plugin für FL Studio (und andere DAWs) für den Montagem-Produktionsstil.  
Automatisches Stutter-Chopping, randomisierter Chaos-Regler, Pitch/Formant-Shifter und Hard Clipper.

---

## Features

| Feature | Beschreibung |
|---------|-------------|
| **Auto-Chopper** | Zerschneidet Audio in Echtzeit – immer perfekt im BPM-Grid des DAW (1/4, 1/8, 1/16, 1/32, 1/64) |
| **CHAOS-Regler** | Zentraler Knopf: löst zufällig Pitch-Sprünge, Reverse-Playback und Divisions-Wechsel aus – alles taktgenau |
| **Pitch** | Zeit-erhaltender Pitch-Shifter (Phasenvokoder) · ±24 Halbtöne |
| **Formant** | Zwei resonante Bandpass-Filter simulieren Vokalformanten · ±12 Halbtöne |
| **Clip Drive** | Hard Clipper mit bis zu 20× Gain für aggressiven Montagem-Sound |
| **Dry/Wet** | Blendet den Effekt stufenlos ein |

---

## Voraussetzungen

| Tool | Version |
|------|---------|
| CMake | ≥ 3.22 |
| C++ Compiler | C++17 (MSVC 2019+, Clang 12+, GCC 10+) |
| Git | beliebig |
| Internetverbindung | Beim ersten Build lädt CMake JUCE 7.0.9 automatisch herunter |

---

## Build-Anleitung

```bash
# 1. Repository klonen
git clone https://github.com/Keapfel/apfel.git
cd apfel

# 2. Build-Verzeichnis anlegen
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Plugin bauen (dauert beim ersten Mal ~5–10 Min. wegen JUCE-Download)
cmake --build build --config Release
```

Nach erfolgreichem Build findest du das Plugin hier:

| Format | Pfad (Windows) |
|--------|---------------|
| **VST3** | `build/ChaosChopper_artefacts/Release/VST3/Chaos Chopper.vst3` |
| **Standalone** | `build/ChaosChopper_artefacts/Release/Standalone/Chaos Chopper.exe` |

Kopiere die `.vst3`-Datei in deinen VST3-Plugin-Ordner:  
`C:\Program Files\Common Files\VST3\`

---

## Parameter

| Parameter | Bereich | Default | Funktion |
|-----------|---------|---------|----------|
| **Chop On/Off** | Aus / An | An | Stutter-Engine aktivieren |
| **Division** | 1/4 … 1/64 | 1/16 | Taktunterteilung des Choppers |
| **Chaos** | 0 – 1 | 0 | Zufallseffekte (Pitch, Reverse, Division) |
| **Pitch** | −24 … +24 st | 0 | Tonhöhe ohne Tempoänderung |
| **Formant** | −12 … +12 st | 0 | Vokalformant-Verschiebung |
| **Clip Drive** | 0 – 1 | 0 | Hard-Clipper-Gain (0 = aus) |
| **Dry/Wet** | 0 – 1 | 1 | Effektanteil |

---

## Verwendung in FL Studio

1. **Chaos Chopper** auf einen Vocal- oder Bass-Channel in der Mixer-Kette einfügen.  
2. BPM des Songs korrekt einstellen – das Plugin liest das Grid direkt aus FL Studio.  
3. Division auf **1/16** stellen, Dry/Wet auf **1.0** drehen.  
4. **CHAOS-Regler** langsam aufdrehen → ab ~0.5 beginnen Pitch-Sprünge und Reverse-Segmente.  
5. **Clip Drive** auf ~0.4–0.7 für typischen Montagem-Distortion-Charakter.  
6. Mit dem **Pitch-Knopf** Vocals auf +12 oder −12 Halbtöne pitchen (bei ausgeschaltetem Chop).

---

## Projektstruktur

```
apfel/
├── CMakeLists.txt          # JUCE-Build via FetchContent
└── Source/
    ├── PitchShifter.h/.cpp # Phasenvokoder (Zeit-erhaltend)
    ├── PluginProcessor.h/.cpp # DSP: Stutter, Chaos, Formant, Clipper
    └── PluginEditor.h/.cpp    # UI (520 × 370 px, dunkles Theme)
```
