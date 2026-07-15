cmake_minimum_required(VERSION 3.22)

project(VoxShred VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(VoxShred
    COMPANY_NAME                "Malus Audio"
    BUNDLE_ID                   "audio.malus.voxshred"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    PLUGIN_MANUFACTURER_CODE    Malu
    PLUGIN_CODE                 Vxsh
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "VOXSHRED")

juce_generate_juce_header(VoxShred)

target_sources(VoxShred
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/FormantShifter.cpp
        Source/dsp/TempoSyncedLFO.cpp
        Source/dsp/Saturator.cpp
        Source/gui/VoxShredLookAndFeel.cpp
        Source/gui/GlowKnob.cpp
        Source/gui/FormantCurveDisplay.cpp
        Source/gui/ModRateDisplay.cpp)

target_compile_definitions(VoxShred
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(VoxShred
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
