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

// ── WriteAutomationCommand ────────────────────────────────────────────────────

bool WriteAutomationCommand::execute(Project& project)
{
    if (!minted_) {
        if (written_.empty())
            return false;

        std::sort(written_.begin(), written_.end(),
                  [](const AutomationPoint& a, const AutomationPoint& b) {
                      return a.tick < b.tick;
                  });

        const Tick firstTick = written_.front().tick;
        const Tick lastTick  = written_.back().tick;

        project::AutomationLane* lane = nullptr;
        for (std::size_t index = 0; index < project.automation().size(); ++index) {
            project::AutomationLane& candidate = project.automation()[index];
            if (candidate.targetEntity == target_ && candidate.parameterKey == key_) {
                lane       = &candidate;
                laneIndex_ = index;
                break;
            }
        }

        if (lane == nullptr) {
            lane         = &project.addAutomationLane(target_, key_);
            laneIndex_   = project.automation().size() - 1;
            laneCreated_ = true;
        }

        laneId_         = lane->id;
        previousPoints_ = lane->points;

        // The written range replaces that range; everything outside survives.
        std::vector<AutomationPoint> merged;
        for (const AutomationPoint& point : previousPoints_)
            if (point.tick < firstTick)
                merged.push_back(point);

        merged.insert(merged.end(), written_.begin(), written_.end());

        for (const AutomationPoint& point : previousPoints_)
            if (point.tick > lastTick)
                merged.push_back(point);

        lane->points = std::move(merged);

        // Redo restores this recorded result whether the lane was created or
        // merely rewritten.
        laneAfter_ = *lane;

        if (laneCreated_) {
            // The pass lands visibly, like a recorded take does: on the
            // first automation track, spanning what was written.
            project::Track* target = nullptr;
            for (project::Track& track : project.tracks())
                if (track.type == project::TrackType::automation)
                    target = target != nullptr ? target : &track;

            if (target == nullptr) {
                project::Track& created =
                    project.addTrack(project::TrackType::automation, "Automation 1");
                trackIndex_   = project.tracks().size() - 1;
                track_        = created;
                trackCreated_ = true;
                target        = &created;
            }

            project::Clip& clip = project.addClip(project::ClipType::automation,
                                                  target->id, laneId_);
            clip.startTick   = firstTick;
            clip.lengthTicks = std::max<Tick>(lastTick - firstTick,
                                              engine::ticksPerQuarterNote);
            clip.sourceOffsetTicks = firstTick;   // lane ticks are absolute here
            clip.name   = key_;
            clip.colour = 0xFF9977CCu;   // automation reads as its own family

            clip_      = clip;
            clipIndex_ = project.clips().size() - 1;
        }

        minted_ = true;
        return true;
    }

    // Redo: the recorded results, with the same ids.
    if (laneCreated_) {
        if (trackCreated_)
            project.insertTrack(trackIndex_, track_);

        const std::size_t position = std::min(laneIndex_, project.automation().size());
        project.ids().observe(laneAfter_.id);
        project.automation().insert(project.automation().begin()
                                        + static_cast<std::ptrdiff_t>(position),
                                    laneAfter_);
        project.insertClip(clipIndex_, clip_);
    } else {
        for (project::AutomationLane& lane : project.automation())
            if (lane.id == laneId_)
                lane.points = laneAfter_.points;
    }

    return true;
}

void WriteAutomationCommand::undo(Project& project)
{
    if (laneCreated_) {
        (void)project.removeClip(clip_.id);

        for (std::size_t index = 0; index < project.automation().size(); ++index) {
            if (project.automation()[index].id == laneId_) {
                project.automation().erase(project.automation().begin()
                                           + static_cast<std::ptrdiff_t>(index));
                break;
            }
        }

        if (trackCreated_)
            (void)project.removeTrack(track_.id);
    } else {
        for (project::AutomationLane& lane : project.automation())
            if (lane.id == laneId_)
                lane.points = previousPoints_;
    }
}

} // namespace incdaw::app
