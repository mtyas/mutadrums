<p align="center">
  <img src="docs/images/banner.svg" alt="Mutadrums — generative drum machine by mtyas" width="100%">
</p>

<p align="center">
  <a href="LICENSE"><img alt="Licence: GPL v3" src="https://img.shields.io/badge/licence-GPL--3.0-ffb547?style=flat-square"></a>
  <img alt="Formats: CLAP, VST3, Standalone" src="https://img.shields.io/badge/formats-CLAP%20%7C%20VST3%20%7C%20Standalone-3fd5b6?style=flat-square">
  <img alt="Platforms: Windows, macOS, Linux" src="https://img.shields.io/badge/platforms-Windows%20%7C%20macOS%20%7C%20Linux-4dabf7?style=flat-square">
  <a href="https://github.com/mtyas/mutadrums/actions/workflows/build.yml"><img alt="Build status" src="https://img.shields.io/github/actions/workflow/status/mtyas/mutadrums/build.yml?branch=main&style=flat-square&label=build"></a>
  <img alt="Built with JUCE" src="https://img.shields.io/badge/built%20with-JUCE%207-9d7bff?style=flat-square">
</p>

<p align="center">
  <b>Eight drum voices, two generative sequencers, a channel strip per voice, and no two bars quite the same.</b><br>
  A love letter to Émilie Gillet's Grids, Marbles, Plaits and Braids, in plugin form.
</p>

<p align="center">
  <img src="docs/images/voice-page.png" alt="Mutadrums, VOICE page" width="100%">
</p>

---

## ✨ Features

|  |  |
|---|---|
| 🗺️ **Topo patterns** | Drag across a topographic map of drum grooves, or switch to Euclidean rhythms. Density, chaos and swing for each lane, and a 32-step preview of what will play. |
| 🎲 **Chance randomness** | Three gate streams and three random voltages, with Déjà-Vu looping. Use them to trigger voices or to push the Topo map around. |
| 🥁 **12 drum engines** | 808 and 909-style kicks and snares, hats, a modal resonator, a clap, a pulse kick, a resonant snare, a metal cymbal and an additive bell. |
| 🎛️ **Channel strip per voice** | 3-band EQ you can **drag directly on the curve**, Tape/Tube/Clip drive, compressor, pan, sends, mute and solo. |
| 🔀 **Separate outputs** | Main plus **7 extra stereo outs**. Send any voice to its own DAW mixer channel. |
| 🌊 **Per-hit modulation** | Three slots per voice: Chance, velocity, random, LFO or bar ramp, sampled fresh on every hit. |
| 🌀 **Effects** | A stereo/ping-pong tape delay and an 8-line FDN reverb with a real RT60. |
| ↩️ **Everything automatable** | About 390 host parameters, sample-accurate MIDI, and undo/redo for everything. |
| 🧰 **Init Kit** | A plain starting point: eight raw sounds with no processing. |

<p align="center">
  <img src="docs/images/mixer-page.png" alt="Mutadrums, MIXER page with per-channel output selectors" width="100%">
</p>

## 🚀 Quick start

1. **Get it.** Download the zip for Windows, macOS (Apple Silicon and Intel) or Linux from [Releases](https://github.com/mtyas/mutadrums/releases). The latest development builds are under [Actions](https://github.com/mtyas/mutadrums/actions/workflows/build.yml): open the newest green run and scroll to *Artifacts*. You can also [build it yourself](docs/building.md).
2. **Install** it by copying the plugin into your plugin folder, then rescan plugins in your DAW:

   | | CLAP | VST3 |
   |---|---|---|
   | Windows | `C:\Program Files\Common Files\CLAP\` | `C:\Program Files\Common Files\VST3\` |
   | macOS | `~/Library/Audio/Plug-Ins/CLAP/` | `~/Library/Audio/Plug-Ins/VST3/` |
   | Linux | `~/.clap/` | `~/.vst3/` |

   The macOS builds aren't notarised. If macOS says the plugin is damaged, see `INSTALL.txt` in the zip for the one-line fix.
3. **Play.** Load Mutadrums on an instrument track and press play. With **HOST SYNC** on it follows your DAW's tempo and transport. Kick, snare and hats follow Topo straight away. Drag the map and listen to the groove morph.
4. **Explore.** Click a pad to select and hear a voice, shape it on the VOICE page, or open **About** inside the plugin for a tour.

## 📚 Documentation

| Guide | What's inside |
|---|---|
| [User guide](docs/user-guide.md) | Every section of the interface, explained |
| [Separate outputs](docs/separate-outputs.md) | Routing voices to their own DAW channels |
| [Presets](docs/presets.md) | Factory and user presets, file locations and format |
| [Building from source](docs/building.md) | Clone, configure and build on Windows and macOS |
| [Credits](docs/credits.md) | Émilie Gillet, Mutable Instruments, and the libraries Mutadrums uses |
| [Changelog](CHANGELOG.md) | What changed in each version |

## 💛 Credits

Mutadrums exists because **Émilie Gillet** designed the [Mutable Instruments](https://pichenettes.github.io/mutable-instruments-documentation/) Eurorack modules and published [their source code](https://github.com/pichenettes/eurorack). The Grids drum maps, the Marbles random sampler, and the Plaits and Braids drum models are her work; this plugin adapts them for the DAW. Thank you, Émilie. Full credits are in [docs/credits.md](docs/credits.md).

<sub>Mutable Instruments is a registered trademark. Mutadrums is an independent project and is not affiliated with or endorsed by Mutable Instruments or Émilie Gillet. Module names are used only to credit where the ideas come from.</sub>

## ⚖️ Licence

Mutadrums is free software under the **[GNU General Public License v3.0 or later](LICENSE)**. You may use, study, share and modify it. If you distribute it, modified or not, you must share the source under the same licence. The Grids pattern data it includes is © Émilie Gillet under GPL-3.0. See [docs/credits.md](docs/credits.md) for every third-party licence.

<p align="center"><sub>Made by <a href="https://github.com/mtyas">mtyas</a></sub></p>
