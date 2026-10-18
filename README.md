# MutableDrums (VST3 / CLAP / Standalone)

An 8-voice generative drum machine built on Mutable Instruments designs:

- **Pattern generators.** [Grids](https://pichenettes.github.io/mutable-instruments-documentation/modules/grids/) (topographic drum-map morphing and Euclidean rhythms) and [Marbles](https://pichenettes.github.io/mutable-instruments-documentation/modules/marbles/) (random gates t1–t3, random voltages X1–X3, Déjà-Vu looping).
- **Sound engines.** 12 Plaits and Braids-style drum models: 808/909 kicks and snares, closed and open hats, modal percussion, clap, and the Braids kick, snare, cymbal and bell.
- **Per-channel processing.** 3-band EQ, Tape/Tube/Clip drive, compressor, pan, mute/solo, and Delay and Reverb sends.
- **Effects.** A stereo/ping-pong tape delay and an FDN reverb.
- **Automation and undo.** Every control is a host parameter (about 380), so all of it can be automated, and undo/redo works everywhere.

---

## Using the interface

The window is resizable (drag the corner). The whole layout scales.

| Area | What it does |
|---|---|
| **Header** | Presets: ◀ ▶ step through them, click the name to browse, **Save** stores a user preset. Also Undo/Redo, **RUNNING** (start/stop the sequencer), **HOST SYNC** (follow the DAW's tempo and transport, or use the internal BPM), the BPM field (drag it or double-click to type), and Master volume. |
| **Grids** (left, top) | Drag the XY pad to morph between patterns. The 32-step preview shows exactly what BD/SD/HH will play, with accents in bright colours. You also get Chaos, Swing, and a density control for each lane. Switch to **Euclidean** to get length controls for each lane. |
| **Marbles** (left, bottom) | Rate (¼× to 4× of 16ths), Jitter (humanise), Bias, Déjà-Vu and Loop length. The model can be Coin Toss, Polyrhythm or Drums. The "Marbles modulates Grids" knobs let X1/X2/X3/t1 move the Grids map, chaos and density. |
| **Pads** (top row) | One pad per voice. Click a pad to select it **and** hear it. Each pad shows the engine, the trigger source and activity, and has M/S buttons. |
| **VOICE page** | The selected voice: engine, trigger, choke group, MIDI note, then Tune / Decay / Tone / Character. The Tone and Character knobs are relabelled for each engine (e.g. *Snappy*, *Drive*, *Inharmonic*). Below that are the modulation slots, the output controls (level, pan, sends), and the EQ (with response curve), Drive and Compressor (with GR meter). |
| **MIXER page** | All 8 channels: insert on/off, sends, pan, fader with meter, M/S. Also the Delay and Reverb returns and the Master. |
| **EFFECTS page** | Delay and reverb, each with a live picture of the echo pattern or decay. |

Double-click any knob to reset it. Hover over anything for a tooltip.

### Modulation (per hit)
Each voice has 3 slots: **Source → Destination × Amount**. Sources are sampled **once per hit**, so each hit gets its own variation and nothing jumps in the middle of a decaying sound.

- **Sources:** Marbles X1–X3, Velocity (softer hits move the destination), Random (a new value each hit), LFO (a global, tempo-synced LFO with a choice of shapes), and Bar Ramp (rises across the bar).
- **Destinations:** Tune (±24 st), Decay, Tone, Character, Level (±24 dB), Pan.

A teal ring on a knob shows the modulation applied to the last hit.

### MIDI
Each voice has its own MIDI note. The defaults are GM: 36, 38, 42, 46, 39, 45, 48, 49. MIDI notes and pad clicks always play a voice, whatever trigger source it's set to. MIDI is sample-accurate.

---

## What changed in the 2.0 redesign

- **Reverb rewritten.** It's now an 8-line feedback delay network with a Householder matrix, with the decay set from a real RT60 (0.3–12 s). Because of how it's built it can't run away into feedback. The old tank had roughly +38 dB of internal gain and saturation, which is what caused the feedback. The pre-delay also works properly now (at 0 ms it used to read the oldest sample in its buffer).
- **Delay rewritten.** Ping-pong now actually bounces (before, the mono send was fed to both sides equally). Filtering and saturation sit inside the feedback loop, so each repeat gets darker. Changing the time or division crossfades instead of pitch-warping, and wow is subtle and controlled by **Tape**.
- **Modulation rewritten.** The two overlapping "legacy" slot systems are replaced by 3 clean slots per voice that hold their value for each hit. Timbre and Morph are merged into one **Character** knob, because each engine only ever used one of them.
- **Sequencer fixes.** Swing, Marbles Rate and Marbles Jitter now do something. The clock locks to the host position (no drift), and Euclidean lanes are bar-aligned.
- **Engine pitch fixes.** Each engine now has its own base pitch; previously everything was pitched from 55 Hz, so the snares were clamped.
- **MIDI fix.** Before, every voice also answered note 36.
- **Smoother, cheaper processing.** Smoothing on faders, pan and returns, a short fade on choke instead of a click, and pitch is calculated once per change instead of every sample.
- **Thread-safe parameters.** All parameters go through `AudioProcessorValueTreeState`, so the GUI no longer writes into the DSP from the UI thread.
- **Compatibility.** Old sessions (JSON state) and old user presets still load and are converted automatically.

---

## Building

```bash
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Outputs are in `build/MutableDrums_artefacts/Release/` (VST3, CLAP and Standalone). The UI uses the embedded Inter font (SIL OFL, see `resources/fonts/Inter-LICENSE.txt`).

User presets are saved as JSON in `%APPDATA%/MutableDrums/Presets` (Windows) or `~/Library/MutableDrums/Presets` (macOS).
