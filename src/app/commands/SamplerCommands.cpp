#include "app/commands/SamplerCommands.h"

#include "engine/audio/WavFile.h"

namespace incdaw::app {

namespace {

struct MintedAsset {
    bool                ok      = false;
    bool                created = false;
    EntityId            id;
    project::AudioAsset copy;
    std::size_t         index = 0;
};

/// The shared asset rule of every "bring this file in" command: probe the
/// header (never decode; failing before any mutation keeps an unreadable
/// file a clean refusal), reuse an asset that already names the file, mint
/// one otherwise. The caller keeps the result for its redo replay.
MintedAsset ensureAssetForFile(Project& project, const std::string& path)
{
    MintedAsset minted;

    engine::AudioFileData header;
    if (!engine::WavFile::probe(path, header))
        return minted;

    minted.ok = true;

    for (const project::AudioAsset& existing : project.audioAssets())
        if (existing.absolutePath == path)
            minted.id = existing.id;

    if (!minted.id.isValid()) {
        project::AudioAsset& added = project.addAudioAsset(path);
        added.sampleRate           = header.sampleRate;
        added.frameCount           = header.frameCount;
        added.channelCount         = header.channelCount;

        minted.created = true;
        minted.id      = added.id;
        minted.copy    = added;
        minted.index   = project.audioAssets().size() - 1;
    }

    return minted;
}

} // namespace

bool LoadSampleCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr)
        return false;

    if (!minted_) {
        const MintedAsset minted = ensureAssetForFile(project, path_);
        if (!minted.ok)
            return false;

        created_    = minted.created;
        assetId_    = minted.id;
        asset_      = minted.copy;
        assetIndex_ = minted.index;

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

// ── AddSamplerZoneCommand ─────────────────────────────────────────────────────

bool AddSamplerZoneCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || channel->instrument != plugins::builtinSampler())
        return false;

    if (!minted_) {
        const MintedAsset minted = ensureAssetForFile(project, path_);
        if (!minted.ok)
            return false;

        created_    = minted.created;
        assetId_    = minted.id;
        asset_      = minted.copy;
        assetIndex_ = minted.index;
        minted_     = true;
    } else if (created_ && project.indexOfAudioAsset(assetId_) == Project::notFound) {
        project.insertAudioAsset(assetIndex_, asset_);
    }

    project::ChannelSamplerZone zone;
    zone.asset = assetId_;

    zoneIndex_ = channel->samplerZones.size();
    channel->samplerZones.push_back(zone);
    return true;
}

void AddSamplerZoneCommand::undo(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || zoneIndex_ >= channel->samplerZones.size())
        return;

    channel->samplerZones.erase(channel->samplerZones.begin()
                                + static_cast<std::ptrdiff_t>(zoneIndex_));

    if (created_)
        project.removeAudioAsset(assetId_);
}

// ── SetSamplerZoneCommand ─────────────────────────────────────────────────────

bool SetSamplerZoneCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || zoneIndex_ >= channel->samplerZones.size())
        return false;

    if (channel->samplerZones[zoneIndex_] == zone_)
        return false;

    previousZone_                     = channel->samplerZones[zoneIndex_];
    channel->samplerZones[zoneIndex_] = zone_;
    return true;
}

void SetSamplerZoneCommand::undo(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel != nullptr && zoneIndex_ < channel->samplerZones.size())
        channel->samplerZones[zoneIndex_] = previousZone_;
}

bool SetSamplerZoneCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetSamplerZoneCommand*>(&next);
    return other != nullptr && other->channelId_ == channelId_
        && other->zoneIndex_ == zoneIndex_;
}

void SetSamplerZoneCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetSamplerZoneCommand*>(&next))
        zone_ = other->zone_;
}

// ── RemoveSamplerZoneCommand ──────────────────────────────────────────────────

bool RemoveSamplerZoneCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || zoneIndex_ >= channel->samplerZones.size())
        return false;

    removed_ = channel->samplerZones[zoneIndex_];
    channel->samplerZones.erase(channel->samplerZones.begin()
                                + static_cast<std::ptrdiff_t>(zoneIndex_));
    return true;
}

void RemoveSamplerZoneCommand::undo(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || zoneIndex_ > channel->samplerZones.size())
        return;

    channel->samplerZones.insert(channel->samplerZones.begin()
                                     + static_cast<std::ptrdiff_t>(zoneIndex_),
                                 removed_);
}

} // namespace incdaw::app
