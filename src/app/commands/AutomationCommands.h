#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::AutomationLane;
using project::AutomationPoint;
using project::EntityId;
using project::Tick;

/// Automation edits.
///
/// Point edits capture the lane's whole point vector rather than per-point
/// deltas: points are kept sorted by tick, so moving one can reorder them and
/// invalidate indices — the same reasoning as QuantizeNotesCommand. Lanes are
/// small; exactness beats cleverness here.

class AddAutomationLaneCommand final : public Command {
public:
    AddAutomationLaneCommand(EntityId target, std::string parameterKey)
        : target_(target), key_(std::move(parameterKey)) {}

    [[nodiscard]] const char* id() const noexcept override { return "automation.addLane"; }
    [[nodiscard]] std::string name() const override { return "Add Automation"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId laneId() const noexcept { return lane_.id; }

private:
    EntityId       target_;
    std::string    key_;
    AutomationLane lane_;
    std::size_t    index_  = 0;
    bool           minted_ = false;
};

class RemoveAutomationLaneCommand final : public Command {
public:
    explicit RemoveAutomationLaneCommand(EntityId lane) : laneId_(lane) {}

    [[nodiscard]] const char* id() const noexcept override { return "automation.removeLane"; }
    [[nodiscard]] std::string name() const override { return "Remove Automation"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId       laneId_;
    AutomationLane lane_;
    std::size_t    index_ = 0;
};

/// Replaces a lane's points wholesale. Add, move, delete and curve edits are
/// all this command with a different next vector, which is what makes every one
/// of them exactly undoable. Mergeable, so dragging a point is one undo.
class SetAutomationPointsCommand final : public Command {
public:
    SetAutomationPointsCommand(EntityId lane, std::vector<AutomationPoint> points,
                               std::string gestureName = "Edit Automation")
        : laneId_(lane), points_(std::move(points)), name_(std::move(gestureName)) {}

    [[nodiscard]] const char* id() const noexcept override { return "automation.setPoints"; }
    [[nodiscard]] std::string name() const override { return name_; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId                     laneId_;
    std::vector<AutomationPoint> points_;
    std::vector<AutomationPoint> previous_;
    std::string                  name_;
};

/// Creates an automation clip for a parameter that has none.
///
/// The workflow verb behind "automate this control": the lane is made if the
/// project has none for (target, key), an automation track is made if there is
/// none, and a clip lands on it spanning the requested bars — seeded with a
/// flat pair of points at the parameter's current value, so the clip opens on
/// the line the control is already sitting at rather than on an empty lane
/// that would snap it to zero the moment it played.
///
/// Deliberately the same landing pattern as a recorded pass: one undo entry
/// takes the clip, the lane and the track back together, and redo restores the
/// same ids.
class CreateAutomationClipCommand final : public Command {
public:
    CreateAutomationClipCommand(EntityId target, std::string parameterKey,
                                Tick startTick, Tick lengthTicks, double value = 0.5)
        : target_(target), key_(std::move(parameterKey)), start_(startTick),
          length_(lengthTicks), value_(value) {}

    [[nodiscard]] const char* id() const noexcept override { return "automation.createClip"; }
    [[nodiscard]] std::string name() const override { return "Create Automation Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId laneId() const noexcept { return laneId_; }
    [[nodiscard]] EntityId clipId() const noexcept { return clip_.id; }

private:
    EntityId    target_;
    std::string key_;
    Tick        start_  = 0;
    Tick        length_ = 0;
    double      value_  = 0.5;

    EntityId                laneId_;
    project::AutomationLane lane_;
    project::Clip           clip_;
    project::Track          track_;
    std::size_t             laneIndex_  = 0;
    std::size_t             clipIndex_  = 0;
    std::size_t             trackIndex_ = 0;
    bool laneCreated_  = false;
    bool trackCreated_ = false;
    bool minted_       = false;
};

/// Gives a clip a lane of its own, copied from the one it shares.
///
/// Automation clips reference a lane by id, exactly as pattern clips reference
/// a pattern: two clips of one lane are the same ride, and editing either
/// changes both. That is usually what is wanted; this is the verb for when it
/// is not.
class MakeAutomationClipUniqueCommand final : public Command {
public:
    explicit MakeAutomationClipUniqueCommand(EntityId clip) : clipId_(clip) {}

    [[nodiscard]] const char* id() const noexcept override { return "automation.makeUnique"; }
    [[nodiscard]] std::string name() const override { return "Make Automation Unique"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId laneId() const noexcept { return lane_.id; }

private:
    EntityId                clipId_;
    project::AutomationLane lane_;
    EntityId                previousLane_;
    std::size_t             index_  = 0;
    bool                    minted_ = false;
};

/// Lands one recorded automation pass (write mode).
///
/// Existing lane for (target, key): the written range replaces that range's
/// points and everything outside it survives — writing over bars 3..5 must
/// not erase bar 1. No lane yet: the lane is created, and with it an
/// automation clip on the first automation track (created if none) spanning
/// the written range, so the pass is immediately visible in the playlist —
/// the same landing pattern as a recorded audio take. One undo removes it
/// all; redo restores the same ids.
class WriteAutomationCommand final : public Command {
public:
    WriteAutomationCommand(EntityId target, std::string parameterKey,
                           std::vector<AutomationPoint> written)
        : target_(target), key_(std::move(parameterKey)), written_(std::move(written)) {}

    [[nodiscard]] const char* id() const noexcept override { return "automation.write"; }
    [[nodiscard]] std::string name() const override { return "Record Automation"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId                     target_;
    std::string                  key_;
    std::vector<AutomationPoint> written_;

    EntityId                     laneId_;
    std::vector<AutomationPoint> previousPoints_;   ///< when writing to an existing lane
    project::AutomationLane      laneAfter_;        ///< for redo of a created lane
    std::size_t                  laneIndex_ = 0;
    project::Clip                clip_;
    project::Track               track_;
    std::size_t                  clipIndex_  = 0;
    std::size_t                  trackIndex_ = 0;
    bool laneCreated_  = false;
    bool trackCreated_ = false;
    bool minted_       = false;
};

} // namespace incdaw::app
