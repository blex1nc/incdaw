#include "app/commands/RecordingCommands.h"

namespace incdaw::app {

bool InsertRecordedTakeCommand::execute(Project& project)
{
    if (!minted_) {
        if (!placement_.succeeded || placement_.frameCount <= 0)
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

        project::Clip& clip = project.addClip(project::ClipType::audio, target->id, asset.id);
        clip.start  = placement_.startFrame;
        clip.length = placement_.frameCount;
        clip.name   = placement_.path.stem().string();
        clip.colour = 0xFFCC7755u;   // recordings read as a family in the playlist

        asset_      = asset;
        clip_       = clip;
        assetIndex_ = project.audioAssets().size() - 1;
        clipIndex_  = project.clips().size() - 1;
        minted_     = true;
        return true;
    }

    // Redo: the same entities return with the same ids, so anything else on
    // the redo stack that references them stays valid.
    if (trackCreated_)
        project.insertTrack(trackIndex_, track_);

    project.insertAudioAsset(assetIndex_, asset_);
    project.insertClip(clipIndex_, clip_);
    return true;
}

void InsertRecordedTakeCommand::undo(Project& project)
{
    (void)project.removeClip(clip_.id);
    (void)project.removeAudioAsset(asset_.id);

    if (trackCreated_)
        (void)project.removeTrack(track_.id);
}

} // namespace incdaw::app
