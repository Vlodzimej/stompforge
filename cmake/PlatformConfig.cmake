set(STOMPFORGE_IOS_BUNDLE_ID "audio.stompforge.app" CACHE STRING
    "Bundle identifier of the StompForge iOS containing app")
set(STOMPFORGE_IOS_APP_GROUP_ID "group.audio.stompforge.shared" CACHE STRING
    "App Group shared by the StompForge iOS app and AUv3 extension")

if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
        message(FATAL_ERROR "StompForge iOS builds require the Xcode generator")
    endif()

    set(CMAKE_XCODE_GENERATE_SCHEME TRUE)
    set(STOMPFORGE_BUNDLE_ID "${STOMPFORGE_IOS_BUNDLE_ID}")
    set(STOMPFORGE_PLUGIN_FORMATS Standalone AUv3)
    set(STOMPFORGE_PLATFORM_OPTIONS
        MICROPHONE_PERMISSION_ENABLED TRUE
        MICROPHONE_PERMISSION_TEXT
            "StompForge needs microphone access to process a connected guitar in the standalone app."
        FILE_SHARING_ENABLED TRUE
        BACKGROUND_AUDIO_ENABLED TRUE
        REQUIRES_FULL_SCREEN TRUE
        APP_GROUPS_ENABLED TRUE
        APP_GROUP_IDS "${STOMPFORGE_IOS_APP_GROUP_ID}"
        TARGETED_DEVICE_FAMILY "1,2"
        IPHONE_SCREEN_ORIENTATIONS
            UIInterfaceOrientationLandscapeLeft
            UIInterfaceOrientationLandscapeRight
        IPAD_SCREEN_ORIENTATIONS
            UIInterfaceOrientationLandscapeLeft
            UIInterfaceOrientationLandscapeRight)
else()
    set(STOMPFORGE_BUNDLE_ID "audio.stompforge.plugin")
    set(STOMPFORGE_PLUGIN_FORMATS VST3 Standalone)
    set(STOMPFORGE_PLATFORM_OPTIONS)
endif()
