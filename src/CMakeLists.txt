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
    platform/SharedLibrary.cpp
    platform/ChildProcess.cpp
    platform/macos/CoreAudioDevice.cpp
    platform/macos/CoreMidiDevice.cpp
    platform/macos/HostTime.cpp
    platform/macos/HttpClient.mm
    platform/macos/AudioUnitHost.mm)
target_include_directories(incdaw_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_platform PRIVATE incdaw_warnings)

if(APPLE)
    target_link_libraries(incdaw_platform PUBLIC
        "-framework CoreAudio"
        "-framework CoreMIDI"
        "-framework AudioToolbox"
        "-framework CoreAudioKit"
        "-framework AppKit"
        "-framework Foundation"
        "-framework CoreFoundation")

    # ARC for the Objective-C++ files in this layer: the Audio Unit host holds
    # an AppKit view for the generic editor, and the HTTP client holds an
    # NSURLSession and a block. The C++ around them is unaffected — ARC is per
    # translation unit.
    set_source_files_properties(platform/macos/AudioUnitHost.mm
                                platform/macos/HttpClient.mm
        PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
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
    engine/dsp/DelayLineNode.cpp
    engine/dsp/Fft.cpp
    engine/dsp/Resampler.cpp
    engine/dsp/TimeStretch.cpp
    engine/dsp/MixerStripNode.cpp
    engine/dsp/effects/BuiltinEffect.cpp
    engine/dsp/effects/BuiltinEffects.cpp
    engine/dsp/effects/DynamicsEffects.cpp
    engine/dsp/effects/ModulationEffects.cpp
    engine/dsp/effects/SpaceEffects.cpp
    engine/dsp/effects/ToneEffects.cpp
    engine/dsp/effects/UtilityEffects.cpp
    # <<< incdaw:sources:engine-effects — one line per family (Agent 2)
    # >>> incdaw:sources:engine-effects
    engine/dsp/MetronomeNode.cpp
    engine/midi/MidiInput.cpp
    engine/midi/SmfFile.cpp
    engine/midi/MidiRecorder.cpp
    engine/midi/NoteSequence.cpp
    engine/instrument/Instrument.cpp
    engine/instrument/BuiltinInstruments.cpp
    engine/instrument/Sampler.cpp
    engine/instrument/SamplerStream.cpp
    engine/instrument/SimpleSynth.cpp
    engine/instrument/PianoInstrument.cpp
    engine/instrument/InstrumentNode.cpp
    # <<< incdaw:sources:engine-instruments — one line per instrument (Agent 3)
    # >>> incdaw:sources:engine-instruments
    engine/audio/AiffFile.cpp
    engine/audio/SampleCache.cpp
    engine/audio/WavFile.cpp
    engine/audio/WavStreamWriter.cpp
    engine/audio/WavStreamReader.cpp
    engine/audio/AudioStream.cpp
    engine/audio/AudioEdits.cpp
    engine/audio/WaveformOverview.cpp
    engine/audio/OnsetDetection.cpp
    engine/audio/AudioRecorder.cpp
    engine/audio/AudioLogger.cpp
    engine/audio/AuditionPlayer.cpp
    engine/audio/AudioClipNode.cpp
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
    plugins/PluginIdentifier.cpp
    plugins/PluginScan.cpp
    plugins/PluginRegistry.cpp
    plugins/PluginInstanceManager.cpp
    plugins/clap/ClapLibrary.cpp
    plugins/au/AudioUnitInstance.cpp)
target_include_directories(incdaw_plugins PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# The CLAP C ABI (third_party/clap, MIT, D-027). PUBLIC because the host's
# own headers carry clap types; the DISCIPLINE that nothing above plugins/
# includes a clap header is a design rule, enforced by review, not by the
# build — engine/ and project/ have no reason to and do not.
target_include_directories(incdaw_plugins PUBLIC ${CMAKE_SOURCE_DIR}/third_party/clap)
# plugins sits beside engine (same layer rank): PluginNode derives the
# graph's Node, so the link goes plugins -> engine -> platform.
target_link_libraries(incdaw_plugins
    PUBLIC  incdaw_engine
    PRIVATE incdaw_warnings)

# ── project/ : data model, serialization, versioning ──────────────────────────
add_library(incdaw_project STATIC
    project/Identity.cpp
    project/Json.cpp
    project/Model.cpp
    project/ProjectFile.cpp
    project/MidiCapture.cpp
    project/ParameterRegistry.cpp
    project/PatternCompiler.cpp
    project/PluginStateFiles.cpp
    project/MidiFile.cpp
    project/OfflineRender.cpp
    project/ProjectGraphCompiler.cpp
    project/InstrumentFactory.cpp
    project/instruments/CoreInstrumentFactory.cpp
    # <<< incdaw:sources:project-instruments — one line per family (Agent 3)
    # >>> incdaw:sources:project-instruments
    project/RecordingSession.cpp)
target_include_directories(incdaw_project PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_project
    PUBLIC  incdaw_engine
    PRIVATE incdaw_warnings)

# ── app/ : command registry, undo, session ────────────────────────────────────
add_library(incdaw_app STATIC
    app/Version.cpp
    app/AppSettings.cpp
    app/UpdateCheck.cpp
    app/StandardActions.cpp
    app/AutomationWriteSession.cpp
    app/Browser.cpp
    app/AudioAssetImport.cpp
    app/CommandRegistry.cpp
    app/ProjectSession.cpp
    app/MusicTheory.cpp
    app/PianoRollModel.cpp
    app/ChannelRackModel.cpp
    app/PluginPickerModel.cpp
    app/PlaylistModel.cpp
    app/devices/DeviceUiCatalogue.cpp
    app/devices/DeviceUiLayout.cpp
    app/devices/DeviceUiValue.cpp
    app/devices/TonePanels.cpp
    # <<< incdaw:sources:app-panels — one line per family (Agents 2 and 3)
    # >>> incdaw:sources:app-panels
    app/commands/NoteCommands.cpp
    app/commands/TempoCommands.cpp
    app/commands/ChordCommands.cpp
    app/commands/NoteToolCommands.cpp
    app/commands/ChannelCommands.cpp
    app/commands/MidiMappingCommands.cpp
    app/commands/SamplerCommands.cpp
    app/commands/ImportCommands.cpp
    app/commands/PatternCommands.cpp
    app/commands/StepCommands.cpp
    app/commands/TrackCommands.cpp
    app/commands/ClipCommands.cpp
    app/commands/MarkerCommands.cpp
    app/commands/MixerCommands.cpp
    app/commands/PluginCommands.cpp
    app/commands/AutomationCommands.cpp
    app/commands/RecordingCommands.cpp
    app/commands/AudioEditCommands.cpp
    app/commands/SlicerCommands.cpp)
target_include_directories(incdaw_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_app
    PUBLIC  incdaw_project
    PRIVATE incdaw_warnings)

# ── ui/ theme data : the palette and the themes folder, without any OS API ────
# Separate from the shell below because a theme is a file format and a set of
# rules about naming files, and both must be testable on any platform and
# without a window server. ui/macos/Theme.mm draws with it; nothing else does.
add_library(incdaw_theme STATIC
    ui/ThemePalette.cpp
    ui/ThemeLibrary.cpp)
target_include_directories(incdaw_theme PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_theme
    PUBLIC  incdaw_project
    PRIVATE incdaw_warnings)

# ── ui/ : native shell. May use OS APIs; nothing depends on it ────────────────
if(APPLE)
    add_executable(INCDAW MACOSX_BUNDLE
        ui/macos/main.mm
        ui/macos/Theme.mm
        ui/macos/ControlBarView.mm
        ui/macos/PianoRollView.mm
        ui/macos/PianoRollRenderer.mm
        ui/macos/PianoRollHeaderView.mm
        ui/macos/ChannelRackView.mm
        ui/macos/PatternListView.mm
        ui/macos/PlaylistView.mm
        ui/macos/MixerView.mm
        ui/macos/PluginPickerView.mm
        ui/macos/AudioEditorView.mm
        ui/macos/InsertParameterPanel.mm
        ui/macos/DevicePanel.mm
        ui/macos/SpectrumView.mm
        ui/macos/BrowserView.mm
        ui/macos/SettingsWindow.mm
        ui/macos/CommandPalette.mm)
    # incdaw_plugins is linked directly: the shell owns the plugin registry and
    # the instance manager, and injects the insert factory into the graph
    # compiler (docs/DECISIONS.md D-028). project/ deliberately does not link
    # it — the factory is how a hosted plugin reaches the graph.
    target_link_libraries(INCDAW
        PRIVATE incdaw_app incdaw_plugins incdaw_theme incdaw_warnings
                "-framework Cocoa"
                "-framework Metal"
                "-framework QuartzCore"
                "-framework UniformTypeIdentifiers")
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

    # The out-of-process scanner rides inside the bundle: scanning unknown
    # plugins must never happen in the DAW's own process (PLUGIN_HOST §3),
    # so the app needs the child binary wherever it is installed.
    add_dependencies(INCDAW incdaw-pluginscan)
    add_custom_command(TARGET INCDAW POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:incdaw-pluginscan>
                $<TARGET_BUNDLE_CONTENT_DIR:INCDAW>/MacOS/incdaw-pluginscan)
endif()
