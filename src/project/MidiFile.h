#pragma once

#include "engine/midi/SmfFile.h"
#include "project/Model.h"

#include <filesystem>

namespace incdaw::project {

/// Standard MIDI File exchange for projects (docs/ROADMAP.md Phase 17).
///
/// Export walks the ARRANGEMENT through the same pattern compiler playback
/// uses — probability rolls included, seeded — so the file holds exactly
/// what a render would have played. Import creates a new pattern (and a
/// channel per track), which is the pattern-workflow answer to "here is
/// someone else's MIDI": it lands as editable material, not as a mystery
/// arrangement.
[[nodiscard]] engine::SmfFile::Result exportArrangement(const Project& project,
                                                        const std::filesystem::path& path,
                                                        std::uint64_t randomSeed = 0);

struct MidiImportResult {
    bool        succeeded = false;
    std::string error;

    EntityId pattern;                    ///< the created pattern
    std::vector<EntityId> newChannels;   ///< channels created for the tracks

    [[nodiscard]] explicit operator bool() const noexcept { return succeeded; }
};

[[nodiscard]] MidiImportResult importAsPattern(Project& project,
                                               const std::filesystem::path& path);

} // namespace incdaw::project
