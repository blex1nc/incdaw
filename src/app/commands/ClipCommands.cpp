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

    const engine::TempoMap& tempoMap = project.tempoMap();

    // Clamp once for the whole selection rather than per clip: clips keep their
    // relative positions when a drag hits the start of the timeline, which is
    // what dragging a group means. Audio clips take part through the D-013
    // accessor, so a mixed selection clamps as one.
    Tick tickDelta  = tickDelta_;
    int  trackDelta = trackDelta_;

    for (const EntityId id : clips_) {
        const Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        tickDelta = std::max(tickDelta, -project::clipStartTicks(*clip, tempoMap));

        if (trackDelta != 0 && !trackAtOffset(project, clip->track, trackDelta).isValid())
            trackDelta = 0;
    }

    if (tickDelta == 0 && trackDelta == 0)
        return false;

    movedAudio_.clear();

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        if (clip->type == project::ClipType::audio) {
            movedAudio_.push_back({id, clip->start});

            // Only when time actually moves: a pure track move must not push
            // the frame position through a tick round trip, which can shift
            // it by a frame for nothing.
            if (tickDelta != 0) {
                const Tick startTicks = project::clipStartTicks(*clip, tempoMap);
                clip->start = std::max<project::FramePosition>(
                    0, tempoMap.frameForTick(startTicks + tickDelta));
            }
        } else {
            clip->startTick += tickDelta;
        }

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

        if (clip->type != project::ClipType::audio)
            clip->startTick -= appliedTickDelta_;

        if (appliedTrackDelta_ != 0) {
            const EntityId target = trackAtOffset(project, clip->track, -appliedTrackDelta_);
            if (target.isValid())
                clip->track = target;
        }
    }

    // Frame positions come back from the snapshot, not from arithmetic:
    // tick->frame conversion does not invert exactly across tempo changes.
    for (const MovedAudioClip& moved : movedAudio_)
        if (Clip* clip = project.findClip(moved.id))
            clip->start = moved.previousStart;
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

        // The requested deltas must accumulate too: redo re-runs execute(),
        // and an execute that replayed only the gesture's first step would
        // desynchronise the history on the next undo. `movedAudio_` keeps
        // OUR snapshots — the gesture's starting positions.
        tickDelta_  += other->appliedTickDelta_;
        trackDelta_ += other->appliedTrackDelta_;
    }
}

// ── ResizeClipsCommand ────────────────────────────────────────────────────────

bool ResizeClipsCommand::execute(Project& project)
{
    if (clips_.empty() || lengthDelta_ == 0)
        return false;

    const engine::TempoMap& tempoMap = project.tempoMap();

    previousLengths_.clear();
    previousLengths_.reserve(clips_.size());
    previousFrameLengths_.clear();
    previousFrameLengths_.reserve(clips_.size());

    bool changed = false;

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr) {
            previousLengths_.push_back(0);
            previousFrameLengths_.push_back(0);
            continue;
        }

        if (clip->type == project::ClipType::audio) {
            previousLengths_.push_back(0);
            previousFrameLengths_.push_back(clip->length);

            // The delta arrives in ticks (the grid the user drags on); it is
            // applied at the clip's END, where the handle is — with a tempo
            // change inside the clip, that is the only point where grid and
            // audio agree about what just happened.
            const Tick endTick = tempoMap.tickForFrame(clip->start + clip->length);
            const project::FramePosition newEnd = tempoMap.frameForTick(endTick + lengthDelta_);

            const project::FrameCount length =
                std::max<project::FrameCount>(1, newEnd - clip->start);

            changed = changed || length != clip->length;
            clip->length = length;
            continue;
        }

        previousLengths_.push_back(clip->lengthTicks);
        previousFrameLengths_.push_back(0);

        const Tick length = std::max(minimumClipLength, clip->lengthTicks + lengthDelta_);
        changed = changed || length != clip->lengthTicks;
        clip->lengthTicks = length;
    }

    return changed;
}

void ResizeClipsCommand::undo(Project& project)
{
    for (std::size_t index = 0; index < clips_.size() && index < previousLengths_.size(); ++index) {
        Clip* clip = project.findClip(clips_[index]);
        if (clip == nullptr)
            continue;

        if (clip->type == project::ClipType::audio) {
            if (index < previousFrameLengths_.size())
                clip->length = previousFrameLengths_[index];
        } else {
            clip->lengthTicks = previousLengths_[index];
        }
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

// ── StretchClipsCommand ───────────────────────────────────────────────────────

bool StretchClipsCommand::execute(Project& project)
{
    if (lengthDelta_ == 0)
        return false;

    const engine::TempoMap& tempoMap = project.tempoMap();
    const bool firstRun              = previous_.empty();

    bool changed = false;
    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr || clip->type != project::ClipType::audio
            || clip->stretchRatio <= 0.0 || clip->length == 0)
            continue;

        const Tick endTick = project::clipStartTicks(*clip, tempoMap)
                           + project::clipLengthTicks(*clip, tempoMap) + lengthDelta_;
        const project::FramePosition endFrame = tempoMap.frameForTick(endTick);
        if (endFrame <= clip->start)
            continue;   // stretched into nothing — refuse rather than vanish

        const auto newLength = static_cast<project::FrameCount>(endFrame - clip->start);
        if (newLength == clip->length)
            continue;

        if (firstRun)
            previous_.push_back({ id, clip->length, clip->stretchRatio });

        const double factor = static_cast<double>(newLength)
                            / static_cast<double>(clip->length);

        clip->length       = newLength;
        clip->stretchRatio = clip->stretchRatio * factor;
        changed            = true;
    }

    return changed;
}

void StretchClipsCommand::undo(Project& project)
{
    for (const Snapshot& snapshot : previous_) {
        if (Clip* clip = project.findClip(snapshot.id)) {
            clip->length       = snapshot.previousLength;
            clip->stretchRatio = snapshot.previousRatio;
        }
    }
}

bool StretchClipsCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const StretchClipsCommand*>(&next);
    return other != nullptr && other->clips_ == clips_;
}

void StretchClipsCommand::mergeWith(const Command& next)
{
    // previous_ keeps the pre-gesture state; only the distance accumulates.
    if (const auto* other = dynamic_cast<const StretchClipsCommand*>(&next))
        lengthDelta_ += other->lengthDelta_;
}

// ── SplitClipCommand ──────────────────────────────────────────────────────────

bool SplitClipCommand::execute(Project& project)
{
    Clip* clip = project.findClip(clip_);
    if (clip == nullptr)
        return false;

    const engine::TempoMap& tempoMap = project.tempoMap();

    // The cut must fall strictly inside the clip, or one half would be empty.
    const Tick startTicks = project::clipStartTicks(*clip, tempoMap);
    const Tick endTicks   = startTicks + project::clipLengthTicks(*clip, tempoMap);
    if (splitTick_ <= startTicks || splitTick_ >= endTicks)
        return false;

    previous_ = *clip;

    Clip rightHalf = *clip;
    if (!minted_) {
        rightHalf.id = project.ids().next();
        minted_      = true;
    } else {
        rightHalf.id = right_.id;
    }

    if (clip->type == project::ClipType::audio) {
        const project::FramePosition splitFrame = tempoMap.frameForTick(splitTick_);
        const project::FramePosition endFrame =
            clip->start + static_cast<project::FramePosition>(clip->length);
        if (splitFrame <= clip->start || splitFrame >= endFrame)
            return false;

        const project::FrameCount leftLength =
            static_cast<project::FrameCount>(splitFrame - clip->start);

        rightHalf.start        = splitFrame;
        rightHalf.length       = clip->length - leftLength;
        rightHalf.sourceOffset = clip->sourceOffset + leftLength;

        clip->length = leftLength;
    } else {
        const Tick leftLength = splitTick_ - clip->startTick;

        rightHalf.startTick         = splitTick_;
        rightHalf.lengthTicks       = clip->lengthTicks - leftLength;
        rightHalf.sourceOffsetTicks = clip->sourceOffsetTicks + leftLength;

        clip->lengthTicks = leftLength;
    }

    // The fades stay where they audibly were: in at the far left, out at the
    // far right, nothing at the seam.
    clip->fadeOutFrames    = 0;
    rightHalf.fadeInFrames = 0;

    right_ = rightHalf;
    project.insertClip(project.indexOfClip(clip_) + 1, std::move(rightHalf));
    return true;
}

void SplitClipCommand::undo(Project& project)
{
    (void)project.removeClip(right_.id);

    if (Clip* clip = project.findClip(clip_))
        *clip = previous_;
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

// ── SetClipPanCommand ─────────────────────────────────────────────────────────

bool SetClipPanCommand::execute(Project& project)
{
    const double pan = std::clamp(pan_, -1.0, 1.0);

    previous_.clear();
    previous_.reserve(clips_.size());

    bool changed = false;

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr) {
            previous_.push_back(0.0);
            continue;
        }

        previous_.push_back(clip->pan);
        changed = changed || clip->pan != pan;
        clip->pan = pan;
    }

    return changed;
}

void SetClipPanCommand::undo(Project& project)
{
    for (std::size_t index = 0; index < clips_.size() && index < previous_.size(); ++index) {
        if (Clip* clip = project.findClip(clips_[index]))
            clip->pan = previous_[index];
    }
}

bool SetClipPanCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetClipPanCommand*>(&next);
    return other != nullptr && other->clips_ == clips_;
}

void SetClipPanCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetClipPanCommand*>(&next))
        pan_ = other->pan_;
}

} // namespace incdaw::app
