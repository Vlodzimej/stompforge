cmake_minimum_required(VERSION 3.22)
project(StompForge VERSION 0.0.1 LANGUAGES C CXX)
if (POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(ASIO_SDK_PATH "" CACHE PATH "Path to the Steinberg ASIO SDK containing common/iasiodrv.h")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/PlatformConfig.cmake)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# NeuralAmpModelerPlugin is an iPlug2 application, so StompForge uses it as the
# integration reference and links only the framework-independent MIT core.
FetchContent_Declare(NAMCoreBundled
    GIT_REPOSITORY https://github.com/sdatkinson/NeuralAmpModelerCore.git
    GIT_TAG v0.5.4
    GIT_SHALLOW TRUE
    GIT_SUBMODULES Dependencies/eigen Dependencies/AudioDSPTools)
FetchContent_GetProperties(NAMCoreBundled)
if (NOT namcorebundled_POPULATED)
    FetchContent_Populate(NAMCoreBundled)
endif()

# An object library is intentional: NAM architecture parsers self-register via
# translation-unit statics, which a conventional static linker may discard.
add_library(StompForgeNAMCore OBJECT
    ${namcorebundled_SOURCE_DIR}/NAM/activations.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/container.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/conv1d.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/convnet.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/dsp.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/get_dsp.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/linear.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/lstm.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/ring_buffer.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/util.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/wavenet/a2_fast.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/wavenet/model.cpp
    ${namcorebundled_SOURCE_DIR}/NAM/wavenet/slimmable.cpp)
target_include_directories(StompForgeNAMCore SYSTEM PUBLIC
    ${namcorebundled_SOURCE_DIR}
    ${namcorebundled_SOURCE_DIR}/NAM
    ${namcorebundled_SOURCE_DIR}/Dependencies/nlohmann
    ${namcorebundled_SOURCE_DIR}/Dependencies/eigen
    ${namcorebundled_SOURCE_DIR}/Dependencies/AudioDSPTools)
target_compile_definitions(StompForgeNAMCore PUBLIC NAM_SAMPLE_FLOAT=1 NAM_ENABLE_A2_FAST=1 NOMINMAX)
target_compile_features(StompForgeNAMCore PUBLIC cxx_std_20)
set_target_properties(StompForgeNAMCore PROPERTIES POSITION_INDEPENDENT_CODE ON)

juce_add_plugin(StompForge
    COMPANY_NAME "Vlodzimej Garlic"
    COMPANY_COPYRIGHT "Copyright (c) 2026 Vlodzimej Garlic"
    BUNDLE_ID "${STOMPFORGE_BUNDLE_ID}"
    PLUGIN_MANUFACTURER_CODE Sfau
    PLUGIN_CODE Stmp
    FORMATS ${STOMPFORGE_PLUGIN_FORMATS}
    PRODUCT_NAME "StompForge"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/stompforge-icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/stompforge-icon.png"
    VST3_CATEGORIES Fx Distortion
    ${STOMPFORGE_PLATFORM_OPTIONS})

target_sources(StompForge PRIVATE
    Source/AssetRepository.cpp
    Source/AssetRepository.h
    Source/Effects.cpp
    Source/Effects.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h)

target_compile_definitions(StompForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    STOMPFORGE_APP_GROUP_ID="${STOMPFORGE_IOS_APP_GROUP_ID}")

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
    StompForgeNAMCore
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)

# NAM model architectures register themselves from static initialisers. The
# JUCE shared-code target is itself a static archive, so format linkers must
# retain every object from it or parser-only translation units disappear.
foreach(format_target StompForge_VST3 StompForge_Standalone StompForge_AUv3)
    if (TARGET ${format_target})
        if (MSVC)
            target_link_options(${format_target} PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:StompForge>")
        elseif(APPLE)
            target_link_options(${format_target} PRIVATE "SHELL:-Wl,-force_load,$<TARGET_FILE:StompForge>")
        else()
            target_link_options(${format_target} PRIVATE
                "SHELL:-Wl,--whole-archive $<TARGET_FILE:StompForge> -Wl,--no-whole-archive")
        endif()
    endif()
endforeach()

option(STOMPFORGE_BUILD_TESTS "Build offline DSP smoke tests" ON)
if (STOMPFORGE_BUILD_TESTS)
    add_executable(StompForgeDspSmokeTest Tests/DspSmokeTest.cpp Source/Effects.cpp Source/Effects.h)
    target_include_directories(StompForgeDspSmokeTest PRIVATE Source)
    target_compile_definitions(StompForgeDspSmokeTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
        NAM_TEST_MODEL_PATH="${namcorebundled_SOURCE_DIR}/example_models/A2.nam")
    target_link_libraries(StompForgeDspSmokeTest PRIVATE StompForgeNAMCore juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endif()
