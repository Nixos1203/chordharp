cmake_minimum_required(VERSION 3.22)

# Doit être défini avant project()
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Version macOS minimale")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(ChordHarp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(CHORDHARP_RENDER_TEST "Compile l'outil de rendu audio hors-ligne (tests)" OFF)
set(JUCE_PATH "" CACHE PATH "Chemin vers un checkout local de JUCE (sinon téléchargé automatiquement)")

if(JUCE_PATH)
    add_subdirectory(${JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.15
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(CHORDHARP_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND CHORDHARP_FORMATS AU)
endif()

juce_add_plugin(ChordHarp
    COMPANY_NAME                "MIDISOUDRINK"
    BUNDLE_ID                   "com.midisoudrink.chordharp"
    PLUGIN_MANUFACTURER_CODE    Mdsk
    PLUGIN_CODE                 Chhp
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             Instrument Synth
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    FORMATS                     ${CHORDHARP_FORMATS}
    PRODUCT_NAME                "ChordHarp")

target_sources(ChordHarp PRIVATE
    Source/Engine.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(ChordHarp PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ChordHarp
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Outil de test : rend une démo en .wav sans DAW
if(CHORDHARP_RENDER_TEST)
    juce_add_console_app(ChordHarpRender PRODUCT_NAME "ChordHarpRender")
    target_sources(ChordHarpRender PRIVATE tests/Render.cpp Source/Engine.cpp)
    target_compile_definitions(ChordHarpRender PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(ChordHarpRender
        PRIVATE juce::juce_audio_formats juce::juce_dsp
        PUBLIC  juce::juce_recommended_config_flags)

    # Capture PNG de l'interface, sans fenêtre
    juce_add_console_app(ChordHarpSnapshot PRODUCT_NAME "ChordHarpSnapshot")
    target_sources(ChordHarpSnapshot PRIVATE tests/Snapshot.cpp
        Source/Engine.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(ChordHarpSnapshot PRIVATE
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 "JucePlugin_Name=\"ChordHarp\"")
    target_link_libraries(ChordHarpSnapshot
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC  juce::juce_recommended_config_flags)
endif()
