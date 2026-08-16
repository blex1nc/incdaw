// app/ProjectSession.h — the headless half of the shell's project lifecycle:
// recent-project bookkeeping and autosave destinations.
//
// The shell owns the timers, the user-defaults persistence and the dialogs;
// everything that is a decision rather than an interaction lives here, so it
// can be tested without AppKit — the same split PianoRollModel and
// PlaylistModel already use.

#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::app::session {

/// The recents list after `path` has just been saved or opened: moved (or
/// inserted) to the front, deduplicated, capped at `cap`. An empty path
/// returns the list unchanged.
std::vector<std::string> updatedRecents(std::vector<std::string> list,
                                        const std::string&       path,
                                        std::size_t              cap);

/// Where an autosave of the current project belongs. A saved project
/// autosaves next to itself as "<stem>.autosave.incdaw", so the backup is
/// wherever the project is; an unsaved one autosaves under
/// "<supportDirectory>/Autosave/Untitled.autosave.incdaw". Empty when
/// neither location exists to aim at.
std::filesystem::path autosavePathFor(const std::filesystem::path& projectPath,
                                      const std::filesystem::path& supportDirectory);

/// True when both packages exist and the autosave's project.json is strictly
/// newer than the project's own — the "last session ended without a save"
/// signal. A missing file is never newer.
bool autosaveIsNewer(const std::filesystem::path& projectPackage,
                     const std::filesystem::path& autosavePackage);

} // namespace incdaw::app::session
