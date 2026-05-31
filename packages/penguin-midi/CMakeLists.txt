cmake_minimum_required(VERSION 3.22)
project(PenguinMIDI VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Reuse JUCE and VST2 SDK from penguin-lfo — no duplication needed
add_subdirectory(../penguin-lfo/JUCE juce_build)
juce_set_vst2_sdk_path(${CMAKE_CURRENT_SOURCE_DIR}/../penguin-lfo/third_party/VST2_SDK)

juce_add_plugin(PenguinMIDI
    COMPANY_NAME            "DlaZy"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       TRUE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Dlzy
    PLUGIN_CODE             Pmid
    FORMATS                 VST VST3
    PRODUCT_NAME            "PenguinMIDI"
    VST2_CATEGORY           kPlugCategSynth
    NEEDS_WEBVIEW2          TRUE
)

juce_generate_juce_header(PenguinMIDI)

juce_add_binary_data(PenguinMIDIData SOURCES
    web/index.html
)

target_sources(PenguinMIDI PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(PenguinMIDI PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_WIN_WEBVIEW2=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(PenguinMIDI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../penguin-lfo/third_party
)

target_link_libraries(PenguinMIDI
    PRIVATE
        PenguinMIDIData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
