#include "app/commands/RecordingCommands.h"

#include <algorithm>

namespace incdaw::app {

bool InsertRecordedTakeCommand::execute(Project& project)
{
    if (!minted_) {
        if (!placement_.succeeded || placement_.frameCount <= 0)
            return false;

        // A sliced take whose punch window excluded everything lands nothing
        // — the file stays on disk, but the arrangement is left alone.
        if (placement_.sliced && placement_.slices.empty())
            return false;

        // The first audio track takes recordings; without one, one appears.
        project::Track* target = nullptr;
        for (project::Track& track : project.tracks())
            if (track.type == project::TrackType::audio)
                target = target != nullptr ? target : &track;

        if (target == nullptr) {
            project::Track& created = project.addTrack(project::TrackType::audio, "Audio 1");
            trackIndex_   = project.tracks().size() - 1;
            track_        = created;
            trackCreated_ = true;
            target        = &created;
        }

        project::AudioAsset& asset = project.addAudioAsset(placement_.path.string());
        asset.sampleRate   = placement_.sampleRate;
        asset.frameCount   = placement_.frameCount;
        asset.channelCount = placement_.channelCount;

        asset_      = asset;
        assetIndex_ = project.audioAssets().size() - 1;

        // A straight take is one slice covering the whole file; a
        // loop-recorded one is one per pass, earlier passes muted.
        std::vector<project::RecordingSession::Slice> slices = placement_.slices;
        if (slices.empty())
            slices.push_back({placement_.startFrame, 0, placement_.frameCount, false});

        const std::string stem = placement_.path.stem().string();

        for (std::size_t index = 0; index < slices.size(); ++index) {
            const auto& slice = slices[index];

            project::Clip& clip = project.addClip(project::ClipType::audio,
                                                  target->id, asset_.id);
            clip.start        = slice.startFrame;
            clip.length       = slice.length;
            clip.sourceOffset = slice.sourceOffset;
            clip.muted        = slice.muted;
            clip.name         = slices.size() > 1
                                    ? stem + " #" + std::to_string(index + 1) : stem;
            clip.colour       = 0xFFCC7755u;   // recordings read as a family

            clips_.push_back(clip);
            clipIndices_.push_back(project.clips().size() - 1);
        }

        minted_ = true;
        return true;
    }

    // Redo: the same entities return with the same ids, so anything else on
    // the redo stack that references them stays valid.
    if (trackCreated_)
        project.insertTrack(trackIndex_, track_);

    project.insertAudioAsset(assetIndex_, asset_);

    for (std::size_t index = 0; index < clips_.size(); ++index)
        project.insertClip(clipIndices_[index], clips_[index]);

    return true;
}

void InsertRecordedTakeCommand::undo(Project& project)
{
    for (auto clip = clips_.rbegin(); clip != clips_.rend(); ++clip)
        (void)project.removeClip(clip->id);

    (void)project.removeAudioAsset(asset_.id);

    if (trackCreated_)
        (void)project.removeTrack(track_.id);
}

// ── Comping ───────────────────────────────────────────────────────────────────

namespace comping {
namespace {

using engine::FrameCount;
using engine::FramePosition;

[[nodiscard]] bool isAudioClipOn(const project::Clip& clip, project::EntityId track) noexcept
{
    return clip.track == track && clip.type == project::ClipType::audio && clip.length > 0;
}

/// `sourceOffset - start`: the constant that maps a timeline frame onto a
/// frame of the recording, and therefore identifies a take across the splits
/// comping makes.
[[nodiscard]] FrameCount anchorOf(const project::Clip& clip) noexcept
{
    return clip.sourceOffset - static_cast<FrameCount>(clip.start);
}

[[nodiscard]] bool overlaps(const project::Clip& clip, FramePosition from, FramePosition to) noexcept
{
    return clip.start < to && clip.start + clip.length > from;
}

} // namespace

std::vector<Take> takesOver(const Project& project, project::EntityId track,
                            FramePosition from, FramePosition to)
{
    std::vector<Take> takes;

    for (const project::Clip& clip : project.clips()) {
        if (!isAudioClipOn(clip, track) || !overlaps(clip, from, to))
            continue;

        const FrameCount anchor = anchorOf(clip);

        Take* existing = nullptr;
        for (Take& candidate : takes)
            if (candidate.source == clip.source && candidate.anchor == anchor)
                existing = &candidate;

        if (existing == nullptr) {
            Take take;
            take.source  = clip.source;
            take.anchor  = anchor;
            take.start   = clip.start;
            take.end     = clip.start + clip.length;
            take.audible = !clip.muted;
            takes.push_back(take);
            continue;
        }

        existing->start = std::min(existing->start, clip.start);
        existing->end   = std::max(existing->end, clip.start + clip.length);

        // Audible if ANY of its pieces inside the range is: a take that has
        // been comped in for half the range is not silent.
        existing->audible = existing->audible || !clip.muted;
    }

    // By anchor, which for loop recording is the order the passes were played
    // — each pass starts one loop later in the same file. Any other order
    // renumbers the lanes as soon as a take is muted.
    std::stable_sort(takes.begin(), takes.end(), [](const Take& left, const Take& right) {
        if (left.source.value() != right.source.value())
            return left.source.value() < right.source.value();

        return left.anchor < right.anchor;
    });

    return takes;
}

} // namespace comping

// ── AssignCompRangeCommand ────────────────────────────────────────────────────

bool AssignCompRangeCommand::execute(Project& project)
{
    if (!minted_) {
        if (to_ <= from_)
            return false;

        const auto takes = comping::takesOver(project, track_, from_, to_);
        if (takeIndex_ >= takes.size())
            return false;

        const auto& chosen = takes[takeIndex_];

        before_ = project.clips();

        std::vector<project::Clip> result;
        result.reserve(before_.size() + 4);

        bool changed = false;

        for (const project::Clip& clip : before_) {
            const bool mine = clip.track == track_ && clip.type == project::ClipType::audio
                           && clip.length > 0 && clip.start < to_
                           && clip.start + clip.length > from_;

            if (!mine) {
                result.push_back(clip);
                continue;
            }

            const engine::FramePosition clipEnd = clip.start + clip.length;
            const engine::FrameCount    anchor  = clip.sourceOffset
                                               - static_cast<engine::FrameCount>(clip.start);

            const bool isChosen = clip.source == chosen.source && anchor == chosen.anchor;

            // Three pieces at most: before the range, inside it, after it. The
            // outside pieces keep whatever the previous assignments decided —
            // comping one bar must not undo the bar beside it.
            const engine::FramePosition cuts[] = {clip.start, std::max(clip.start, from_),
                                                  std::min(clipEnd, to_), clipEnd};

            for (std::size_t index = 0; index + 1 < 4; ++index) {
                const engine::FramePosition pieceStart = cuts[index];
                const engine::FramePosition pieceEnd   = cuts[index + 1];

                if (pieceEnd <= pieceStart)
                    continue;

                project::Clip piece = clip;
                piece.start        = pieceStart;
                piece.length       = pieceEnd - pieceStart;
                piece.sourceOffset = anchor + static_cast<engine::FrameCount>(pieceStart);

                const bool inside = pieceStart >= from_ && pieceEnd <= to_;

                if (inside)
                    piece.muted = !isChosen;

                // A split piece is a new clip and needs its own id; the first
                // piece keeps the original's so an id held elsewhere still
                // resolves to something.
                if (pieceStart != clip.start)
                    piece.id = project.ids().next();

                if (piece.muted != clip.muted || piece.length != clip.length)
                    changed = true;

                result.push_back(piece);
            }
        }

        if (!changed)
            return false;   // the chosen take was already the audible one here

        after_  = std::move(result);
        minted_ = true;
    }

    project.clips() = after_;
    return true;
}

void AssignCompRangeCommand::undo(Project& project)
{
    if (minted_)
        project.clips() = before_;
}

} // namespace incdaw::app
