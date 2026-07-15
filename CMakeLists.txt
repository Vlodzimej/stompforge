cmake_minimum_required(VERSION 3.22)
project(StompForge VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(StompForge
    COMPANY_NAME "StompForge Audio"
    BUNDLE_ID "audio.stompforge.plugin"
    PLUGIN_MANUFACTURER_CODE Sfau
    PLUGIN_CODE Stmp
    FORMATS VST3 Standalone
    PRODUCT_NAME "StompForge"
    VST3_CATEGORIES Fx Distortion)

target_sources(StompForge PRIVATE
    Source/Effects.cpp
    Source/Effects.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(StompForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(StompForge PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
