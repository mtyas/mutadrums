cmake_minimum_required(VERSION 3.21)

project(Mutadrums VERSION 2.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add JUCE and CLAP extensions
add_subdirectory(juce)
add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)

juce_add_plugin(Mutadrums
    COMPANY_NAME "mtyas"
    COMPANY_WEBSITE "https://github.com/mtyas/mutadrums"
    BUNDLE_ID "com.mtyas.mutadrums"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Mtys
    PLUGIN_CODE Mtdr
    FORMATS VST3 Standalone
    PRODUCT_NAME "Mutadrums"
    DESCRIPTION "Generative 8-voice drum machine"
    VST3_CATEGORIES Instrument Drum
)

# Embedded UI font (Inter, SIL Open Font License)
juce_add_binary_data(MutadrumsData SOURCES
    resources/fonts/Inter-Regular.ttf
    resources/fonts/Inter-SemiBold.ttf
)

target_sources(Mutadrums PRIVATE
    src/Params.h
    src/PluginProcessor.h
    src/PluginProcessor.cpp
    src/PluginEditor.h
    src/PluginEditor.cpp
    src/dsp/GridsResources.h
    src/dsp/GridsSequencer.h
    src/dsp/MarblesSequencer.h
    src/dsp/PlaitsDrums.h
    src/dsp/BraidsDrums.h
    src/dsp/VoiceEngine.h
    src/dsp/ChannelStrip.h
    src/dsp/FxDelay.h
    src/dsp/FxReverb.h
    src/presets/FactoryPresets.h
    src/presets/PresetManager.h
    src/ui/Theme.h
    src/ui/Widgets.h
    src/ui/HeaderBar.h
    src/ui/PatternPanel.h
    src/ui/VoicePads.h
    src/ui/VoicePage.h
    src/ui/MixerPage.h
    src/ui/FxPage.h
    src/ui/PresetBrowser.h
    src/ui/AboutPage.h
)

target_include_directories(Mutadrums PRIVATE
    src
    src/dsp
    src/presets
    src/ui
)

target_compile_definitions(Mutadrums PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Mutadrums PRIVATE
    MutadrumsData
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_core
)

# Build CLAP target alongside VST3 and Standalone
clap_juce_extensions_plugin(TARGET Mutadrums
    CLAP_ID "com.mtyas.mutadrums"
    CLAP_FEATURES instrument drum-machine drum
)
