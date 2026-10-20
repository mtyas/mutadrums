# Separate outputs

[← Back to README](../README.md)

Mutadrums has **eight stereo outputs**: **Main** and **Out 2** to **Out 8**. Each voice can play through any of them, so you can give the kick, snare and hats their own channels in your DAW mixer and process them there.

## Choosing an output for a voice

Every voice has an **Output** selector:

- on the **VOICE** page, at the top right of the *Output* panel,
- on the **MIXER** page, just above each strip's mute and solo buttons.

Choose **Main** or **Out 2** to **Out 8**. Several voices can share one output, for example both toms on Out 6.

## What a separate output carries

- The voice **after its channel strip**: EQ, drive, compressor, fader and pan.
- The **same level** as it would have on Main.
- **No** master volume and **no** safety limiter. Those only apply to Main.

The voice's delay and reverb sends still feed the effects, and the **delay and reverb returns always play on Main**. If you want a voice completely dry on its own output, turn its sends down.

## Enabling the outputs in your DAW

Out 2–8 are optional outputs, so most DAWs keep them off until you ask for them. **If a voice is set to an output the DAW hasn't enabled, it plays through Main instead**, so nothing goes silent while you set things up.

How to enable them depends on the DAW. Typical steps:

- **Bitwig Studio:** open the device's *Chains / outputs* panel (the device's output-chain button) and add the outputs you need. Each appears as its own sub-channel.
- **REAPER:** open the plugin's **I/O** (routing) button, add outputs, and create a new track per output with *Build multichannel routing for output of selected tracks*, or use *Track routing → Audio hardware output*.
- **Ableton Live (VST3):** on another audio track, set *Audio From* to the Mutadrums track and pick the output (*Out 2*, *Out 3*, …) in the second menu.
- **FL Studio:** in the plugin's wrapper settings (*Processing* tab), give each output its own mixer track.
- **Cubase / Studio One / Logic:** activate the extra outputs from the instrument's output menu, or use the multi-output instrument option.

Menu names vary between versions; search your DAW's manual for "multi-output instrument" if they differ.

## Example: kick and snare on their own channels

1. Load the **Init Kit** preset.
2. Set voice 1 (Kick) **Output** to **Out 2** and voice 2 (Snare) to **Out 3**.
3. Enable Out 2 and Out 3 in your DAW as described above.
4. The kick and snare now appear on their own DAW channels. Everything else, and the effect returns, stays on Main.
