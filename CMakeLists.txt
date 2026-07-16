cmake_minimum_required(VERSION 3.22)
project(StompForge VERSION 0.0.1 LANGUAGES C CXX)

set(ASIO_SDK_PATH "" CACHE PATH "Path to the Steinberg ASIO SDK containing common/iasiodrv.h")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(StompForge
    COMPANY_NAME "Vlodzimej Garlic"
    COMPANY_COPYRIGHT "Copyright (c) 2026 Vlodzimej Garlic"
    BUNDLE_ID "audio.stompforge.plugin"
    PLUGIN_MANUFACTURER_CODE Sfau
    PLUGIN_CODE Stmp
    FORMATS VST3 Standalone
    PRODUCT_NAME "StompForge"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/stompforge-icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/stompforge-icon.png"
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

if (WIN32 AND ASIO_SDK_PATH)
    if (EXISTS "${ASIO_SDK_PATH}/common/iasiodrv.h")
        target_compile_definitions(StompForge PUBLIC JUCE_ASIO=1)
        target_include_directories(StompForge PRIVATE
            "${ASIO_SDK_PATH}/common"
            "${ASIO_SDK_PATH}/host"
            "${ASIO_SDK_PATH}/host/pc")
        message(STATUS "StompForge Standalone: ASIO enabled")
    else()
        message(FATAL_ERROR "ASIO_SDK_PATH does not contain common/iasiodrv.h: ${ASIO_SDK_PATH}")
    endif()
else()
    target_compile_definitions(StompForge PUBLIC JUCE_ASIO=0)
    if (WIN32)
        message(STATUS "StompForge Standalone: ASIO disabled (set ASIO_SDK_PATH to enable)")
    endif()
endif()

target_link_libraries(StompForge PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

option(STOMPFORGE_BUILD_TESTS "Build offline DSP smoke tests" ON)
if (STOMPFORGE_BUILD_TESTS)
    add_executable(StompForgeDspSmokeTest Tests/DspSmokeTest.cpp Source/Effects.cpp Source/Effects.h)
    target_include_directories(StompForgeDspSmokeTest PRIVATE Source)
    target_compile_definitions(StompForgeDspSmokeTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(StompForgeDspSmokeTest PRIVATE juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
