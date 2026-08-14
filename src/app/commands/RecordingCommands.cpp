#include "app/commands/RecordingCommands.h"

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

} // namespace incdaw::app
