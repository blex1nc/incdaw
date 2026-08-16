#pragma once

#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>

namespace incdaw::app {

/// The AudioAsset a file became, and everything undo and redo need to put it
/// back exactly as it was.
///
/// Kept as data rather than as behaviour so that a command can hold one as a
/// member: the id has to survive undo/redo unchanged, or the zones and clips
/// written by commands above it in the stack would point at nothing.
struct AudioAssetImport {
    project::EntityId   id;
    project::AudioAsset asset;          ///< the asset itself, found or created
    bool                created = false;
    std::size_t         index   = 0;    ///< where a created asset was inserted
};

/// Finds the project's AudioAsset for `path`, or creates one from the file's
/// header.
///
/// The header is PROBED, never decoded, and before anything is mutated: an
/// unreadable file must be a clean refusal rather than a half-applied edit. A
/// file already in the project is shared, not duplicated — which is what makes
/// dropping the same loop twice cost one asset.
[[nodiscard]] bool importAudioAsset(project::Project& project, const std::string& path,
                                    AudioAssetImport& imported);

/// Puts a created asset back after an undo removed it, keeping its id and its
/// place in the list. A no-op when the asset was found rather than created, or
/// is already there.
void restoreImportedAsset(project::Project& project, const AudioAssetImport& imported);

} // namespace incdaw::app
