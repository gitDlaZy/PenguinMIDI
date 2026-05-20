cmake_minimum_required(VERSION 3.22)
project(PenguinLFO VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)
juce_set_vst2_sdk_path(${CMAKE_CURRENT_SOURCE_DIR}/third_party/VST2_SDK)

juce_add_plugin(PenguinLFO
    COMPANY_NAME "DlaZy"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Dlzy
    PLUGIN_CODE Plfo
    FORMATS VST
    PRODUCT_NAME "PenguinLFO"
    VST2_CATEGORY kPlugCategEffect
)

juce_generate_juce_header(PenguinLFO)

target_sources(PenguinLFO PRIVATE
    Source/LFOEngine.cpp
    Source/PresetManager.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/WaveformVisualizer.cpp
)

target_compile_definitions(PenguinLFO PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_include_directories(PenguinLFO PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(PenguinLFO PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

enable_testing()
add_subdirectory(tests)
