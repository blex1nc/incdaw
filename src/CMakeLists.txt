# ──────────────────────────────────────────────────────────────────────────────
# Layer libraries.
#
# Dependencies point DOWNWARD only. The allowed link order is:
#
#     ui  ->  app  ->  project  ->  engine  ->  platform
#                                      \-----> plugins -> platform
#
# tools/check_layering.py enforces this statically over the source tree and
# fails the build if it is violated. See docs/ARCHITECTURE.md §2.
# ──────────────────────────────────────────────────────────────────────────────

# ── platform/ : the ONLY layer permitted to contain OS-specific code ──────────
add_library(incdaw_platform STATIC
    platform/SystemInfo.cpp
    platform/macos/CoreAudioDevice.cpp
    platform/macos/CoreMidiDevice.cpp
    platform/macos/HostTime.cpp)
target_include_directories(incdaw_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_platform PRIVATE incdaw_warnings)

if(APPLE)
    target_link_libraries(incdaw_platform PUBLIC
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework CoreFoundation")
endif()

# ── engine/ : realtime core. Knows nothing above it, no OS symbols ────────────
add_library(incdaw_engine STATIC
    engine/core/Time.cpp
    engine/core/AudioBufferPool.cpp
    engine/core/CallbackProfiler.cpp
    engine/core/RealtimeGuard.cpp
    engine/graph/RenderGraph.cpp
    engine/transport/TempoMap.cpp
    engine/transport/Transport.cpp
    engine/dsp/SineOscillatorNode.cpp
    engine/dsp/GainNode.cpp
    engine/dsp/ChannelStripNode.cpp
    engine/dsp/MetronomeNode.cpp
    engine/midi/MidiInput.cpp
    engine/midi/MidiRecorder.cpp
    engine/midi/NoteSequence.cpp
    engine/instrument/Instrument.cpp
    engine/instrument/SimpleSynth.cpp
    engine/instrument/InstrumentNode.cpp
    engine/AudioEngine.cpp)
target_include_directories(incdaw_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_engine
    PUBLIC  incdaw_platform
    PRIVATE incdaw_warnings)

if(INCDAW_REALTIME_GUARD)
    target_compile_definitions(incdaw_engine PUBLIC INCDAW_REALTIME_GUARD=1)
endif()

# ── plugins/ : plugin hosting (Phase 13) ──────────────────────────────────────
add_library(incdaw_plugins STATIC
    plugins/PluginIdentifier.cpp)
target_include_directories(incdaw_plugins PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_plugins
    PUBLIC  incdaw_platform
    PRIVATE incdaw_warnings)

# ── project/ : data model, serialization, versioning ──────────────────────────
add_library(incdaw_project STATIC
    project/Identity.cpp
    project/Json.cpp
    project/Model.cpp
    project/ProjectFile.cpp
    project/MidiCapture.cpp
    project/PatternCompiler.cpp
    project/InstrumentFactory.cpp
    project/GraphCompiler.cpp)
target_include_directories(incdaw_project PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_project
    PUBLIC  incdaw_engine incdaw_plugins
    PRIVATE incdaw_warnings)

# ── app/ : command registry, undo, session ────────────────────────────────────
add_library(incdaw_app STATIC
    app/Version.cpp
    app/CommandRegistry.cpp
    app/PianoRollModel.cpp
    app/StepSequencerModel.cpp
    app/commands/NoteCommands.cpp
    app/commands/PatternCommands.cpp)
target_include_directories(incdaw_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_app
    PUBLIC  incdaw_project
    PRIVATE incdaw_warnings)

# ── ui/ : native shell. May use OS APIs; nothing depends on it ────────────────
if(APPLE)
    add_executable(INCDAW MACOSX_BUNDLE
        ui/macos/main.mm
        ui/macos/PianoRollView.mm
        ui/macos/ChannelRackView.mm
        ui/macos/PianoRollRenderer.mm)
    target_link_libraries(INCDAW
        PRIVATE incdaw_app incdaw_warnings
                "-framework Cocoa"
                "-framework Metal"
                "-framework QuartzCore")
    # ARC for the Objective-C++ shell. The C++ core is manually managed and
    # unaffected; only the UI layer holds Objective-C objects.
    target_compile_options(INCDAW PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)

    set_target_properties(INCDAW PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME        "INCDAW"
        MACOSX_BUNDLE_BUNDLE_VERSION     "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER     "com.incdaw.INCDAW"
        MACOSX_BUNDLE_COPYRIGHT          "INCDAW"
        MACOSX_BUNDLE_INFO_PLIST         "${CMAKE_CURRENT_SOURCE_DIR}/ui/macos/Info.plist.in")
endif()
