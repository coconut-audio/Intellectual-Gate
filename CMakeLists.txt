cmake_minimum_required(VERSION 3.22)

project(INTELLECTUAL_GATE VERSION 0.0.1)

set(PROJECT_NAME Intellectual.Gate)
set(CURRENT_VERSION 0.0.1)
set(CURRENT_PLUGIN_CODE INTL)

message(STATUS "Plugin Name: ${PROJECT_NAME}")
message(STATUS "Version: ${CURRENT_VERSION}")
message(STATUS "Plugin Code: ${CURRENT_PLUGIN_CODE}")

# platform specific settings (must be before subdirectories)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version" FORCE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

# set JUCE path
add_subdirectory(modules/JUCE)

# set Google Test path
add_subdirectory(modules/googletest)

# set plugin formats
set(FORMATS VST3 Standalone)

# JUCE specific settings
juce_add_plugin(${PROJECT_NAME}
    VERSION ${CURRENT_VERSION}
    COMPANY_NAME "CoconutAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE COAU
    FORMATS "${FORMATS}"
    PRODUCT_NAME "${PROJECT_NAME}"
    PLUGIN_CODE "${CURRENT_PLUGIN_CODE}")

# add source files
target_sources(${PROJECT_NAME}
    PRIVATE
        "source/Processor.cpp"
        "source/Editor.cpp"
        "source/ui/LevelMeter.cpp"
        "source/ui/SpectrumAnalyser.cpp"
        "source/dsp/Compressor.cpp"
        "source/dsp/HighpassFilter.cpp")

# generate JUCE header
juce_generate_juce_header(${PROJECT_NAME})

# set C++ standard
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

# link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# set compile definitions
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

# test executable
enable_testing()
add_executable(Tests
    tests/Processor_test.cpp
)
target_include_directories(Tests
    PRIVATE
        ${CMAKE_SOURCE_DIR}/source
        ${CMAKE_BINARY_DIR}/${PROJECT_NAME}_artefacts/JuceLibraryCode
)
target_link_libraries(Tests
    PRIVATE
        GTest::gtest_main
        ${PROJECT_NAME}
        juce::juce_core
        juce::juce_audio_utils
        juce::juce_dsp
)
target_compile_features(Tests PRIVATE cxx_std_17)
include(GoogleTest)
gtest_discover_tests(Tests)
