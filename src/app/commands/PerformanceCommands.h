#pragma once

#include "app/Command.h"
#include "engine/performance/PerformanceScheduler.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::EntityId;

/// Performance Mode's project state (docs/PERFORMANCE_MODE.md §6, D-041).
///
/// Everything here is arrangement data like any other: undoable, saved, and
/// shared with nothing. The scheduler itself is built from it at compile time
/// and holds no state the project does not.

/// Marks — or unmarks — the arrangement's start marker.
///
/// At most one per arrangement, so setting a new one clears the old. The
/// region before it is the performance zone; an arrangement without one has no
/// zone, which is what every project has until someone asks for one.
class SetStartMarkerCommand final : public Command {
public:
    /// An invalid id clears the start marker without setting another.
    explicit SetStartMarkerCommand(EntityId marker) : markerId_(marker) {}

    [[nodiscard]] const char* id() const noexcept override { return "performance.setStart"; }
    [[nodiscard]] std::string name() const override
    {
        return markerId_.isValid() ? "Set Start Marker" : "Clear Start Marker";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId              markerId_;
    std::vector<EntityId> previous_;   ///< markers that were the start one
};

/// A track's press behaviour, motion behaviour, trigger sync and position sync.
///
/// One command for all four because they are read as one setting — "how this
/// track answers a pad" — and a user who changes two of them has made one
/// decision, not two.
class SetTrackPerformanceCommand final : public Command {
public:
    SetTrackPerformanceCommand(EntityId track, engine::PerformancePress press,
                               engine::PerformanceMotion motion,
                               project::Tick triggerSyncTicks, bool positionSync)
        : trackId_(track), press_(press), motion_(motion),
          sync_(triggerSyncTicks), positionSync_(positionSync) {}

    [[nodiscard]] const char* id() const noexcept override
    {
        return "performance.setTrack";
    }
    [[nodiscard]] std::string name() const override { return "Set Performance Behaviour"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId                  trackId_;
    engine::PerformancePress  press_;
    engine::PerformanceMotion motion_;
    project::Tick             sync_ = 0;
    bool                      positionSync_ = false;

    engine::PerformancePress  previousPress_  = engine::PerformancePress::retrigger;
    engine::PerformanceMotion previousMotion_ = engine::PerformanceMotion::stay;
    project::Tick             previousSync_   = 0;
    bool                      previousPositionSync_ = false;
};

/// Binds a clip to a pad or key, or unbinds it with -1.
///
/// The key is taken off whatever clip on the same track already held it: two
/// clips on one track answering one pad is a layout that cannot be played,
/// because only one of them can win.
class SetClipPerformanceKeyCommand final : public Command {
public:
    SetClipPerformanceKeyCommand(EntityId clip, int key) : clipId_(clip), key_(key) {}

    [[nodiscard]] const char* id() const noexcept override { return "performance.setKey"; }
    [[nodiscard]] std::string name() const override { return "Assign Performance Pad"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct Previous {
        EntityId id;
        int      key = -1;
    };

    EntityId              clipId_;
    int                   key_ = -1;
    std::vector<Previous> previous_;
};

} // namespace incdaw::app
