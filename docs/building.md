# Building from source

[← Back to README](../README.md)

## Requirements

| | Windows | macOS |
|---|---|---|
| Compiler | Visual Studio 2022 or newer, with *Desktop development with C++* | Xcode 14 or newer |
| CMake | 3.21 or newer | 3.21 or newer |
| Git | any recent version | any recent version |

> Every push to `main` is built for Windows, macOS (universal) and Linux by [GitHub Actions](../.github/workflows/build.yml); see that file for the exact steps on each platform.
>
> **Linux** needs these development packages (Debian/Ubuntu names): `libasound2-dev libjack-jackd2-dev ladspa-sdk libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev mesa-common-dev ninja-build`.

## Get the source

JUCE and clap-juce-extensions are git submodules:

```bash
git clone https://github.com/mtyas/mutadrums.git
cd mutadrums
git submodule update --init --recursive juce clap-juce-extensions
```

The third submodule, `mi-eurorack`, is Émilie Gillet's original Mutable Instruments source, kept for reference. The build doesn't need it, so skip it unless you want to read the originals (`git submodule update --init mi-eurorack`).

## Configure and build

```bash
cmake -B build
cmake --build build --config Release
```

This builds every format. To build just one, add a target:

| Target | Output |
|---|---|
| `Mutadrums_CLAP` | `build/Mutadrums_artefacts/Release/CLAP/Mutadrums.clap` |
| `Mutadrums_VST3` | `build/Mutadrums_artefacts/Release/VST3/Mutadrums.vst3` |
| `Mutadrums_Standalone` | `build/Mutadrums_artefacts/Release/Standalone/Mutadrums.exe` (`.app` on macOS) |

```bash
cmake --build build --config Release --target Mutadrums_CLAP
```

On macOS, add `-G Xcode` to the configure step if you prefer an Xcode project, and `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` for a universal binary. With Xcode 16 or newer, also set `MACOSX_DEPLOYMENT_TARGET=11.0` in the environment before configuring (JUCE 7 needs it to build its helper tool).

## Install

| Format | Windows | macOS |
|---|---|---|
| CLAP | `C:\Program Files\Common Files\CLAP\` or `%LOCALAPPDATA%\Programs\Common\CLAP\` | `~/Library/Audio/Plug-Ins/CLAP/` |
| VST3 | `C:\Program Files\Common Files\VST3\` | `~/Library/Audio/Plug-Ins/VST3/` |

Close any DAW session using the plugin before replacing the file (Windows locks loaded plugins), then rescan plugins.

## Project layout

```
src/
  PluginProcessor.*   audio engine, parameters, routing, state
  PluginEditor.*      main window
  Params.h            every host parameter
  dsp/                sequencers, drum engines, channel strip, effects
  presets/            factory presets and the preset manager
  ui/                 pages, widgets, theme, About page
resources/fonts/      embedded Inter typeface
docs/                 documentation and images
```

The UI is drawn at a fixed design size (1240 × 800) and scaled as a whole when the window is resized.
