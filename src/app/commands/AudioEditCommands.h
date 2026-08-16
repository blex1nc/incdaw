#pragma once

#include "app/Command.h"
#include "engine/audio/AudioEdits.h"
#include "project/Model.h"

#include <string>
#include <vector>

namespace incdaw::app {

/// Destructive audio edits, undoably.
///
/// The edit rewrites the asset's FILE — that is what "destructive" means and
/// what the user asked for — but the command snapshots the affected samples
/// first, so Cmd+Z restores the file bit-exactly. Redo writes the snapshotted
/// RESULT rather than re-running the operation: re-normalizing normalized
/// audio happens to be harmless, re-applying gain would not be.
///
/// Edited audio renders as float32 whatever the file held before: edits are
/// float math, and quantising the result back into 16 bits on every write
/// would accumulate damage the user never asked for.
enum class AudioEditOp : std::uint8_t { gain, normalize, reverse, silence, fadeIn, fadeOut };

class EditAssetRegionCommand final : public Command {
public:
    /// `factor` is the gain multiplier for `gain`, the target peak for
    /// `normalize`, ignored otherwise.
    EditAssetRegionCommand(project::EntityId asset, engine::edits::Region region,
                           AudioEditOp op, engine::Sample factor = 1.0f)
        : asset_(asset), region_(region), op_(op), factor_(factor) {}

    [[nodiscard]] const char* id() const noexcept override { return "audio.editRegion"; }
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    project::EntityId    asset_;
    engine::edits::Region region_;
    AudioEditOp          op_;
    engine::Sample       factor_ = 1.0f;

    /// Region samples before and after the first execute, per channel.
    std::vector<std::vector<engine::Sample>> before_;
    std::vector<std::vector<engine::Sample>> after_;
    engine::edits::Region                    applied_;   ///< clamped region actually edited
    bool                                     minted_ = false;
};

/// Keeps only [from, to) of the asset. Clips referencing the asset are not
/// re-pointed: audio beyond the shortened file plays as silence, which is
/// safe, and re-anchoring clips to trimmed content is an editor affordance
/// for later, not something to guess at here.
class TrimAssetCommand final : public Command {
public:
    TrimAssetCommand(project::EntityId asset, engine::edits::Region keep)
        : asset_(asset), keep_(keep) {}

    [[nodiscard]] const char* id() const noexcept override { return "audio.trim"; }
    [[nodiscard]] std::string name() const override { return "Trim"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    project::EntityId    asset_;
    engine::edits::Region keep_;

    /// What the trim removed, for reconstruction on undo.
    std::vector<std::vector<engine::Sample>> head_;
    std::vector<std::vector<engine::Sample>> tail_;
    engine::edits::Region                    applied_;
    engine::FrameCount                       previousFrameCount_ = 0;
    bool                                     minted_ = false;
};

/// Replaces [region) of the asset with a time-stretched and/or pitch-shifted
/// rendering of it (engine::dsp::timeStretch — WSOLA with transient locking).
/// The file's length changes; undo restores the original samples bit-exactly
/// and redo writes the recorded result rather than re-rendering.
class StretchAssetCommand final : public Command {
public:
    StretchAssetCommand(project::EntityId asset, engine::edits::Region region, double ratio,
                        double pitchSemitones)
        : asset_(asset), region_(region), ratio_(ratio), pitchSemitones_(pitchSemitones) {}

    [[nodiscard]] const char* id() const noexcept override { return "audio.stretch"; }
    [[nodiscard]] std::string name() const override
    {
        return ratio_ != 1.0 ? "Time Stretch" : "Pitch Shift";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    project::EntityId     asset_;
    engine::edits::Region region_;
    double                ratio_          = 1.0;
    double                pitchSemitones_ = 0.0;

    std::vector<std::vector<engine::Sample>> before_;   ///< the original region
    std::vector<std::vector<engine::Sample>> after_;    ///< the rendered replacement
    engine::edits::Region                    applied_;
    bool                                     minted_ = false;
};

} // namespace incdaw::app
