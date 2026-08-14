#include "app/commands/ArrangementCommands.h"

#include <algorithm>

namespace incdaw::app {
namespace {

template <typename Entity>
[[nodiscard]] std::size_t indexOf(const std::vector<Entity>& entities, EntityId id) noexcept
{
    for (std::size_t index = 0; index < entities.size(); ++index)
        if (entities[index].id == id)
            return index;

    return entities.size();
}

template <typename Entity>
void restoreAt(std::vector<Entity>& entities, std::size_t index, Entity entity)
{
    const std::size_t position = std::min(index, entities.size());
    entities.insert(entities.begin() + static_cast<std::ptrdiff_t>(position), std::move(entity));
}

[[nodiscard]] Clip* findClipForEdit(Project& project, EntityId id) noexcept
{
    for (Clip& clip : project.clips())
        if (clip.id == id)
            return &clip;

    return nullptr;
}

[[nodiscard]] Track* findTrackForEdit(Project& project, EntityId id) noexcept
{
    for (Track& track : project.tracks())
        if (track.id == id)
            return &track;

    return nullptr;
}

} // namespace

// ── AddPatternClipCommand ─────────────────────────────────────────────────────

bool AddPatternClipCommand::execute(Project& project)
{
    const Pattern* pattern = project.findPattern(pattern_);
    if (pattern == nullptr)
        return false;

    const project::FrameCount length =
        length_ > 0 ? length_ : project.tempoMap().frameForTick(pattern->length);

    if (!created_.isValid()) {
        Clip& clip = project.addClip(project::ClipType::pattern, track_, pattern_);
        clip.start  = start_;
        clip.length = length;
        clip.name   = pattern->name;
        clip.colour = pattern->colour;
        created_    = clip.id;
        return true;
    }

    Clip clip;
    clip.id     = created_;
    clip.type   = project::ClipType::pattern;
    clip.track  = track_;
    clip.source = pattern_;
    clip.start  = start_;
    clip.length = length;
    clip.name   = pattern->name;
    clip.colour = pattern->colour;
    project.ids().observe(created_);
    project.clips().push_back(std::move(clip));
    return true;
}

void AddPatternClipCommand::undo(Project& project)
{
    auto& clips = project.clips();
    const std::size_t index = indexOf(clips, created_);

    if (index < clips.size())
        clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(index));
}

// ── DeleteClipCommand ─────────────────────────────────────────────────────────

bool DeleteClipCommand::execute(Project& project)
{
    auto& clips = project.clips();
    index_ = indexOf(clips, clip_);

    if (index_ >= clips.size())
        return false;

    removed_ = clips[index_];
    clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(index_));
    return true;
}

void DeleteClipCommand::undo(Project& project)
{
    restoreAt(project.clips(), index_, removed_);
}

// ── MoveClipCommand ───────────────────────────────────────────────────────────

bool MoveClipCommand::execute(Project& project)
{
    Clip* target = nullptr;

    for (Clip& clip : project.clips())
        if (clip.id == clip_)
            target = &clip;

    if (target == nullptr || target->locked)
        return false;

    const project::FramePosition start = std::max<project::FramePosition>(0, start_);
    const EntityId track = track_.isValid() ? track_ : target->track;

    if (target->start == start && target->track == track)
        return false;

    previousStart_ = target->start;
    previousTrack_ = target->track;

    target->start = start;
    target->track = track;
    return true;
}

void MoveClipCommand::undo(Project& project)
{
    for (Clip& clip : project.clips()) {
        if (clip.id != clip_)
            continue;

        clip.start = previousStart_;
        clip.track = previousTrack_;
        return;
    }
}

bool MoveClipCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const MoveClipCommand*>(&next);
    return other != nullptr && other->clip_ == clip_;
}

void MoveClipCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const MoveClipCommand*>(&next)) {
        start_ = other->start_;
        track_ = other->track_.isValid() ? other->track_ : track_;
    }
}

// ── ResizeClipCommand ─────────────────────────────────────────────────────────

bool ResizeClipCommand::execute(Project& project)
{
    Clip* clip = findClipForEdit(project, clip_);
    if (clip == nullptr || clip->locked)
        return false;

    // One frame is the floor. A zero-length clip is invisible and ungrabbable,
    // which reads to the user as a clip that was deleted by a drag.
    const FrameCount length = std::max<FrameCount>(1, length_);

    if (clip->length == length)
        return false;

    previous_     = clip->length;
    clip->length  = length;
    return true;
}

void ResizeClipCommand::undo(Project& project)
{
    if (Clip* clip = findClipForEdit(project, clip_))
        clip->length = previous_;
}

bool ResizeClipCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const ResizeClipCommand*>(&next);
    return other != nullptr && other->clip_ == clip_;
}

void ResizeClipCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const ResizeClipCommand*>(&next))
        length_ = other->length_;
}

// ── SplitClipCommand ──────────────────────────────────────────────────────────

bool SplitClipCommand::execute(Project& project)
{
    Clip* clip = findClipForEdit(project, clip_);
    if (clip == nullptr || clip->locked)
        return false;

    const FrameCount length = clip->length > 0 ? clip->length : 0;

    // A cut at or outside the edges would produce an empty half.
    if (length <= 1 || at_ <= clip->start || at_ >= clip->start + static_cast<FramePosition>(length))
        return false;

    const FrameCount leftLength = static_cast<FrameCount>(at_ - clip->start);

    Clip right = *clip;
    right.start        = at_;
    right.length       = length - leftLength;
    right.sourceOffset = clip->sourceOffset + leftLength;

    // A fade belongs to the edge it was drawn on: the left half keeps the fade
    // in, the right half keeps the fade out, and neither inherits the other's.
    right.fadeInFrames = 0;
    clip->fadeOutFrames = 0;

    originalLength_ = length;
    clip->length    = leftLength;

    if (!created_.isValid())
        created_ = project.ids().next();

    right.id = created_;
    project.ids().observe(created_);
    project.clips().push_back(std::move(right));
    return true;
}

void SplitClipCommand::undo(Project& project)
{
    auto& clips = project.clips();
    const std::size_t index = indexOf(clips, created_);

    if (index < clips.size())
        clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(index));

    if (Clip* clip = findClipForEdit(project, clip_))
        clip->length = originalLength_;
}

// ── DuplicateClipCommand ──────────────────────────────────────────────────────

bool DuplicateClipCommand::execute(Project& project)
{
    const Clip* source = project.findClip(clip_);
    if (source == nullptr)
        return false;

    Clip copy = *source;
    copy.start = source->start + static_cast<FramePosition>(std::max<FrameCount>(1, source->length));

    if (!created_.isValid())
        created_ = project.ids().next();

    copy.id = created_;
    project.ids().observe(created_);
    project.clips().push_back(std::move(copy));
    return true;
}

void DuplicateClipCommand::undo(Project& project)
{
    auto& clips = project.clips();
    const std::size_t index = indexOf(clips, created_);

    if (index < clips.size())
        clips.erase(clips.begin() + static_cast<std::ptrdiff_t>(index));
}

// ── SetClipValueCommand ───────────────────────────────────────────────────────

bool SetClipValueCommand::execute(Project& project)
{
    Clip* clip = findClipForEdit(project, clip_);
    if (clip == nullptr)
        return false;

    const double value = property_ == Property::gain ? std::clamp(value_, 0.0, 2.0)
                                                     : std::clamp(value_, -1.0, 1.0);

    double& target = property_ == Property::gain ? clip->gain : clip->pan;

    if (target == value)
        return false;

    previous_ = target;
    target    = value;
    return true;
}

void SetClipValueCommand::undo(Project& project)
{
    Clip* clip = findClipForEdit(project, clip_);
    if (clip == nullptr)
        return;

    (property_ == Property::gain ? clip->gain : clip->pan) = previous_;
}

bool SetClipValueCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetClipValueCommand*>(&next);
    return other != nullptr && other->clip_ == clip_ && other->property_ == property_;
}

void SetClipValueCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetClipValueCommand*>(&next))
        value_ = other->value_;
}

// ── SetClipFlagCommand ────────────────────────────────────────────────────────

const char* SetClipFlagCommand::id() const noexcept
{
    switch (flag_) {
        case Flag::muted:     return "playlist.setClipMuted";
        case Flag::locked:    return "playlist.setClipLocked";
        case Flag::normalize: return "playlist.setClipNormalize";
    }

    return "playlist.setClipFlag";
}

std::string SetClipFlagCommand::name() const
{
    switch (flag_) {
        case Flag::muted:     return "Mute Clip";
        case Flag::locked:    return "Lock Clip";
        case Flag::normalize: return "Normalize Clip";
    }

    return "Set Clip Flag";
}

bool SetClipFlagCommand::execute(Project& project)
{
    Clip* clip = findClipForEdit(project, clip_);
    if (clip == nullptr)
        return false;

    // Locking is the one flag a locked clip still accepts, or it could never be
    // unlocked again.
    if (clip->locked && flag_ != Flag::locked)
        return false;

    bool* target = nullptr;

    switch (flag_) {
        case Flag::muted:     target = &clip->muted; break;
        case Flag::locked:    target = &clip->locked; break;
        case Flag::normalize: target = &clip->normalize; break;
    }

    if (target == nullptr || *target == value_)
        return false;

    previous_ = *target;
    *target   = value_;
    return true;
}

void SetClipFlagCommand::undo(Project& project)
{
    Clip* clip = findClipForEdit(project, clip_);
    if (clip == nullptr)
        return;

    switch (flag_) {
        case Flag::muted:     clip->muted = previous_; break;
        case Flag::locked:    clip->locked = previous_; break;
        case Flag::normalize: clip->normalize = previous_; break;
    }
}

// ── AddTrackCommand ───────────────────────────────────────────────────────────

bool AddTrackCommand::execute(Project& project)
{
    if (!created_.isValid()) {
        created_ = project.addTrack(type_, name_).id;
        return true;
    }

    Track track;
    track.id              = created_;
    track.type            = type_;
    track.name            = name_;
    track.outputMixerNode = project.masterMixerNode();
    project.ids().observe(created_);
    project.tracks().push_back(std::move(track));
    return true;
}

void AddTrackCommand::undo(Project& project)
{
    auto& tracks = project.tracks();
    const std::size_t index = indexOf(tracks, created_);

    if (index < tracks.size())
        tracks.erase(tracks.begin() + static_cast<std::ptrdiff_t>(index));
}

// ── DeleteTrackCommand ────────────────────────────────────────────────────────

bool DeleteTrackCommand::execute(Project& project)
{
    auto& tracks = project.tracks();
    index_ = indexOf(tracks, track_);

    if (index_ >= tracks.size())
        return false;

    removed_ = tracks[index_];
    tracks.erase(tracks.begin() + static_cast<std::ptrdiff_t>(index_));

    removedClips_.clear();
    auto& clips = project.clips();

    for (const Clip& clip : clips)
        if (clip.track == track_)
            removedClips_.push_back(clip);

    clips.erase(std::remove_if(clips.begin(), clips.end(),
                               [this](const Clip& clip) { return clip.track == track_; }),
                clips.end());

    return true;
}

void DeleteTrackCommand::undo(Project& project)
{
    restoreAt(project.tracks(), index_, removed_);

    for (const Clip& clip : removedClips_)
        project.clips().push_back(clip);
}

// ── RenameTrackCommand ────────────────────────────────────────────────────────

bool RenameTrackCommand::execute(Project& project)
{
    Track* track = findTrackForEdit(project, track_);
    if (track == nullptr || track->name == name_)
        return false;

    previous_   = track->name;
    track->name = name_;
    return true;
}

void RenameTrackCommand::undo(Project& project)
{
    if (Track* track = findTrackForEdit(project, track_))
        track->name = previous_;
}

// ── SetTrackFlagCommand ───────────────────────────────────────────────────────

bool SetTrackFlagCommand::execute(Project& project)
{
    Track* track = findTrackForEdit(project, track_);
    if (track == nullptr)
        return false;

    bool& target = flag_ == Flag::muted ? track->muted : track->soloed;

    if (target == value_)
        return false;

    previous_ = target;
    target    = value_;
    return true;
}

void SetTrackFlagCommand::undo(Project& project)
{
    Track* track = findTrackForEdit(project, track_);
    if (track == nullptr)
        return;

    (flag_ == Flag::muted ? track->muted : track->soloed) = previous_;
}

// ── SetTrackParentCommand ─────────────────────────────────────────────────────

bool SetTrackParentCommand::execute(Project& project)
{
    Track* track = findTrackForEdit(project, track_);
    if (track == nullptr || track->parent == parent_ || parent_ == track_)
        return false;

    // A cycle would make `trackIsAudible` walk forever, and there is no sane
    // interpretation of a folder that contains itself.
    EntityId walker = parent_;

    for (int depth = 0; depth < 64 && walker.isValid(); ++depth) {
        if (walker == track_)
            return false;

        const Track* entry = project.findTrack(walker);
        if (entry == nullptr)
            break;

        walker = entry->parent;
    }

    previous_     = track->parent;
    track->parent = parent_;
    return true;
}

void SetTrackParentCommand::undo(Project& project)
{
    if (Track* track = findTrackForEdit(project, track_))
        track->parent = previous_;
}

} // namespace incdaw::app
