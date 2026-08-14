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
    platform/SystemInfo.cpp)
target_include_directories(incdaw_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_platform PRIVATE incdaw_warnings)

# ── engine/ : realtime core. Knows nothing above it, no OS symbols ────────────
add_library(incdaw_engine STATIC
    engine/core/Time.cpp)
target_include_directories(incdaw_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_engine
    PUBLIC  incdaw_platform
    PRIVATE incdaw_warnings)

# ── plugins/ : plugin hosting (Phase 13) ──────────────────────────────────────
add_library(incdaw_plugins STATIC
    plugins/PluginIdentifier.cpp)
target_include_directories(incdaw_plugins PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_plugins
    PUBLIC  incdaw_platform
    PRIVATE incdaw_warnings)

# ── project/ : data model, serialization, versioning ──────────────────────────
add_library(incdaw_project STATIC
    project/Identity.cpp)
target_include_directories(incdaw_project PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_project
    PUBLIC  incdaw_engine
    PRIVATE incdaw_warnings)

# ── app/ : command registry, undo, session ────────────────────────────────────
add_library(incdaw_app STATIC
    app/Version.cpp)
target_include_directories(incdaw_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(incdaw_app
    PUBLIC  incdaw_project
    PRIVATE incdaw_warnings)

# ── ui/ : native shell. May use OS APIs; nothing depends on it ────────────────
if(APPLE)
    add_executable(INCDAW MACOSX_BUNDLE
        ui/macos/main.mm)
    target_link_libraries(INCDAW
        PRIVATE incdaw_app incdaw_warnings
                "-framework Cocoa")
    set_target_properties(INCDAW PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME        "INCDAW"
        MACOSX_BUNDLE_BUNDLE_VERSION     "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_GUI_IDENTIFIER     "com.incdaw.INCDAW"
        MACOSX_BUNDLE_COPYRIGHT          "INCDAW"
        MACOSX_BUNDLE_INFO_PLIST         "${CMAKE_CURRENT_SOURCE_DIR}/ui/macos/Info.plist.in")
endif()
