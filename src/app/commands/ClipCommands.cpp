#include "app/commands/ClipCommands.h"

#include <algorithm>
#include <utility>

namespace incdaw::app {
namespace {

/// A clip shorter than this is invisible at any sane zoom and plays nothing,
/// which reads to the user as a clip that vanished.
constexpr Tick minimumClipLength = 1;

/// The first lane of `track` where [start, start + length) is free, starting
/// at `preferred`.
///
/// Placing a clip on top of another used to mean one of them disappeared
/// behind the other; it now means the new one takes the next lane down. The
/// search is over the clip's own span rather than the whole track, so a track
/// stays one lane deep wherever nothing actually overlaps.
int freeLane(const Project& project, EntityId track, Tick start, Tick length,
             int preferred, EntityId ignore = {})
{
    const engine::TempoMap& tempoMap = project.tempoMap();
    const Tick              end      = start + std::max<Tick>(length, 1);

    for (int lane = std::max(0, preferred); lane < 1024; ++lane) {
        bool taken = false;

        for (const Clip& clip : project.clips()) {
            if (clip.track != track || clip.lane != lane || clip.id == ignore)
                continue;

            const Tick clipStart = project::clipStartTicks(clip, tempoMap);
            const Tick clipEnd   = clipStart
                                 + std::max<Tick>(project::clipLengthTicks(clip, tempoMap), 1);

            if (clipStart < end && clipEnd > start) {
                taken = true;
                break;
            }
        }

        if (!taken)
            return lane;
    }

    return preferred;   // a thousand deep is not a lane problem any more
}

/// Pulls in every clip that shares a group with one already in the list.
///
/// A group is a decision about the arrangement — these four bars belong
/// together — so a verb aimed at one of them is aimed at all of them, however
/// the user happened to select. Applied inside the commands, like the lock, so
/// every route to an edit gets it.
void expandToGroups(const Project& project, ClipIds& clips)
{
    std::vector<EntityId> groups;

    for (const EntityId id : clips) {
        const Clip* clip = project.findClip(id);
        if (clip == nullptr || !clip->group.isValid())
            continue;

        if (std::find(groups.begin(), groups.end(), clip->group) == groups.end())
            groups.push_back(clip->group);
    }

    if (groups.empty())
        return;

    // Walked in project order so the expanded list is stable: two commands
    // built from the same selection target the same clips in the same order,
    // which is what keeps `canMergeWith` comparing lists meaningful.
    for (const Clip& clip : project.clips()) {
        if (!clip.group.isValid())
            continue;
        if (std::find(groups.begin(), groups.end(), clip.group) == groups.end())
            continue;
        if (std::find(clips.begin(), clips.end(), clip.id) != clips.end())
            continue;

        clips.push_back(clip.id);
    }
}

/// Drops locked clips from an edit's target list.
///
/// A lock refuses the verb rather than being checked in the UI: the playlist,
/// a menu item, a keyboard shortcut and an eventual script all reach the same
/// commands, and a rule enforced in only one of those routes is not a rule.
/// The filter runs on every execute, so redo after locking a clip drops it too
/// — the redone gesture is the one the lock now allows.
void dropLocked(const Project& project, ClipIds& clips)
{
    // A group moves as one, so it locks as one: one locked member pins the
    // whole group, rather than the rest sliding out from under it.
    std::vector<EntityId> lockedGroups;
    for (const EntityId id : clips) {
        const Clip* clip = project.findClip(id);
        if (clip != nullptr && clip->locked && clip->group.isValid())
            lockedGroups.push_back(clip->group);
    }

    clips.erase(std::remove_if(clips.begin(), clips.end(),
                               [&project, &lockedGroups](const EntityId id) {
                                   const Clip* clip = project.findClip(id);
                                   if (clip == nullptr)
                                       return false;
                                   if (clip->locked)
                                       return true;

                                   return clip->group.isValid()
                                       && std::find(lockedGroups.begin(), lockedGroups.end(),
                                                    clip->group) != lockedGroups.end();
                               }),
                clips.end());
}

/// What every geometry verb does to its target list before it starts: the
/// group comes with the clip, and a lock refuses the lot.
void resolveTargets(const Project& project, ClipIds& clips)
{
    expandToGroups(project, clips);
    dropLocked(project, clips);
}

/// The track a row offset lands on, or an invalid id when it falls off either
/// end of the list — or onto a folder, which holds other tracks and never
/// clips. An invalid answer clamps the whole drag, exactly as running off the
/// end of the list does, so a group keeps its shape rather than half of it
/// landing somewhere a clip cannot live.
EntityId trackAtOffset(const Project& project, EntityId from, int delta)
{
    const std::size_t index = project.indexOfTrack(from);
    if (index == Project::notFound)
        return {};

    const long long target = static_cast<long long>(index) + delta;
    if (target < 0 || target >= static_cast<long long>(project.tracks().size()))
        return {};

    const Track& track = project.tracks()[static_cast<std::size_t>(target)];
    if (track.type == project::TrackType::folder)
        return {};

    return track.id;
}

} // namespace

// ── AddPatternClipCommand ─────────────────────────────────────────────────────

bool AddPatternClipCommand::execute(Project& project)
{
    if (!minted_) {
        const project::Pattern* pattern = project.findPattern(pattern_);
        const Track*            track   = project.findTrack(track_);

        // A folder groups tracks; it is not one. Clips live on the rows under
        // it, and one placed on the folder itself would play with no row to
        // draw it on.
        if (pattern == nullptr || track == nullptr
            || track->type == project::TrackType::folder)
            return false;

        Clip& created = project.addClip(project::ClipType::pattern, track_, pattern_);

        created.startTick   = std::max<Tick>(0, start_);
        created.lengthTicks = length_ > 0 ? length_ : pattern->length;
        created.name        = pattern->name;
        created.colour      = pattern->colour;
        created.lane        = freeLane(project, track_, created.startTick,
                                       created.lengthTicks, 0, created.id);

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
    resolveTargets(project, clips_);

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
    resolveTargets(project, clips_);

    if (clips_.empty() || (tickDelta_ == 0 && trackDelta_ == 0 && laneDelta_ == 0))
        return false;

    const engine::TempoMap& tempoMap = project.tempoMap();

    // Clamp once for the whole selection rather than per clip: clips keep their
    // relative positions when a drag hits the start of the timeline, which is
    // what dragging a group means. Audio clips take part through the D-013
    // accessor, so a mixed selection clamps as one.
    Tick tickDelta  = tickDelta_;
    int  trackDelta = trackDelta_;
    int  laneDelta  = laneDelta_;

    for (const EntityId id : clips_) {
        const Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        tickDelta = std::max(tickDelta, -project::clipStartTicks(*clip, tempoMap));

        if (trackDelta != 0 && !trackAtOffset(project, clip->track, trackDelta).isValid())
            trackDelta = 0;

        // Lane zero is the floor. There is no ceiling: dragging into the band
        // below the last used lane is how a lane comes into being, and the
        // count is derived from what the clips say rather than stored.
        laneDelta = std::max(laneDelta, -clip->lane);
    }

    if (tickDelta == 0 && trackDelta == 0 && laneDelta == 0)
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

        clip->lane = std::max(0, clip->lane + laneDelta);
    }

    appliedTickDelta_  = tickDelta;
    appliedTrackDelta_ = trackDelta;
    appliedLaneDelta_  = laneDelta;
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

        clip->lane = std::max(0, clip->lane - appliedLaneDelta_);
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
        appliedLaneDelta_  += other->appliedLaneDelta_;

        // The requested deltas must accumulate too: redo re-runs execute(),
        // and an execute that replayed only the gesture's first step would
        // desynchronise the history on the next undo. `movedAudio_` keeps
        // OUR snapshots — the gesture's starting positions.
        tickDelta_  += other->appliedTickDelta_;
        trackDelta_ += other->appliedTrackDelta_;
        laneDelta_  += other->appliedLaneDelta_;
    }
}

// ── ResizeClipsCommand ────────────────────────────────────────────────────────

bool ResizeClipsCommand::execute(Project& project)
{
    resolveTargets(project, clips_);

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
        // A copy of a group is a group — its own, not the original's, so that
        // moving the copy afterwards does not drag the source along with it.
        expandToGroups(project, clips_);

        std::vector<std::pair<EntityId, EntityId>> newGroups;

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

            if (copy.group.isValid()) {
                const auto found = std::find_if(
                    newGroups.begin(), newGroups.end(),
                    [&copy](const auto& pair) { return pair.first == copy.group; });

                if (found != newGroups.end()) {
                    copy.group = found->second;
                } else {
                    const EntityId minted = project.ids().next();
                    newGroups.emplace_back(copy.group, minted);
                    copy.group = minted;
                }
            }

            copy.id        = added.id;
            copy.track     = track;
            copy.startTick = std::max<Tick>(0, copy.startTick + tickDelta_);
            copy.lane      = freeLane(project, track,
                                      project::clipStartTicks(copy, project.tempoMap()),
                                      project::clipLengthTicks(copy, project.tempoMap()),
                                      copy.lane, added.id);
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
    resolveTargets(project, clips_);

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
    if (clip == nullptr || clip->locked)
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

// ── SetClipFadesCommand ───────────────────────────────────────────────────────

bool SetClipFadesCommand::execute(Project& project)
{
    expandToGroups(project, clips_);

    previous_.clear();

    bool changed = false;

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr || clip->type != project::ClipType::audio)
            continue;

        previous_.push_back({id, clip->fadeInFrames, clip->fadeOutFrames});

        // A fade longer than the clip would ramp past its own end.
        const auto limit = static_cast<long long>(clip->length);

        if (in_ >= 0) {
            const auto wanted = static_cast<project::FrameCount>(std::min(in_, limit));
            changed = changed || wanted != clip->fadeInFrames;
            clip->fadeInFrames = wanted;
        }

        if (out_ >= 0) {
            const auto wanted = static_cast<project::FrameCount>(std::min(out_, limit));
            changed = changed || wanted != clip->fadeOutFrames;
            clip->fadeOutFrames = wanted;
        }
    }

    return changed;
}

void SetClipFadesCommand::undo(Project& project)
{
    for (const Previous& entry : previous_) {
        if (Clip* clip = project.findClip(entry.id)) {
            clip->fadeInFrames  = entry.in;
            clip->fadeOutFrames = entry.out;
        }
    }
}

bool SetClipFadesCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetClipFadesCommand*>(&next);
    return other != nullptr && other->clips_ == clips_
        && (other->in_ >= 0) == (in_ >= 0) && (other->out_ >= 0) == (out_ >= 0);
}

void SetClipFadesCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetClipFadesCommand*>(&next)) {
        in_  = other->in_;
        out_ = other->out_;
    }
}

// ── CrossfadeClipsCommand ─────────────────────────────────────────────────────

bool CrossfadeClipsCommand::execute(Project& project)
{
    previous_.clear();

    // Every overlapping same-lane pair inside the selection. A two-clip
    // selection is the common case and falls straight out of this.
    std::vector<const Clip*> audio;
    for (const EntityId id : clips_) {
        const Clip* clip = project.findClip(id);
        if (clip != nullptr && clip->type == project::ClipType::audio)
            audio.push_back(clip);
    }

    std::sort(audio.begin(), audio.end(),
              [](const Clip* a, const Clip* b) { return a->start < b->start; });

    std::vector<std::pair<EntityId, EntityId>> pairs;

    for (std::size_t index = 0; index + 1 < audio.size(); ++index) {
        const Clip* earlier = audio[index];
        const Clip* later   = audio[index + 1];

        if (earlier->track != later->track || earlier->lane != later->lane)
            continue;

        const auto earlierEnd = earlier->start
                              + static_cast<project::FramePosition>(earlier->length);
        if (later->start >= earlierEnd || later->start <= earlier->start)
            continue;   // no overlap, or one swallows the other

        pairs.emplace_back(earlier->id, later->id);
    }

    if (pairs.empty())
        return false;

    const auto remember = [this, &project](EntityId id) {
        if (std::find_if(previous_.begin(), previous_.end(),
                         [id](const Previous& entry) { return entry.id == id; })
            != previous_.end())
            return;

        const Clip* clip = project.findClip(id);
        previous_.push_back({id, clip->crossfadeIn, clip->crossfadeOut});
    };

    bool changed = false;

    for (const auto& [earlierId, laterId] : pairs) {
        remember(earlierId);
        remember(laterId);

        Clip* earlier = project.findClip(earlierId);
        Clip* later   = project.findClip(laterId);

        changed = changed || earlier->crossfadeOut != crossfade_
               || later->crossfadeIn != crossfade_;

        earlier->crossfadeOut = crossfade_;
        later->crossfadeIn    = crossfade_;
    }

    return changed;
}

void CrossfadeClipsCommand::undo(Project& project)
{
    for (const Previous& entry : previous_) {
        if (Clip* clip = project.findClip(entry.id)) {
            clip->crossfadeIn  = entry.in;
            clip->crossfadeOut = entry.out;
        }
    }
}

// ── GroupClipsCommand ─────────────────────────────────────────────────────────

bool GroupClipsCommand::execute(Project& project)
{
    // Grouping a clip that is already in a group folds that whole group in:
    // the gesture means "these belong together", and half a group left behind
    // would be a group the user cannot see they still have.
    expandToGroups(project, clips_);

    previous_.clear();

    std::vector<EntityId> valid;
    for (const EntityId id : clips_)
        if (project.findClip(id) != nullptr)
            valid.push_back(id);

    // One clip is not a group.
    if (valid.size() < 2)
        return false;

    if (!group_.isValid())
        group_ = project.ids().next();

    bool changed = false;

    for (const EntityId id : valid) {
        Clip* clip = project.findClip(id);
        previous_.push_back({id, clip->group});
        changed    = changed || clip->group != group_;
        clip->group = group_;
    }

    return changed;
}

void GroupClipsCommand::undo(Project& project)
{
    for (const Previous& entry : previous_) {
        if (Clip* clip = project.findClip(entry.id))
            clip->group = entry.group;
    }
}

// ── UngroupClipsCommand ───────────────────────────────────────────────────────

bool UngroupClipsCommand::execute(Project& project)
{
    expandToGroups(project, clips_);

    previous_.clear();

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr || !clip->group.isValid())
            continue;

        previous_.push_back({id, clip->group});
        clip->group = {};
    }

    return !previous_.empty();
}

void UngroupClipsCommand::undo(Project& project)
{
    for (const Previous& entry : previous_) {
        if (Clip* clip = project.findClip(entry.id))
            clip->group = entry.group;
    }
}

// ── SetClipColourCommand ──────────────────────────────────────────────────────

bool SetClipColourCommand::execute(Project& project)
{
    expandToGroups(project, clips_);

    previous_.clear();

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr || clip->colour == colour_)
            continue;

        previous_.push_back({id, clip->colour});
        clip->colour = colour_;
    }

    return !previous_.empty();
}

void SetClipColourCommand::undo(Project& project)
{
    for (const Previous& entry : previous_) {
        if (Clip* clip = project.findClip(entry.id))
            clip->colour = entry.colour;
    }
}

// ── SetClipLockedCommand ──────────────────────────────────────────────────────

bool SetClipLockedCommand::execute(Project& project)
{
    previous_.clear();

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr || clip->locked == locked_)
            continue;

        previous_.push_back({id, clip->locked});
        clip->locked = locked_;
    }

    return !previous_.empty();
}

void SetClipLockedCommand::undo(Project& project)
{
    for (const Previous& entry : previous_) {
        if (Clip* clip = project.findClip(entry.id))
            clip->locked = entry.locked;
    }
}

// ── SetClipReversedCommand ────────────────────────────────────────────────────

bool SetClipReversedCommand::execute(Project& project)
{
    previous_.clear();

    for (const EntityId id : clips_) {
        Clip* clip = project.findClip(id);
        if (clip == nullptr || clip->type != project::ClipType::audio)
            continue;
        if (clip->reversed == reversed_)
            continue;

        previous_.push_back({id, clip->reversed});
        clip->reversed = reversed_;
    }

    return !previous_.empty();
}

void SetClipReversedCommand::undo(Project& project)
{
    for (const Previous& entry : previous_) {
        if (Clip* clip = project.findClip(entry.id))
            clip->reversed = entry.reversed;
    }
}

} // namespace incdaw::app
