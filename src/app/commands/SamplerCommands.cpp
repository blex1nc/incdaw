#include "app/commands/SamplerCommands.h"

#include "engine/audio/WavFile.h"

namespace incdaw::app {

bool LoadSampleCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr)
        return false;

    if (!minted_) {
        // Probe, never decode: the command needs the header's metadata, and
        // failing HERE — before any mutation — is what makes an unreadable
        // file a clean refusal rather than a half-applied edit.
        engine::AudioFileData header;
        if (!engine::WavFile::probe(path_, header))
            return false;

        assetId_ = EntityId{};
        for (const project::AudioAsset& existing : project.audioAssets())
            if (existing.absolutePath == path_)
                assetId_ = existing.id;

        if (!assetId_.isValid()) {
            project::AudioAsset& added = project.addAudioAsset(path_);
            added.sampleRate           = header.sampleRate;
            added.frameCount           = header.frameCount;
            added.channelCount         = header.channelCount;

            created_    = true;
            assetId_    = added.id;
            asset_      = added;
            assetIndex_ = project.audioAssets().size() - 1;
        }

        previousInstrument_ = channel->instrument;
        previousStateFile_  = channel->instrumentStateFile;
        previousZones_      = channel->samplerZones;
        minted_             = true;
    } else if (created_ && project.indexOfAudioAsset(assetId_) == Project::notFound) {
        project.insertAudioAsset(assetIndex_, asset_);
    }

    channel->instrument = plugins::builtinSampler();
    channel->instrumentStateFile.clear();

    // The default program: the whole file across the whole keyboard, rooted
    // at middle C — the note a bare click in the Piano Roll writes.
    project::ChannelSamplerZone zone;
    zone.asset            = assetId_;
    channel->samplerZones = {zone};

    return true;
}

void LoadSampleCommand::undo(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr)
        return;

    channel->instrument          = previousInstrument_;
    channel->instrumentStateFile = previousStateFile_;
    channel->samplerZones        = previousZones_;

    if (created_)
        project.removeAudioAsset(assetId_);
}

} // namespace incdaw::app
