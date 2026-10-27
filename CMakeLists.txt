cmake_minimum_required(VERSION 3.22)

# Static MSVC runtime -> the plugin needs no VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

project(Voidbringer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(VB_BUILD_CLAP  "Build CLAP format (clap-juce-extensions)" ON)
option(VB_BUILD_TOOLS "Build the offline render / screenshot test" ON)

include(FetchContent)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/JUCE/CMakeLists.txt")
    add_subdirectory(external/JUCE)
else()
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(juce)
endif()

# voidbringer by sonicsystems: FM screech synth. The bundle ID and the codes are what FL Studio stores in projects:
# never change them once released. Parameter IDs are frozen too (Source/Params.h, add new ones at the END).
juce_add_plugin(Voidbringer
    COMPANY_NAME              "sonicsystems"
    PRODUCT_NAME              "voidbringer"
    PLUGIN_NAME               "voidbringer"
    BUNDLE_ID                 "com.sonicsystems.voidbringer"
    PLUGIN_MANUFACTURER_CODE  Sncs
    PLUGIN_CODE               Vdbr
    IS_SYNTH                  TRUE
    NEEDS_MIDI_INPUT          TRUE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD   FALSE
    VST3_CATEGORIES           Instrument Synth
    FORMATS                   VST3)

set(VB_SOURCES Source/Params.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/VoidUI.cpp)
set(VB_DEFS JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_REPORT_APP_USAGE=0
            JUCE_USE_MP3AUDIOFORMAT=1)

# JetBrains Mono (OFL, see fonts/JetBrainsMono-OFL.txt)
juce_add_binary_data(VoidFonts HEADER_NAME VoidFonts.h NAMESPACE VoidFonts SOURCES
    fonts/JetBrainsMonoRegular.ttf fonts/JetBrainsMonoSemiBold.ttf fonts/JetBrainsMonoExtraBold.ttf)
set_target_properties(VoidFonts PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_sources(Voidbringer PRIVATE ${VB_SOURCES})
target_include_directories(Voidbringer PRIVATE Source)
target_compile_definitions(Voidbringer PUBLIC ${VB_DEFS})
target_link_libraries(Voidbringer
    PRIVATE
        VoidFonts
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(VB_BUILD_CLAP)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/clap-juce-extensions/CMakeLists.txt")
        add_subdirectory(external/clap-juce-extensions EXCLUDE_FROM_ALL)
    else()
        FetchContent_Declare(clap-juce-extensions
            GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
            GIT_TAG        55525c9858d4b25687be7759a5e0f70eccef218e
            GIT_SUBMODULES_RECURSE TRUE)
        FetchContent_MakeAvailable(clap-juce-extensions)
    endif()
    clap_juce_extensions_plugin(TARGET Voidbringer
        CLAP_ID "com.sonicsystems.voidbringer"
        CLAP_FEATURES instrument synthesizer stereo)
endif()

# offline render + GUI screenshot: VoidbringerRender demo.wav gui.png -> must print RESULT: PASS
if(VB_BUILD_TOOLS)
    juce_add_console_app(VoidbringerRender PRODUCT_NAME "VoidbringerRender")
    target_sources(VoidbringerRender PRIVATE Tools/RenderDemo.cpp ${VB_SOURCES})
    target_include_directories(VoidbringerRender PRIVATE Source)
    target_compile_definitions(VoidbringerRender PRIVATE ${VB_DEFS} SST_STANDALONE_TOOL=1)
    target_link_libraries(VoidbringerRender
        PRIVATE
            VoidFonts
            juce::juce_audio_utils
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
