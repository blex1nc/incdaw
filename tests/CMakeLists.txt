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
    unit/CommandTests.cpp
    unit/PianoRollTests.cpp
    unit/InstrumentTests.cpp
    unit/PatternTests.cpp
    unit/ChannelRackTests.cpp
    unit/PlaylistTests.cpp
    unit/MixerTests.cpp
    unit/AutomationTests.cpp
    unit/WavFileTests.cpp
    unit/WavStreamWriterTests.cpp
    unit/AudioRecorderTests.cpp
    unit/RecordingPlacementTests.cpp
    unit/DiskStreamingTests.cpp)

# incdaw_project pulls in engine -> platform transitively; listing them again
# here puts the same archive on the link line twice and ld warns about it.
# incdaw_app pulls in project -> engine -> platform transitively; listing them
# again here puts the same archive on the link line twice and ld warns about it.
target_link_libraries(incdaw_tests
    PRIVATE doctest
            incdaw_app
            incdaw_plugins
            incdaw_warnings)

target_compile_definitions(incdaw_tests PRIVATE
    INCDAW_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

add_test(NAME unit COMMAND incdaw_tests)

# ── Layering test ─────────────────────────────────────────────────────────────
# Enforces docs/ARCHITECTURE.md §2 statically over the source tree. It runs as
# part of the suite so that an architectural violation fails the build the same
# way a failing assertion does.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_test(NAME layering
         COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/check_layering.py
                 ${CMAKE_SOURCE_DIR}/src)
