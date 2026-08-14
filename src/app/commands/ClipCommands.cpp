#include "app/commands/ClipCommands.h"

#include <algorithm>
#include <utility>

namespace incdaw::app {
namespace {

/// A clip shorter than this is invisible at any sane zoom and plays nothing,
/// which reads to the user as a clip that vanished.
constexpr Tick minimumClipLength = 1;

/// The track a row offset lands on, or an invalid id when it falls off either
/// end of the list.
EntityId trackAtOffset(const Project& project, EntityId from, int delta)
{
    const std::size_t index = project.indexOfTrack(from);
    if (index == Project::notFound)
        return {};

    const long long target = static_cast<long long>(index) + delta;
    if (target < 0 || target >= static_cast<long long>(project.tracks().size()))
        return {};

    return project.tracks()[static_cast<std::size_t>(target)].id;
}

} // namespace

// ── AddPatternClipCommand ─────────────────────────────────────────────────────

bool AddPatternClipCommand::execute(Project& project)
{
    if (!minted_) {
        const project::Pattern* pattern = project.findPattern(pattern_);
        if (pattern == nullptr || project.findTrack(track_) == nullptr)
            return false;

        Clip& created = project.addClip(project::ClipType::pattern, track_, pattern_);

        created.startTick   = std::max<Tick>(0, start_);
        created.lengthTicks = length_ > 0 ? length_ : pattern->length;
        created.name        = pattern->name;
        created.colour      = pattern->colour;

        clip_   = created;
        index_  = project.clips().size() - 1;
        minted_ = true;
        return true;
    }

    project.insertClip(index_, clip_);
    return true;
}

void AddPatternClipCommand::undo(Project& project)
{
    (void)project.removeClip(clip_.id);
}

// ── RemoveClipsCommand ────────────────────────────────────────────────────────

std::string RemoveClipsCommand::name() const
{
    return clips_.size() == 1 ? "Remove Clip" : "Remove Clips";
}

bool RemoveClipsCommand::execute(Project& project)
{
    removed_.clear();

    // Collect first, then erase back to front, so the recorded positions stay
    // valid for undo however the ids were ordered.
    std::vector<std::size_t> indices;
    for (const EntityId id : clips_) {
        const std::size_t index = project.indexOfClip(id);
        if (index != Project::notFound)
            indices.push_back(index);
    }

    if (indices.empty())
        return false;

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    for (auto index = indices.rbegin(); index != indices.rend(); ++index) {
        removed_.push_back({*index, project.clips()[*index]});
        project.clips().erase(project.clips().begin() + static_cast<std::ptrdiff_t>(*index));
    }

    return true;
}

void RemoveClipsCommand::undo(Project& project)
{
    for (auto entry = removed_.rbegin(); entry != removed_.rend(); ++entry)
        project.insertClip(entry->index, entry->clip);
}

// ── MoveClipsCommand ──────────────────────────────────────────────────────────

bool MoveClipsCommand::execute(Project& project)
{
    if (clips_.empty() || (tickDelta_ == 0 && trackDelta_ == 0))
        return false;

    // Clamp once for the whole selection rather than per clip: clips keep their
    // relative positions when a drag hits the start of the timeline, which is
    // what dragging a group means.
    Tick tickDelta  = tickDelta_;
    int  trackDelta = trackDelta_;

    for (const EntityId id : clips_) {
        const Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        tickDelta = std::max(tickDelta, -clip->startTick);

        if (trackDelta != 0 && !trackAtOffset(project, clip->track, trackDelta).isValid())
            trackDelta = 0;
    }

    if (tickDelta == 0 && trackDelta == 0)
        return false;

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        clip->startTick += tickDelta;

        if (trackDelta != 0) {
            const EntityId target = trackAtOffset(project, clip->track, trackDelta);
            if (target.isValid())
                clip->track = target;
        }
    }

    appliedTickDelta_  = tickDelta;
    appliedTrackDelta_ = trackDelta;
    return true;
}

void MoveClipsCommand::undo(Project& project)
{
    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        clip->startTick -= appliedTickDelta_;

        if (appliedTrackDelta_ != 0) {
            const EntityId target = trackAtOffset(project, clip->track, -appliedTrackDelta_);
            if (target.isValid())
                clip->track = target;
        }
    }
}

bool MoveClipsCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const MoveClipsCommand*>(&next);
    return other != nullptr && other->clips_ == clips_;
}

void MoveClipsCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const MoveClipsCommand*>(&next)) {
        appliedTickDelta_  += other->appliedTickDelta_;
        appliedTrackDelta_ += other->appliedTrackDelta_;
    }
}

// ── ResizeClipsCommand ────────────────────────────────────────────────────────

bool ResizeClipsCommand::execute(Project& project)
{
    if (clips_.empty() || lengthDelta_ == 0)
        return false;

    previousLengths_.clear();
    previousLengths_.reserve(clips_.size());

    bool changed = false;

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr) {
            previousLengths_.push_back(0);
            continue;
        }

        previousLengths_.push_back(clip->lengthTicks);

        const Tick length = std::max(minimumClipLength, clip->lengthTicks + lengthDelta_);
        changed = changed || length != clip->lengthTicks;
        clip->lengthTicks = length;
    }

    return changed;
}

void ResizeClipsCommand::undo(Project& project)
{
    for (std::size_t index = 0; index < clips_.size() && index < previousLengths_.size(); ++index) {
        if (Clip* clip = project.findClip(clips_[index]))
            clip->lengthTicks = previousLengths_[index];
    }
}

bool ResizeClipsCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const ResizeClipsCommand*>(&next);
    return other != nullptr && other->clips_ == clips_;
}

void ResizeClipsCommand::mergeWith(const Command& next)
{
    // The lengths captured by this command are the ones before the gesture
    // started, so merging keeps them and only accumulates the delta.
    if (const auto* other = dynamic_cast<const ResizeClipsCommand*>(&next))
        lengthDelta_ += other->lengthDelta_;
}

// ── DuplicateClipsCommand ─────────────────────────────────────────────────────

bool DuplicateClipsCommand::execute(Project& project)
{
    if (!minted_) {
        created_.clear();
        createdIds_.clear();

        for (const EntityId id : clips_) {
            const Clip* source = project.findClip(id);
            if (source == nullptr)
                continue;

            // Copy before adding: addClip can reallocate the vector the source
            // lives in.
            Clip copy = *source;

            const EntityId track = trackDelta_ != 0
                                       ? trackAtOffset(project, source->track, trackDelta_)
                                       : source->track;
            if (!track.isValid())
                continue;

            Clip& added = project.addClip(project::ClipType::pattern, track, source->source);

            copy.id        = added.id;
            copy.track     = track;
            copy.startTick = std::max<Tick>(0, copy.startTick + tickDelta_);
            added          = std::move(copy);

            created_.push_back(added);
            createdIds_.push_back(added.id);
        }

        if (created_.empty())
            return false;

        minted_ = true;
        return true;
    }

    for (const Clip& clip : created_)
        project.insertClip(project.clips().size(), clip);

    return true;
}

void DuplicateClipsCommand::undo(Project& project)
{
    for (const EntityId id : createdIds_)
        (void)project.removeClip(id);
}

// ── SetClipMutedCommand ───────────────────────────────────────────────────────

bool SetClipMutedCommand::execute(Project& project)
{
    previous_.clear();
    previous_.reserve(clips_.size());

    bool changed = false;

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr) {
            previous_.push_back(false);
            continue;
        }

        previous_.push_back(clip->muted);
        changed = changed || clip->muted != muted_;
        clip->muted = muted_;
    }

    return changed;
}

void SetClipMutedCommand::undo(Project& project)
{
    for (std::size_t index = 0; index < clips_.size() && index < previous_.size(); ++index) {
        if (Clip* clip = project.findClip(clips_[index]))
            clip->muted = previous_[index];
    }
}

} // namespace incdaw::app
