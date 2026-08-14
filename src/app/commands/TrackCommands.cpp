#include "app/commands/TrackCommands.h"

#include <algorithm>
#include <utility>

namespace incdaw::app {
namespace {

/// Below this a row cannot show a clip name, and above it one track fills the
/// window. Both extremes read as a bug rather than as a preference.
constexpr int minimumTrackHeight = 24;
constexpr int maximumTrackHeight = 400;

} // namespace

// ── AddTrackCommand ───────────────────────────────────────────────────────────

bool AddTrackCommand::execute(Project& project)
{
    if (!minted_) {
        const Track& created = project.addTrack(project::TrackType::instrument, name_);
        track_  = created;
        index_  = project.tracks().size() - 1;
        minted_ = true;
        return true;
    }

    project.insertTrack(index_, track_);
    return true;
}

void AddTrackCommand::undo(Project& project)
{
    (void)project.removeTrack(track_.id);
}

// ── RemoveTrackCommand ────────────────────────────────────────────────────────

bool RemoveTrackCommand::execute(Project& project)
{
    index_ = project.indexOfTrack(trackId_);
    if (index_ == Project::notFound)
        return false;

    track_ = project.tracks()[index_];

    clips_.clear();

    std::vector<project::Clip>& clips = project.clips();

    // Back to front, so each erase leaves the earlier indices — and therefore
    // the positions recorded for undo — still valid.
    for (std::size_t index = clips.size(); index > 0; --index) {
        const std::size_t position = index - 1;
        if (clips[position].track != trackId_)
            continue;

        clips_.push_back({position, clips[position]});
        clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(position));
    }

    return project.removeTrack(trackId_);
}

void RemoveTrackCommand::undo(Project& project)
{
    project.insertTrack(index_, track_);

    // Front to back over what was captured back to front: each clip returns to
    // the index it originally occupied.
    for (auto entry = clips_.rbegin(); entry != clips_.rend(); ++entry)
        project.insertClip(entry->index, entry->clip);
}

// ── RenameTrackCommand ────────────────────────────────────────────────────────

bool RenameTrackCommand::execute(Project& project)
{
    Track* track = project.findTrack(trackId_);
    if (track == nullptr || track->name == name_)
        return false;

    previousName_ = track->name;
    track->name   = name_;
    return true;
}

void RenameTrackCommand::undo(Project& project)
{
    if (Track* track = project.findTrack(trackId_))
        track->name = previousName_;
}

// ── SetTrackMutedCommand ──────────────────────────────────────────────────────

bool SetTrackMutedCommand::execute(Project& project)
{
    Track* track = project.findTrack(trackId_);
    if (track == nullptr || track->muted == muted_)
        return false;

    track->muted = muted_;
    return true;
}

void SetTrackMutedCommand::undo(Project& project)
{
    if (Track* track = project.findTrack(trackId_))
        track->muted = !muted_;
}

// ── SetTrackSoloedCommand ─────────────────────────────────────────────────────

bool SetTrackSoloedCommand::execute(Project& project)
{
    Track* track = project.findTrack(trackId_);
    if (track == nullptr || track->soloed == soloed_)
        return false;

    track->soloed = soloed_;
    return true;
}

void SetTrackSoloedCommand::undo(Project& project)
{
    if (Track* track = project.findTrack(trackId_))
        track->soloed = !soloed_;
}

// ── SetTrackHeightCommand ─────────────────────────────────────────────────────

bool SetTrackHeightCommand::execute(Project& project)
{
    Track* track = project.findTrack(trackId_);
    if (track == nullptr)
        return false;

    const int clamped = std::clamp(height_, minimumTrackHeight, maximumTrackHeight);
    if (track->height == clamped)
        return false;

    previousHeight_ = track->height;
    height_         = clamped;
    track->height   = clamped;
    return true;
}

void SetTrackHeightCommand::undo(Project& project)
{
    if (Track* track = project.findTrack(trackId_))
        track->height = previousHeight_;
}

bool SetTrackHeightCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetTrackHeightCommand*>(&next);
    return other != nullptr && other->trackId_ == trackId_;
}

void SetTrackHeightCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetTrackHeightCommand*>(&next))
        height_ = other->height_;
}

} // namespace incdaw::app
