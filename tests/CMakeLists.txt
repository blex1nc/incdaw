add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE ${CMAKE_SOURCE_DIR}/third_party/doctest)

add_executable(incdaw_tests
    main.cpp
    unit/TimeTests.cpp
    unit/IdentityTests.cpp
    unit/PluginIdentifierTests.cpp
    unit/SystemInfoTests.cpp
    unit/RealtimeSafetyTests.cpp
    unit/AudioBufferTests.cpp
    unit/RenderGraphTests.cpp
    unit/DspTests.cpp
    unit/TempoMapTests.cpp
    unit/TransportTests.cpp
    unit/ProjectFormatTests.cpp
    unit/MidiTests.cpp
    unit/MidiOutputTests.cpp
    unit/MidiClockTests.cpp
    unit/MidiClockReceiveTests.cpp
    unit/MidiFeedbackTests.cpp
    unit/DeviceWatcherTests.cpp
    unit/MpeTests.cpp
    unit/PreRecordTests.cpp
    unit/CompingTests.cpp
    unit/SpectralTests.cpp
    unit/CommandTests.cpp
    unit/ProjectSessionTests.cpp
    unit/StoppedTransportTests.cpp
    unit/TempoCommandTests.cpp
    unit/BrowserTests.cpp
    unit/BrowserDropTests.cpp
    unit/ChordToolTests.cpp
    unit/NoteToolTests.cpp
    unit/PianoRollTests.cpp
    unit/InstrumentTests.cpp
    unit/PianoInstrumentTests.cpp
    unit/PluginPickerTests.cpp
    unit/InstrumentParameterTests.cpp
    unit/BuiltinEffectTests.cpp
    unit/BuiltinInsertTests.cpp
    unit/FuzzTests.cpp
    unit/MidiMappingTests.cpp
    unit/RenderTests.cpp
    unit/SmfTests.cpp
    unit/StressTests.cpp
    unit/SamplerStreamingTests.cpp
    unit/SamplerTests.cpp
    unit/SamplerWiringTests.cpp
    unit/PatternTests.cpp
    unit/ChannelRackTests.cpp
    unit/PlaylistTests.cpp
    unit/MarkerAndSplitTests.cpp
    unit/MixerTests.cpp
    unit/SidechainTests.cpp
    unit/LoudnessAndRoutingTests.cpp
    unit/ModulationEffectTests.cpp
    unit/TimeStretchTests.cpp
    unit/SlicerTests.cpp
    unit/AutomationTests.cpp
    unit/WavFileTests.cpp
    unit/WavStreamWriterTests.cpp
    unit/AudioRecorderTests.cpp
    unit/RecordingPlacementTests.cpp
    unit/DiskStreamingTests.cpp
    unit/AudioEditsTests.cpp
    unit/AudioClipEditingTests.cpp
    unit/AutomationClipTests.cpp
    unit/InputMonitorTests.cpp
    unit/LoopRecordingTests.cpp
    unit/AudioLoggerTests.cpp
    unit/AuditionTests.cpp
    unit/PluginHostTests.cpp
    unit/AudioUnitTests.cpp
    unit/PluginRegistryTests.cpp
    unit/PluginInsertTests.cpp
    unit/PluginParameterTests.cpp
    unit/PluginStateTests.cpp
    unit/PluginLatencyTests.cpp
    unit/PluginPersistenceTests.cpp
    unit/PluginEditorTests.cpp
    unit/InsertCommandTests.cpp
    unit/AppSettingsTests.cpp
    unit/UpdateCheckTests.cpp
    unit/StandardActionsTests.cpp
    unit/ThemePaletteTests.cpp
    unit/ThemeLibraryTests.cpp)

# incdaw_project pulls in engine -> platform transitively; listing them again
# here puts the same archive on the link line twice and ld warns about it.
# incdaw_app pulls in project -> engine -> platform transitively; listing them
# again here puts the same archive on the link line twice and ld warns about it.
target_link_libraries(incdaw_tests
    PRIVATE doctest
            incdaw_app
            incdaw_plugins
            incdaw_theme
            incdaw_warnings)

target_compile_definitions(incdaw_tests PRIVATE
    INCDAW_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

# ── The test suite's own CLAP plugins ────────────────────────────────────────
# The plugin host cannot be tested against third-party binaries and must not
# be tested against nothing: one well-behaved plugin, one hostile one.
add_library(incdaw-testgain MODULE plugins/TestGainPlugin.cpp)
add_library(incdaw-testcrash MODULE plugins/TestCrashPlugin.cpp)
add_library(incdaw-testlatency MODULE plugins/TestLatencyPlugin.cpp)

foreach(testplugin incdaw-testgain incdaw-testcrash incdaw-testlatency)
    set_target_properties(${testplugin} PROPERTIES PREFIX "" SUFFIX ".clap")
    target_include_directories(${testplugin} PRIVATE ${CMAKE_SOURCE_DIR}/third_party/clap)
endforeach()

add_dependencies(incdaw_tests incdaw-testgain incdaw-testcrash incdaw-testlatency
                 incdaw-pluginscan)

target_compile_definitions(incdaw_tests PRIVATE
    INCDAW_TESTGAIN_PLUGIN="$<TARGET_FILE:incdaw-testgain>"
    INCDAW_TESTCRASH_PLUGIN="$<TARGET_FILE:incdaw-testcrash>"
    INCDAW_TESTLATENCY_PLUGIN="$<TARGET_FILE:incdaw-testlatency>"
    INCDAW_PLUGINSCAN_BINARY="$<TARGET_FILE:incdaw-pluginscan>")

add_test(NAME unit COMMAND incdaw_tests)

# ── Layering test ─────────────────────────────────────────────────────────────
# Enforces docs/ARCHITECTURE.md §2 statically over the source tree. It runs as
# part of the suite so that an architectural violation fails the build the same
# way a failing assertion does.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(NAME layering
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_layering.py
                 ${CMAKE_SOURCE_DIR}/src)
