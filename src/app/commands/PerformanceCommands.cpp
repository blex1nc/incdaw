#include "app/commands/PerformanceCommands.h"

#include <algorithm>

namespace incdaw::app {

// ── SetStartMarkerCommand ─────────────────────────────────────────────────────

bool SetStartMarkerCommand::execute(Project& project)
{
    previous_.clear();

    bool changed = false;

    for (project::TimelineMarker& marker : project.markers()) {
        if (!marker.isStart)
            continue;

        previous_.push_back(marker.id);
        marker.isStart = false;
        changed        = true;
    }

    if (markerId_.isValid()) {
        project::TimelineMarker* marker = project.findMarker(markerId_);
        if (marker == nullptr) {
            // Put back whatever this command has already cleared: a refusal
            // must leave the project as it found it.
            undo(project);
            return false;
        }

        if (previous_.size() == 1 && previous_.front() == markerId_) {
            marker->isStart = true;
            return false;   // it was already the start marker
        }

        marker->isStart = true;
        changed         = true;
    }

    return changed;
}

void SetStartMarkerCommand::undo(Project& project)
{
    if (project::TimelineMarker* marker = project.findMarker(markerId_))
        marker->isStart = false;

    for (const EntityId id : previous_)
        if (project::TimelineMarker* marker = project.findMarker(id))
            marker->isStart = true;
}

// ── SetTrackPerformanceCommand ────────────────────────────────────────────────

bool SetTrackPerformanceCommand::execute(Project& project)
{
    project::Track* track = project.findTrack(trackId_);
    if (track == nullptr)
        return false;

    if (track->performancePress == press_ && track->performanceMotion == motion_
        && track->triggerSyncTicks == sync_ && track->positionSync == positionSync_)
        return false;

    previousPress_        = track->performancePress;
    previousMotion_       = track->performanceMotion;
    previousSync_         = track->triggerSyncTicks;
    previousPositionSync_ = track->positionSync;

    track->performancePress  = press_;
    track->performanceMotion = motion_;
    track->triggerSyncTicks  = std::max<project::Tick>(0, sync_);
    track->positionSync      = positionSync_;
    return true;
}

void SetTrackPerformanceCommand::undo(Project& project)
{
    if (project::Track* track = project.findTrack(trackId_)) {
        track->performancePress  = previousPress_;
        track->performanceMotion = previousMotion_;
        track->triggerSyncTicks  = previousSync_;
        track->positionSync      = previousPositionSync_;
    }
}

// ── SetClipPerformanceKeyCommand ──────────────────────────────────────────────

bool SetClipPerformanceKeyCommand::execute(Project& project)
{
    project::Clip* clip = project.findClip(clipId_);
    if (clip == nullptr || clip->performanceKey == key_)
        return false;

    previous_.clear();

    const EntityId track = clip->track;

    // The pad comes off whatever else on this track was holding it: two clips
    // answering one pad is a layout that cannot be played.
    if (key_ >= 0) {
        for (project::Clip& other : project.clips()) {
            if (other.id == clipId_ || other.track != track || other.performanceKey != key_)
                continue;

            previous_.push_back({other.id, other.performanceKey});
            other.performanceKey = -1;
        }
    }

    clip = project.findClip(clipId_);
    previous_.push_back({clipId_, clip->performanceKey});
    clip->performanceKey = key_;
    return true;
}

void SetClipPerformanceKeyCommand::undo(Project& project)
{
    for (const Previous& entry : previous_)
        if (project::Clip* clip = project.findClip(entry.id))
            clip->performanceKey = entry.key;
}

} // namespace incdaw::app
