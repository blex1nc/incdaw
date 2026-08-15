#pragma once

#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"

#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::project {

/// Plugin state blobs in the project package (docs/PLUGIN_HOST.md §6).
///
/// The blobs live under `plugins/` inside the package, one file per insert
/// slot, named by the slot's entity id. `PluginSlot::stateFile` records the
/// package-relative path — that field already travels in project.json, so no
/// format change is involved: a missing plugin keeps its `stateFile` (and the
/// file keeps sitting in the package), and installing the plugin later
/// restores the session intact.
///
/// Both calls are main-thread and filesystem-touching: they happen at project
/// save and after a compile, never anywhere near the audio thread.

/// Captures the state of every live insert into `packageDir`, updating each
/// slot's `stateFile`. Call BEFORE ProjectFile::save so the fields land in
/// project.json.
///
/// A slot with no live state carrier (missing plugin, bypassed, no state
/// extension) is left exactly as it was. A plugin that fails to save keeps
/// its previous blob — state survives a misbehaving plugin. Problems are
/// reported as warnings; capturing state never fails a save wholesale.
[[nodiscard]] std::vector<std::string> capturePluginState(Project& project,
                                                          const CompiledProjectGraph& graph,
                                                          const std::filesystem::path& packageDir);

/// Restores each slot's recorded blob into its live instance. Call after a
/// compile of a project loaded from `packageDir`.
///
/// A slot with an empty `stateFile`, or one that is not in the graph, is
/// skipped silently. A blob that cannot be read, or that the plugin refuses,
/// is a warning — the plugin then plays with its defaults, which the UI
/// should say rather than hide.
[[nodiscard]] std::vector<std::string> restorePluginState(const Project& project,
                                                          const CompiledProjectGraph& graph,
                                                          const std::filesystem::path& packageDir);

} // namespace incdaw::project
