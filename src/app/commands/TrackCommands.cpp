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
        const Track& created = project.addTrack(type_, name_);
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

    // Children move up to the removed folder's own parent. Recorded so undo
    // puts them back where they were, whichever level that was.
    reparented_.clear();

    for (Track& track : project.tracks()) {
        if (track.parent != trackId_)
            continue;

        reparented_.push_back({track.id, track.parent});
        track.parent = track_.parent;
    }

    return project.removeTrack(trackId_);
}

void RemoveTrackCommand::undo(Project& project)
{
    project.insertTrack(index_, track_);

    for (const Reparented& entry : reparented_) {
        if (Track* track = project.findTrack(entry.track))
            track->parent = entry.previousParent;
    }

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

// ── SetTrackParentCommand ─────────────────────────────────────────────────────

bool SetTrackParentCommand::execute(Project& project)
{
    Track* track = project.findTrack(trackId_);
    if (track == nullptr)
        return false;

    if (parent_.isValid()) {
        const Track* parent = project.findTrack(parent_);

        // Only a folder can hold tracks, nothing can hold itself, and a loop
        // is refused before it exists rather than defended against after.
        if (parent == nullptr || parent->type != project::TrackType::folder)
            return false;
        if (project::trackWouldCycle(project, trackId_, parent_))
            return false;
    }

    if (track->parent == parent_)
        return false;

    previousParent_ = track->parent;
    previousOrder_  = project.tracks();

    track->parent = parent_;

    // The moved block is the track plus everything under it, kept in its own
    // order so a folder's internal shape survives the move.
    std::vector<EntityId> block{trackId_};
    for (const EntityId id : project::tracksUnder(project, trackId_))
        block.push_back(id);

    const auto inBlock = [&block](const EntityId id) {
        return std::find(block.begin(), block.end(), id) != block.end();
    };

    // What the new parent already holds, so the block lands after it rather
    // than in the middle of it.
    const std::vector<EntityId> siblings = project::tracksUnder(project, parent_);

    std::vector<Track> moved;
    std::vector<Track> rest;
    moved.reserve(block.size());
    rest.reserve(project.tracks().size());

    for (const Track& entry : project.tracks()) {
        if (inBlock(entry.id))
            moved.push_back(entry);
        else
            rest.push_back(entry);
    }

    // Directly after the new parent and whatever it already holds, or at the
    // end of the list when the track came out to the top level.
    std::size_t insertAt = rest.size();

    if (parent_.isValid()) {
        for (std::size_t index = 0; index < rest.size(); ++index) {
            if (rest[index].id != parent_)
                continue;

            insertAt = index + 1;
            while (insertAt < rest.size()
                   && std::find(siblings.begin(), siblings.end(), rest[insertAt].id)
                          != siblings.end())
                ++insertAt;

            break;
        }
    }

    rest.insert(rest.begin() + static_cast<std::ptrdiff_t>(insertAt),
                moved.begin(), moved.end());
    project.tracks() = std::move(rest);

    return true;
}

void SetTrackParentCommand::undo(Project& project)
{
    if (previousOrder_.empty())
        return;

    project.tracks() = previousOrder_;

    if (Track* track = project.findTrack(trackId_))
        track->parent = previousParent_;
}

// ── SetTrackCollapsedCommand ──────────────────────────────────────────────────

bool SetTrackCollapsedCommand::execute(Project& project)
{
    Track* track = project.findTrack(trackId_);
    if (track == nullptr || track->type != project::TrackType::folder
        || track->collapsed == collapsed_)
        return false;

    track->collapsed = collapsed_;
    return true;
}

void SetTrackCollapsedCommand::undo(Project& project)
{
    if (Track* track = project.findTrack(trackId_))
        track->collapsed = !collapsed_;
}

// ── SetTrackColourCommand ─────────────────────────────────────────────────────

bool SetTrackColourCommand::execute(Project& project)
{
    Track* track = project.findTrack(trackId_);
    if (track == nullptr || track->colour == colour_)
        return false;

    previousColour_ = track->colour;
    track->colour   = colour_;
    return true;
}

void SetTrackColourCommand::undo(Project& project)
{
    if (Track* track = project.findTrack(trackId_))
        track->colour = previousColour_;
}

} // namespace incdaw::app
