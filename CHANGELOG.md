# Changelog

## 2.1.0: Mutadrums

First public release, under the new name.

- **Renamed to Mutadrums by mtyas.** New plugin IDs (CLAP `com.mtyas.mutadrums`), so it installs alongside the old *MutableDrums* build instead of replacing it. User presets are copied over from the old folder automatically.
- **Interactive EQ.** Drag the L / M / H points on the EQ curve to set frequency and gain. Mouse wheel on M sets Q; double-click resets gain.
- **Smoother EQ.** Coefficients glide over about 30 ms instead of jumping once per audio block, so sweeps and automation don't zipper. The curve redraws as soon as a value changes, instead of lagging behind the knobs.
- **Separate outputs.** Seven extra stereo outputs (Out 2–8). Each voice has an Output selector on the VOICE page and the mixer. Disabled outputs fall back to Main.
- **Init Kit preset.** A plain eight-voice kit with no processing, as the first preset.
- **About page.** A built-in guide to every section, with credits and licence.
- **Licensed under GPL-3.0-or-later**, with documentation for GitHub.

## 2.0: GUI and engine redesign

- **Reverb rewritten.** It's now an 8-line feedback delay network with a Householder matrix, and the decay is set from a real RT60 (0.3–12 s). Because of how it's built it can't run away into feedback. The old tank had roughly +38 dB of internal gain and saturation. Pre-delay also works properly now.
- **Delay rewritten.** Ping-pong now actually bounces. Filtering and saturation sit inside the feedback loop, so each repeat gets darker. Changing the time or division crossfades instead of pitch-warping, and wow is subtle and controlled by **Tape**.
- **Modulation rewritten.** The two overlapping "legacy" slot systems are replaced by 3 clean slots per voice that hold their value for each hit. Timbre and Morph are merged into one **Character** knob.
- **Sequencer fixes.** Swing, Marbles Rate and Marbles Jitter now do something. The clock locks to the host position (no drift), and Euclidean lanes are bar-aligned.
- **Engine pitch fixes.** Each engine now has its own base pitch.
- **MIDI fix.** Before, every voice also answered note 36.
- **Smoother, cheaper processing.** Smoothing on faders, pan and returns, a short fade on choke instead of a click, and pitch is calculated once per change instead of every sample.
- **Thread-safe parameters.** All parameters go through `AudioProcessorValueTreeState`.
- **Compatibility.** Old sessions (JSON state) and old user presets still load and are converted automatically.
