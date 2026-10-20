# Credits

[← Back to README](../README.md)

## Émilie Gillet and Mutable Instruments

Mutadrums is built on the work of **Émilie Gillet**, who designed the Mutable Instruments Eurorack modules and released their source code ([pichenettes/eurorack](https://github.com/pichenettes/eurorack)) so others could learn from them and build on them. This plugin would not exist without that generosity.

| Module | Year | What Mutadrums takes from it |
|---|---|---|
| **Grids** | 2012 | The topographic drum sequencer: the 5×5 map of drum patterns, the interpolation between them, and the Euclidean tables. The pattern data is Émilie's, taken from the Grids firmware. |
| **Marbles** | 2018 | The random sampler: t gate generators (coin toss, polyrhythm, drums), X random voltages, Déjà-Vu looping. |
| **Plaits** | 2016 | The 808 and 909-style kick and snare models, the hi-hat model, the modal resonator and the clap. |
| **Braids** | 2012 | The Braids percussion voices: kick, snare, cymbal and bell. |

The ideas, the sound and the musical thinking behind these modules are Émilie's; any mistakes in this adaptation are mine. **Thank you, Émilie.**

> Mutable Instruments is a registered trademark. Mutadrums is an independent project by mtyas. It is not affiliated with or endorsed by Mutable Instruments or Émilie Gillet. Module names are used only to credit where the ideas come from.

## Libraries and assets

| Project | Use | Licence |
|---|---|---|
| [JUCE 7](https://juce.com) | Plugin framework, UI, VST3 and Standalone wrappers | GPLv3 (used under the GPL option) |
| [CLAP](https://github.com/free-audio/clap) | CLAP plugin standard | MIT |
| [clap-juce-extensions](https://github.com/free-audio/clap-juce-extensions) | CLAP wrapper for JUCE | MIT |
| [Inter](https://rsms.me/inter/) by Rasmus Andersson | UI typeface (embedded) | SIL Open Font License 1.1 |
| VST is a trademark of Steinberg Media Technologies GmbH | | |

## Licences of Mutable Instruments code

| Code | Licence |
|---|---|
| Grids (AVR firmware) | GPL-3.0-or-later |
| Marbles, Plaits, Braids (STM32 firmware) | MIT |

Because Mutadrums includes the Grids pattern data, the whole plugin is released under **GPL-3.0-or-later**. See [LICENSE](../LICENSE).

## Author

Mutadrums is made by **mtyas**: [github.com/mtyas](https://github.com/mtyas).
