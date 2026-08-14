#pragma once

#include "app/Command.h"
#include "project/Model.h"
#include "project/RecordingSession.h"

namespace incdaw::app {

/// Lands a finished take in the project: the file becomes an AudioAsset, the
/// asset becomes an audio clip at the take's latency-compensated position, on
/// the first audio track — created on the spot when the project has none.
///
/// One command for all of it so that one Cmd+Z removes the take from the
/// arrangement whole: clip, asset, and the track if this command created it.
/// The FILE on disk is deliberately left alone — undo removes the take from
/// the project, it does not delete a recording the user may want back.
class InsertRecordedTakeCommand final : public Command {
public:
    explicit InsertRecordedTakeCommand(project::RecordingSession::Placement placement)
        : placement_(std::move(placement)) {}

    [[nodiscard]] const char* id() const noexcept override { return "recording.insertTake"; }
    [[nodiscard]] std::string name() const override { return "Record Take"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// The first landed clip, valid only after the first execute.
    [[nodiscard]] project::EntityId clipId() const noexcept
    {
        return clips_.empty() ? project::EntityId{} : clips_.front().id;
    }

private:
    project::RecordingSession::Placement placement_;

    project::AudioAsset asset_;

    /// One clip per placed slice; a straight take has exactly one. Kept with
    /// their insertion indices so redo restores identical ids and positions.
    std::vector<project::Clip> clips_;
    std::vector<std::size_t>   clipIndices_;

    project::Track      track_;         ///< only meaningful when trackCreated_
    std::size_t         assetIndex_ = 0;
    std::size_t         trackIndex_ = 0;
    bool                trackCreated_ = false;
    bool                minted_       = false;
};

} // namespace incdaw::app
