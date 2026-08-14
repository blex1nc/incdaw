#include "app/commands/AutomationCommands.h"

#include <algorithm>

namespace incdaw::app {
namespace {

AutomationLane* findLane(Project& project, EntityId id) noexcept
{
    for (AutomationLane& lane : project.automation())
        if (lane.id == id)
            return &lane;

    return nullptr;
}

/// Points live sorted by tick; every write goes through here so no reader ever
/// has to wonder.
void sortPoints(std::vector<AutomationPoint>& points)
{
    std::stable_sort(points.begin(), points.end(),
                     [](const AutomationPoint& a, const AutomationPoint& b) {
                         return a.tick < b.tick;
                     });
}

} // namespace

// ── AddAutomationLaneCommand ──────────────────────────────────────────────────

bool AddAutomationLaneCommand::execute(Project& project)
{
    if (!minted_) {
        const AutomationLane& created = project.addAutomationLane(target_, key_);
        lane_   = created;
        index_  = project.automation().size() - 1;
        minted_ = true;
        return true;
    }

    project.ids().observe(lane_.id);

    const std::size_t position = std::min(index_, project.automation().size());
    project.automation().insert(project.automation().begin() + static_cast<std::ptrdiff_t>(position),
                                lane_);
    return true;
}

void AddAutomationLaneCommand::undo(Project& project)
{
    for (std::size_t index = 0; index < project.automation().size(); ++index) {
        if (project.automation()[index].id == lane_.id) {
            project.automation().erase(project.automation().begin()
                                       + static_cast<std::ptrdiff_t>(index));
            return;
        }
    }
}

// ── RemoveAutomationLaneCommand ───────────────────────────────────────────────

bool RemoveAutomationLaneCommand::execute(Project& project)
{
    for (std::size_t index = 0; index < project.automation().size(); ++index) {
        if (project.automation()[index].id != laneId_)
            continue;

        index_ = index;
        lane_  = project.automation()[index];
        project.automation().erase(project.automation().begin()
                                   + static_cast<std::ptrdiff_t>(index));
        return true;
    }

    return false;
}

void RemoveAutomationLaneCommand::undo(Project& project)
{
    const std::size_t position = std::min(index_, project.automation().size());
    project.automation().insert(project.automation().begin() + static_cast<std::ptrdiff_t>(position),
                                lane_);
}

// ── SetAutomationPointsCommand ────────────────────────────────────────────────

bool SetAutomationPointsCommand::execute(Project& project)
{
    AutomationLane* lane = findLane(project, laneId_);
    if (lane == nullptr)
        return false;

    sortPoints(points_);

    if (lane->points == points_)
        return false;

    previous_    = lane->points;
    lane->points = points_;
    return true;
}

void SetAutomationPointsCommand::undo(Project& project)
{
    if (AutomationLane* lane = findLane(project, laneId_))
        lane->points = previous_;
}

bool SetAutomationPointsCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetAutomationPointsCommand*>(&next);
    return other != nullptr && other->laneId_ == laneId_ && other->name_ == name_;
}

void SetAutomationPointsCommand::mergeWith(const Command& next)
{
    // The merged entry keeps `previous_` from before the gesture and adopts the
    // latest points: undoing a drag returns to before the drag began.
    if (const auto* other = dynamic_cast<const SetAutomationPointsCommand*>(&next))
        points_ = other->points_;
}

} // namespace incdaw::app
