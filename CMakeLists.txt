# ==============================================================================
#  Ambient Composer AI — VST3 / AU / Standalone MIDI generator plugin
#
#  Self-contained, CROSS-PLATFORM build: JUCE is fetched automatically via
#  CMake FetchContent. Works on macOS (Xcode/clang) and Windows (MSVC).
#
#  macOS:    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
#            (VST3 + AU auto-install to ~/Library/Audio/Plug-Ins/)
#  Windows:  cmake -S . -B build && cmake --build build --config Release
#            (Visual Studio is a multi-config generator, hence --config)
#
#  On Windows the MSVC runtime is linked STATICALLY so the built .exe/.vst3 run
#  on any machine with no Visual C++ redistributable to install.
# ==============================================================================
cmake_minimum_required(VERSION 3.24)

# Must be set before project() so JUCE and all targets agree on the min OS.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Statically link the MSVC C/C++ runtime (/MT) so distributed binaries have no
# external VC++ redistributable dependency. Must be set before targets.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

project(AmbientComposerAI VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# Per-platform plugin formats + auto-install.
#   * AU is macOS-only.
#   * COPY_PLUGIN_AFTER_BUILD installs to the user plug-in folder on macOS; on
#     Windows we package a proper installer instead (writing to Program Files
#     needs elevation), so we skip the post-build copy there.
# ------------------------------------------------------------------------------
if(APPLE)
    set(ACAI_FORMATS Standalone VST3 AU)
    set(ACAI_COPY_AFTER_BUILD TRUE)
else()
    set(ACAI_FORMATS Standalone VST3)
    set(ACAI_COPY_AFTER_BUILD FALSE)
endif()

# ------------------------------------------------------------------------------
# JUCE (latest stable 8.x, pinned for reproducible builds)
# ------------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.14
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ------------------------------------------------------------------------------
# Plugin target
# ------------------------------------------------------------------------------
juce_add_plugin(AmbientComposerAI
    COMPANY_NAME                "AmritAudio"
    BUNDLE_ID                   "com.amritaudio.ambientcomposerai"
    PLUGIN_MANUFACTURER_CODE    Amra                # 4 chars, mixed case (AU requirement)
    PLUGIN_CODE                 Acai
    FORMATS                     ${ACAI_FORMATS}
    PRODUCT_NAME                "Ambient Composer AI"

    # MIDI generator: registered as an instrument so hosts give it an audio bus
    # (needed for the built-in preview synth). It renders silence by default.
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE

    COPY_PLUGIN_AFTER_BUILD     ${ACAI_COPY_AFTER_BUILD}
    VST3_CAN_REPLACE_VST2       FALSE
)

# ------------------------------------------------------------------------------
# Shared engine/core sources (pure C++ + MidiExporter). Used by BOTH the plugin
# and the headless test app so the tests verify exactly what ships.
# ------------------------------------------------------------------------------
set(ACAI_ENGINE_SOURCES
    Source/Theory/Theory.cpp
    Source/Engines/ScaleEngine.cpp
    Source/Engines/MoodEngine.cpp
    Source/Engines/ChordEngine.cpp
    Source/Engines/MelodyEngine.cpp
    Source/Engines/Humanizer.cpp
    Source/Engines/CompositionValidator.cpp
    Source/Engines/PatternGenerator.cpp
    Source/IO/MidiExporter.cpp
)

# GUI + playback sources (JUCE UI/audio; plugin only).
set(ACAI_PLUGIN_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/GUI/LookAndFeelDark.cpp
    Source/GUI/PianoRollComponent.cpp
    Source/Playback/PreviewSynth.cpp
    Source/Playback/PreviewPlayer.cpp
    Source/IO/PatternState.cpp
)

target_sources(AmbientComposerAI PRIVATE
    ${ACAI_PLUGIN_SOURCES}
    ${ACAI_ENGINE_SOURCES}
)

target_include_directories(AmbientComposerAI PRIVATE Source)

target_compile_definitions(AmbientComposerAI PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(AmbientComposerAI
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# ------------------------------------------------------------------------------
# Headless engine test app — fast to build (no plugin wrappers), verifies the
# generation core offline. Build+run: cmake --build build --target AmbientComposerTests
# ------------------------------------------------------------------------------
juce_add_console_app(AmbientComposerTests PRODUCT_NAME "AmbientComposerTests")

target_sources(AmbientComposerTests PRIVATE
    Tests/EngineTests.cpp
    ${ACAI_ENGINE_SOURCES}
)

target_include_directories(AmbientComposerTests PRIVATE Source)

target_compile_definitions(AmbientComposerTests PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(AmbientComposerTests
    PRIVATE
        juce::juce_audio_basics
        juce::juce_core
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
