#pragma once

#include "app/Command.h"
#include "project/Identity.h"

#include <cstddef>
#include <vector>

namespace incdaw::app {

using project::EntityId;
using project::Tick;

/// Slices an audio asset onto a new sampler channel and writes its timing
/// into a pattern — the Fruity Slicer 2 workflow (docs/FL2026_GAP.md P8):
/// every slice lands on its own key, chromatically from `firstKey`, and the
/// pattern replays the loop with its original timing, ready to rearrange.
///
/// One command, one undo: the channel, its zones and the notes appear and
/// disappear together. Onset detection runs in the caller
/// (engine::audio::detectOnsets), so executing — and re-executing on redo —
/// is deterministic and cheap.
class SliceAssetCommand final : public Command {
public:
    SliceAssetCommand(EntityId asset, EntityId pattern, std::vector<project::FrameCount> onsets,
                      int firstKey = 48)
        : asset_(asset), pattern_(pattern), onsets_(std::move(onsets)), firstKey_(firstKey) {}

    [[nodiscard]] const char* id() const noexcept override { return "slicer.sliceAsset"; }
    [[nodiscard]] std::string name() const override { return "Slice to Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// The created channel, valid after the first execute.
    [[nodiscard]] EntityId channelId() const noexcept { return channel_.id; }

private:
    EntityId                         asset_;
    EntityId                         pattern_;
    std::vector<project::FrameCount> onsets_;
    int                              firstKey_ = 48;

    project::Channel channel_;        ///< the built channel, zones included
    std::size_t      channelIndex_ = 0;
    std::size_t      firstNoteIndex_ = 0;
    std::size_t      noteCount_      = 0;
    bool             minted_         = false;
};

} // namespace incdaw::app
