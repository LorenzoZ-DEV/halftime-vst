cmake_minimum_required(VERSION 3.22)
project(Halftime VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(HALFTIME_TESTS "Build DSP core tests" ON)
option(HALFTIME_PLUGIN "Fetch JUCE and build the VST3 plugin" ON)

if(HALFTIME_TESTS)
    enable_testing()
    add_executable(halftime_tests tests/test_core.cpp)
    add_test(NAME halftime_tests COMMAND halftime_tests)
endif()

if(HALFTIME_PLUGIN)
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(juce)

    juce_add_plugin(Halftime
        COMPANY_NAME "Lorenzzzz"
        PLUGIN_MANUFACTURER_CODE Lrnz
        PLUGIN_CODE Hlft
        FORMATS VST3
        PRODUCT_NAME "Halftime"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        VST3_CATEGORIES Fx)

    target_sources(Halftime PRIVATE Source/HalftimeProcessor.cpp)
    target_compile_definitions(Halftime PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(Halftime
        PRIVATE juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endif()
