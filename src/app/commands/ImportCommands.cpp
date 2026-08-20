#include "app/commands/ImportCommands.h"

#include "plugins/PluginIdentifier.h"

#include <filesystem>

namespace incdaw::app {

namespace {

/// The file's name without its extension — what the user calls the sound.
std::string displayNameFor(const std::string& path)
{
    const std::string stem = std::filesystem::path{path}.stem().string();
    return stem.empty() ? std::string{"Sample"} : stem;
}

} // namespace

bool ImportSampleAsChannelCommand::execute(Project& project)
{
    if (!minted_) {
        // Probe first: an unreadable file leaves the project untouched.
        if (!importAudioAsset(project, path_, import_))
            return false;

        project::Channel& channel = project.addChannel(name_.empty() ? displayNameFor(path_) : name_);

        channel.instrument = plugins::builtinSampler();
        channel.instrumentStateFile.clear();

        project::ChannelSamplerZone zone;
        zone.asset            = import_.id;
        channel.samplerZones  = {zone};

        channel_      = channel;
        channelIndex_ = project.channels().size() - 1;
        minted_       = true;

        return true;
    }

    restoreImportedAsset(project, import_);
    project.insertChannel(channelIndex_, channel_);
    return true;
}

void ImportSampleAsChannelCommand::undo(Project& project)
{
    project.removeChannel(channel_.id);

    // The file on disk is never touched: undo takes the sample out of the
    // project, it does not delete the user's sample.
    if (import_.created)
        project.removeAudioAsset(import_.id);
}

bool ImportAudioClipCommand::execute(Project& project)
{
    if (project.findTrack(track_) == nullptr)
        return false;

    if (!minted_) {
        if (!importAudioAsset(project, path_, import_))
            return false;

        project::Clip& clip = project.addClip(project::ClipType::audio, track_, import_.id);

        clip.start        = project.tempoMap().frameForTick(startTick_);
        clip.length       = import_.asset.frameCount;
        clip.sourceOffset = 0;
        clip.name         = displayNameFor(path_);
        clip.colour       = 0xFF5588BBu;

        clip_      = clip;
        clipIndex_ = project.clips().size() - 1;
        minted_    = true;

        return true;
    }

    restoreImportedAsset(project, import_);
    project.insertClip(clipIndex_, clip_);
    return true;
}

void ImportAudioClipCommand::undo(Project& project)
{
    project.removeClip(clip_.id);

    if (import_.created)
        project.removeAudioAsset(import_.id);
}

} // namespace incdaw::app
