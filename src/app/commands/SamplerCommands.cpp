#include "app/commands/SamplerCommands.h"


namespace incdaw::app {

bool LoadSampleCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr)
        return false;

    if (!minted_) {
        // Probes the header, never decodes, and fails BEFORE any mutation —
        // which is what makes an unreadable file a clean refusal rather than a
        // half-applied edit (app/AudioAssetImport.h).
        if (!importAudioAsset(project, path_, import_))
            return false;

        previousInstrument_ = channel->instrument;
        previousStateFile_  = channel->instrumentStateFile;
        previousZones_      = channel->samplerZones;
        minted_             = true;
    } else {
        restoreImportedAsset(project, import_);
    }

    channel->instrument = plugins::builtinSampler();
    channel->instrumentStateFile.clear();

    // The default program: the whole file across the whole keyboard, rooted
    // at middle C — the note a bare click in the Piano Roll writes.
    project::ChannelSamplerZone zone;
    zone.asset            = import_.id;
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

    if (import_.created)
        project.removeAudioAsset(import_.id);
}

} // namespace incdaw::app
