#pragma once

#include "app/Command.h"
#include "project/Identity.h"

#include <cstddef>
#include <string>

namespace incdaw::app {

using project::EntityId;
using project::Tick;
using project::TimelineMarker;

/// Timeline marker and region edits (docs/FL2026_GAP.md P3). Markers carry
/// arrangement navigation and, later, FL-style performance-mode triggering;
/// a region is just a marker with a length.

/// Drops a marker (or region, when `length` > 0) on the timeline.
class AddMarkerCommand final : public Command {
public:
    AddMarkerCommand(Tick tick, std::string name, Tick length = 0)
        : tick_(tick), name_(std::move(name)), length_(length) {}

    [[nodiscard]] const char* id() const noexcept override { return "marker.add"; }
    [[nodiscard]] std::string name() const override
    {
        return length_ > 0 ? "Add Region" : "Add Marker";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId markerId() const noexcept { return marker_.id; }

private:
    Tick           tick_   = 0;
    std::string    name_;
    Tick           length_ = 0;

    TimelineMarker marker_;
    std::size_t    index_  = 0;
    bool           minted_ = false;
};

class RemoveMarkerCommand final : public Command {
public:
    explicit RemoveMarkerCommand(EntityId marker) : markerId_(marker) {}

    [[nodiscard]] const char* id() const noexcept override { return "marker.remove"; }
    [[nodiscard]] std::string name() const override { return "Remove Marker"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId       markerId_;
    TimelineMarker removed_;
    std::size_t    index_ = 0;
};

/// Rewrites a marker's position, span, name or colour in one step. Mergeable,
/// so dragging a marker along the ruler is a single undo.
class EditMarkerCommand final : public Command {
public:
    EditMarkerCommand(EntityId marker, TimelineMarker updated)
        : markerId_(marker), updated_(std::move(updated)) {}

    [[nodiscard]] const char* id() const noexcept override { return "marker.edit"; }
    [[nodiscard]] std::string name() const override { return "Edit Marker"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId       markerId_;
    TimelineMarker updated_;
    TimelineMarker previous_;
};

} // namespace incdaw::app
