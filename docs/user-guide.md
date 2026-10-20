# Mutadrums user guide

[← Back to README](../README.md)

- [Overview](#overview)
- [Header](#header)
- [Topo: pattern generator](#topo-pattern-generator)
- [Chance: random generator](#chance-random-generator)
- [Pads](#pads)
- [VOICE page](#voice-page)
- [Modulation](#modulation)
- [Channel strip: EQ, drive, compressor](#channel-strip-eq-drive-compressor)
- [MIXER page](#mixer-page)
- [EFFECTS page](#effects-page)
- [MIDI](#midi)
- [Tips and shortcuts](#tips-and-shortcuts)

---

## Overview

![VOICE page](images/voice-page.png)

Mutadrums has eight voices. Each voice has:

1. a **trigger**: what makes it play (a Topo lane, a Chance gate, or only MIDI and the pad),
2. an **engine**: what it sounds like (one of 12 drum models),
3. a **channel strip**: EQ, drive, compressor, level, pan, delay and reverb sends, and an output.

The left of the window holds the two pattern generators. The right holds the pads and three pages: **VOICE**, **MIXER** and **EFFECTS**.

The window is resizable from its bottom-right corner, and the whole layout scales.

## Header

| Control | What it does |
|---|---|
| **◀ / ▶** | Previous / next preset |
| **Preset name** | Opens the preset browser (categories, search, load, delete) |
| **Save** | Saves the current sound as a user preset |
| **Undo / Redo** | Undo or redo any change, including preset loads |
| **About** | Built-in guide, credits and licence |
| **RUNNING / STOPPED** | Starts and stops the sequencer. With Host Sync on it shows **ARMED** until the DAW plays. |
| **HOST SYNC** | On: follow the DAW's tempo and transport. Off: run freely at the internal BPM. |
| **BPM** | Internal tempo. Drag it, or double-click to type a value. Shows the host tempo while synced. |
| **MASTER** | Output volume of the Main output, with a stereo meter |

## Topo: pattern generator

Topo decides when the kick (**BD**), snare (**SD**) and hats (**HH**) play.

- **Map XY pad.** Every point on the map is a drum pattern, and neighbouring points are related. Drag slowly to morph one groove into another. A teal ring shows where Chance modulation is currently pushing the map.
- **Density BD / SD / HH.** How busy each lane is. Low settings keep only the strongest hits.
- **Chaos.** Adds random variations on top of the pattern.
- **Swing.** Delays every second 16th note.
- **32-step preview.** Shows exactly what will play. Accents are drawn brighter.
- **Euclidean mode.** Each lane instead spreads its hits evenly across its own **Length** (1 to 32 steps), and Density sets how many hits.

## Chance: random generator

Chance produces three random **gate** streams (**t1, t2, t3**) and three random **voltages** (**X1, X2, X3**).

| Control | What it does |
|---|---|
| **Model** | *Coin Toss* (independent random gates), *Polyrhythm* (related divisions) or *Drums* (kick/snare/hat-like roles) |
| **Rate** | Clock speed relative to 16th notes, from ¼× to 4× |
| **Jitter** | Humanises timing by randomly delaying hits |
| **Bias** | Below 50% favours t1, above 50% favours t3; it also skews the X voltages low or high |
| **Déjà Vu** | 0% = always new; higher values repeat the recent loop more often |
| **Loop** | Length of the Déjà-Vu loop, 1 to 16 steps |

**Chance modulates Topo:** *X1 › Map X*, *X2 › Map Y*, *X3 › Chaos* and *t1 › Density* let the random voltages move the Topo map, so the groove wanders on its own.

## Pads

One pad per voice. Click a pad to **select** it and **hear** it. Each pad shows the engine, the trigger source, a hit light, and **M**ute / **S**olo buttons.

## VOICE page

Everything about the selected voice.

| Control | What it does |
|---|---|
| **Engine** | The sound model (see the table below) |
| **Trigger** | *Topo BD/SD/HH*, *Chance t1/t2/t3* or *MIDI / pad only* |
| **Choke** | Voices in the same group cut each other off, e.g. closed hat stops open hat |
| **MIDI note** | The note that plays this voice |
| **Tune** | Pitch, ±36 semitones |
| **Decay** | Length of the sound |
| **Tone / Character** | Engine-specific; their labels change with the engine |
| **Output panel** | Level, pan, delay send, reverb send, and the **Output** selector (see [Separate outputs](separate-outputs.md)) |

### Engines

| Engine | Tone | Character | Sound |
|---|---|---|---|
| Analog Kick | Tone | Drive | Bridged-T resonator with pitch dive (808 style) |
| Synth Kick | Click | Drive | Swept sine with click (909 style) |
| Analog Snare | Noise Tone | Snappy | Twin resonant shells and noise (808 style) |
| Synth Snare | Noise Tone | Snappy | Swept oscillators and noise (909 style) |
| Closed Hat | Colour | Noise | Metallic square-wave cluster, short |
| Open Hat | Colour | Noise | Metallic cluster, long. Choke it with the closed hat. |
| Modal Resonator | Brightness | Inharmonic | Woodblock → tom → bell as Inharmonic rises |
| Clap | Body | Spread | Multi-burst handclap |
| Pulse Kick | Tone | Drive | Pulse-excited resonator kick |
| Resonant Snare | Noise Tone | Snappy | Dual resonators and noise |
| Metal Cymbal | Colour | Noise | Six detuned metallic oscillators |
| Additive Bell | Brightness | Inharmonic | Additive inharmonic partials |

## Modulation

Each voice has three slots: **Source → Destination × Amount**.

- **Sources:** Chance X1–X3, Velocity (softer hits move the destination), Random (a new value per hit), LFO (one global, tempo-synced LFO: Sine, Triangle, Ramp Up/Down, Square, S&H, 1/16 to 8 bars), Bar Ramp (rises across each bar).
- **Destinations:** Tune (±24 st), Decay, Tone, Character, Level (±24 dB), Pan.

Sources are **sampled once per hit**, so every hit gets its own variation and nothing jumps in the middle of a decaying sound. A teal ring on a knob shows the modulation the last hit received.

## Channel strip: EQ, drive, compressor

### EQ

A low shelf, a peaking mid and a high shelf.

- **Drag the L, M and H points** on the curve to set a band's frequency (left/right) and gain (up/down) together.
- **Mouse wheel over M** changes the mid band's Q (width).
- **Double-click a point** to reset its gain to 0 dB.
- The knobs underneath do the same job and show exact values.
- Changes glide over about 30 ms in the audio, so sweeping or automating a band doesn't zipper. The curve redraws as soon as a value changes, including from automation.

### Drive

**Tape** (smooth, odd harmonics), **Tube** (asymmetric, even harmonics) or **Clip** (hard and punchy). **Tone** darkens below 50% and adds presence above it. **Trim** compensates the level.

### Compressor

Threshold, ratio, attack, release and makeup gain, with a gain-reduction meter. It is off by default; switch it on with its **ON** chip.

## MIXER page

![MIXER page](images/mixer-page.png)

All eight channels side by side. Each strip has **EQ / DRV / CMP** insert switches, Delay and Reverb sends, pan, a fader with meter, an **Output** selector, and mute / solo. Click a strip to select that voice. On the right are the **Delay return**, **Reverb return** and **Master** faders.

## EFFECTS page

- **Delay:** tempo-synced divisions or free time in ms, Feedback, Tone, Ping-Pong (how much the echoes bounce left and right) and Tape (wow and warmth). Filtering sits inside the feedback loop, so each repeat gets darker.
- **Reverb:** an 8-line feedback delay network. **Decay** is set as a real RT60 (0.3–12 s), plus Brightness, Pre-delay and Width. It cannot run away into feedback.

Both show a live picture of the echo pattern or decay.

## MIDI

Each voice answers its own MIDI note. The defaults follow General MIDI:

| Voice | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| Note | 36 C1 | 38 D1 | 42 F#1 | 46 A#1 | 39 D#1 | 45 A1 | 48 C2 | 49 C#2 |

MIDI notes and pad clicks always play a voice, whatever its trigger, and MIDI is sample-accurate. Velocity sets the hit level and can also be a modulation source.

## Tips and shortcuts

- **Double-click** any knob to reset it.
- **Ctrl/Cmd + Z** undoes, **Ctrl/Cmd + Shift + Z** or **Ctrl + Y** redoes.
- Hover over anything for a tooltip.
- Start from the **Init Kit** preset for a blank kit, or from a factory preset in a style you like.
- For a hat that never repeats, set a hat voice's trigger to **Chance t2**, raise **Jitter** a little, and add **Random → Decay**.
