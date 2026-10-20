# Presets

[← Back to README](../README.md)

## Factory presets

Mutadrums ships with the **Init Kit** and more than 30 factory kits in the Techno, Electro, IDM, Hip Hop, House, DnB, Ambient and World categories.

- **Init Kit** (category *Init*) is the blank starting point: eight raw voices (kick, snare, closed and open hat, clap, low and high tom, cymbal) with flat EQ and no drive, compression or effect sends. Kick, snare and closed hat follow Grids; the others play from MIDI or the pads.
- Factory presets can't be overwritten or deleted. Save a copy under a new name instead.

## Browsing and saving

- **◀ / ▶** in the header step through all presets.
- Click the **preset name** to open the browser: pick a category, type to search, and double-click or press **Load**.
- **Save** stores the current sound. Give it a name and a category (default *User*). Saving with the name of an existing user preset in the same category replaces it.
- Loading a preset can be undone with **Undo**.

## Where user presets live

| OS | Folder |
|---|---|
| Windows | `%APPDATA%\Mutadrums\Presets` |
| macOS | `~/Library/Mutadrums/Presets` |

Each preset is one `.json` file, so you can back presets up, share them, or keep them in version control. Presets saved by earlier *MutableDrums* builds are copied into the new folder the first time Mutadrums runs.

## File format

Presets are JSON. Top-level fields cover the sequencers, effects and master. `channels` is an array of eight voice objects. An abridged example:

```json
{
  "name": "My Kit",
  "category": "User",
  "author": "User",
  "formatVersion": 2,
  "bpm": 120.0,
  "syncToHost": true,
  "gridsX": 0.5, "gridsY": 0.5, "gridsChaos": 0.0,
  "gridsDensity0": 0.6, "gridsDensity1": 0.5, "gridsDensity2": 0.7,
  "marblesRate": 1.0, "marblesDejaVu": 0.0,
  "delayFeedback": 0.4, "reverbDecay": 0.6,
  "masterVolumeDB": -3.0,
  "channels": [
    {
      "engine": 0, "tune": 0.0, "decay": 0.5, "tone": 0.5, "timbre": 0.5, "morph": 0.5,
      "triggerSource": 1, "chokeGroup": 0, "midiNote": 36,
      "volumeDB": -10.0, "pan": 0.0, "aux1SendDB": -60.0, "aux2SendDB": -60.0,
      "eqLowFreq": 100.0, "eqLowGain": 0.0, "eqMidFreq": 1200.0, "eqMidQ": 1.0, "eqMidGain": 0.0,
      "eqHighFreq": 8000.0, "eqHighGain": 0.0, "eqBypass": false,
      "drive": 0.0, "driveMode": 0, "driveBypass": true,
      "compBypass": true,
      "modSource1": 0, "modDest1": 0, "modAmount1": 0.0,
      "outputBus": 0
    }
  ]
}
```

Missing fields fall back to their defaults, so hand-written presets can be short.

| Field | Values |
|---|---|
| `engine` | 0 Analog Kick, 1 Synth Kick, 2 Analog Snare, 3 Synth Snare, 4 Closed Hat, 5 Open Hat, 6 Modal, 7 Clap, 8 Braids Kick, 9 Braids Snare, 10 Braids Cymbal, 11 Braids Bell |
| `triggerSource` | 0 MIDI/pad only, 1 Grids BD, 2 Grids SD, 3 Grids HH, 4–6 Marbles t1–t3 |
| `chokeGroup` | 0 none, 1–4 groups A–D |
| `modSourceN` | 0 Off, 1–3 Marbles X1–X3, 4 Velocity, 5 Random, 6 LFO, 7 Bar Ramp |
| `modDestN` | 0 Tune, 1 Decay, 2 Tone, 3 Character, 4 Level, 5 Pan |
| `driveMode` | 0 Tape, 1 Tube, 2 Clip |
| `outputBus` | 0 Main, 1–7 Out 2–Out 8 |
| `aux1SendDB` / `aux2SendDB` | Delay / Reverb send in dB (−60 = off) |

`timbre` and `morph` both hold the **Character** value; older presets used one or the other depending on the engine.
