cmake_minimum_required(VERSION 3.21)

project(MutableDrums VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add JUCE and CLAP extensions
add_subdirectory(juce)
add_subdirectory(clap-juce-extensions EXCLUDE_FROM_ALL)

juce_add_plugin(MutableDrums
    COMPANY_NAME "MutableDSP"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE MDsp
    PLUGIN_CODE MDrm
    FORMATS VST3 Standalone
    PRODUCT_NAME "MutableDrums"
)

# Embedded UI font (Inter, SIL Open Font License)
juce_add_binary_data(MutableDrumsData SOURCES
    resources/fonts/Inter-Regular.ttf
    resources/fonts/Inter-SemiBold.ttf
)

target_sources(MutableDrums PRIVATE
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
)

target_include_directories(MutableDrums PRIVATE
    src
    src/dsp
    src/presets
    src/ui
)

target_compile_definitions(MutableDrums PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MutableDrums PRIVATE
    MutableDrumsData
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_graphics
    juce::juce_core
)

# Build CLAP target alongside VST3 and Standalone
clap_juce_extensions_plugin(TARGET MutableDrums
    CLAP_ID "org.mutabledsp.mutabledrums"
    CLAP_FEATURES instrument drum-machine drum
)
