cmake_minimum_required(VERSION 3.28...3.30)

# Include OBS plugin template bootstrap (handles OBS SDK download/build)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version} LANGUAGES C CXX)

# We need the frontend API for replay buffer control
option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

# Find Vosk (our custom module)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(Vosk REQUIRED)

# Create the plugin shared library
add_library(${CMAKE_PROJECT_NAME} MODULE)

# Find and link OBS
find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

# Link frontend API (required for replay buffer control)
find_package(obs-frontend-api REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)

# Find Qt6 for settings dialog UI
find_package(Qt6 REQUIRED COMPONENTS Widgets Core)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core)

# Enable Qt MOC for automatic meta-object processing
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON)

# Plugin sources
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    src/plugin-main.c
    src/voice-recognition/vosk-engine.c
    src/voice-recognition/phrase-detector.c
    src/audio-capture/wasapi-capture.c
    src/audio-capture/device-enum.c
    src/replay-control/replay-buffer.c
    src/settings/plugin-settings.c
    src/settings/properties-ui.c
    src/settings/settings-dialog.cpp
)

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${VOSK_INCLUDE_DIR}
)

# Link Vosk and Windows libraries
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    ${VOSK_LIBRARY}
    ole32
    oleaut32
    uuid
    ksuser
    mmdevapi
    avrt
)

# Add version definition
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    PLUGIN_VERSION="${PROJECT_VERSION}"
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        WIN32
        _WIN32
        UNICODE
        _UNICODE
    )

    # Copy all Vosk DLLs to output directory after build
    if(VOSK_ALL_DLLS)
        foreach(DLL ${VOSK_ALL_DLLS})
            add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL}"
                "$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>"
                COMMENT "Copying ${DLL} to output directory"
            )
        endforeach()
    endif()
endif()

# Use the plugin template's target properties helper
set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# Install all Vosk DLLs alongside the plugin
if(WIN32 AND VOSK_ALL_DLLS)
    install(FILES ${VOSK_ALL_DLLS} DESTINATION "obs-plugins/64bit")
endif()

# Install Vosk models (if present)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data/models")
    install(DIRECTORY data/models DESTINATION "data/obs-plugins/${_name}")
endif()
