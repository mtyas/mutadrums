# Contributing to Mutadrums

Thanks for your interest! Bug reports, presets and pull requests are all welcome.

## Reporting a bug

Please [open an issue](https://github.com/mtyas/mutadrums/issues) with:

- your OS, DAW and version, and the plugin format (CLAP, VST3 or Standalone),
- what you did, what you expected, and what happened instead,
- a preset file or a short screen recording if it helps.

## Sharing presets

User presets are plain JSON files (see [docs/presets.md](docs/presets.md)). Attach them to an issue, or open a pull request that adds them to `src/presets/FactoryPresets.h`.

## Pull requests

1. Fork the repository and create a branch from `main`.
2. Build with `cmake -B build && cmake --build build --config Release` (see [docs/building.md](docs/building.md)) and check the plugin loads in a host.
3. Match the surrounding code style: C++17, 4-space indents, `camelCase_` members.
4. Keep the audio thread real-time safe: no allocation, locks or file access in `processBlock`.
5. Keep pull requests focused, and describe what changed and why.

By contributing you agree that your contribution is licensed under the project's licence, [GPL-3.0-or-later](LICENSE).
