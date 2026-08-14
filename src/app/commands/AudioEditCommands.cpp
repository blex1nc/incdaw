#include "app/commands/AudioEditCommands.h"

#include "engine/audio/WavFile.h"

namespace incdaw::app {
namespace {

using engine::AudioFileData;
using engine::FrameCount;
using engine::Sample;
using engine::WavFile;
using engine::edits::Region;

const project::AudioAsset* findAsset(const Project& project, project::EntityId id)
{
    for (const project::AudioAsset& asset : project.audioAssets())
        if (asset.id == id)
            return &asset;

    return nullptr;
}

std::string assetFilePath(const project::AudioAsset& asset)
{
    return !asset.absolutePath.empty() ? asset.absolutePath : asset.relativePath;
}

/// Copies [region) of every channel.
std::vector<std::vector<Sample>> snapshotRegion(const AudioFileData& data, Region region)
{
    std::vector<std::vector<Sample>> snapshot;
    snapshot.reserve(data.channelCount);

    for (const auto& channel : data.channels)
        snapshot.emplace_back(channel.begin() + static_cast<std::ptrdiff_t>(region.from),
                              channel.begin() + static_cast<std::ptrdiff_t>(region.to));

    return snapshot;
}

void restoreRegion(AudioFileData& data, Region region,
                   const std::vector<std::vector<Sample>>& snapshot)
{
    for (std::size_t channel = 0; channel < data.channelCount && channel < snapshot.size();
         ++channel) {
        std::copy(snapshot[channel].begin(), snapshot[channel].end(),
                  data.channels[channel].begin() + static_cast<std::ptrdiff_t>(region.from));
    }
}

} // namespace

// ── EditAssetRegionCommand ────────────────────────────────────────────────────

std::string EditAssetRegionCommand::name() const
{
    switch (op_) {
        case AudioEditOp::gain:      return "Gain";
        case AudioEditOp::normalize: return "Normalize";
        case AudioEditOp::reverse:   return "Reverse";
        case AudioEditOp::silence:   return "Silence";
        case AudioEditOp::fadeIn:    return "Fade In";
        case AudioEditOp::fadeOut:   return "Fade Out";
    }
    return "Edit Audio";
}

bool EditAssetRegionCommand::execute(Project& project)
{
    const project::AudioAsset* asset = findAsset(project, asset_);
    if (asset == nullptr)
        return false;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return false;

    if (!minted_) {
        applied_ = engine::edits::clampedRegion(data, region_);
        if (applied_.length() <= 0)
            return false;

        before_ = snapshotRegion(data, applied_);

        bool changed = true;
        switch (op_) {
            case AudioEditOp::gain:      engine::edits::applyGain(data, applied_, factor_); break;
            case AudioEditOp::normalize: changed = engine::edits::normalize(data, applied_, factor_); break;
            case AudioEditOp::reverse:   engine::edits::reverse(data, applied_); break;
            case AudioEditOp::silence:   engine::edits::silence(data, applied_); break;
            case AudioEditOp::fadeIn:    engine::edits::fadeIn(data, applied_); break;
            case AudioEditOp::fadeOut:   engine::edits::fadeOut(data, applied_); break;
        }

        if (!changed) {
            before_.clear();
            return false;   // normalizing silence: refused, nothing to undo
        }

        after_  = snapshotRegion(data, applied_);
        minted_ = true;
    } else {
        // Redo: the recorded result, not a re-derivation.
        restoreRegion(data, applied_, after_);
    }

    return bool(WavFile::write(assetFilePath(*asset), data));
}

void EditAssetRegionCommand::undo(Project& project)
{
    const project::AudioAsset* asset = findAsset(project, asset_);
    if (asset == nullptr)
        return;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return;

    restoreRegion(data, applied_, before_);
    (void)WavFile::write(assetFilePath(*asset), data);
}

// ── TrimAssetCommand ──────────────────────────────────────────────────────────

bool TrimAssetCommand::execute(Project& project)
{
    project::AudioAsset* asset = nullptr;
    for (project::AudioAsset& candidate : project.audioAssets())
        if (candidate.id == asset_)
            asset = &candidate;

    if (asset == nullptr)
        return false;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return false;

    if (!minted_) {
        applied_ = engine::edits::clampedRegion(data, keep_);

        // Trimming everything away, or trimming nothing, are both refusals:
        // the first destroys the asset, the second is not an edit.
        if (applied_.length() <= 0 || applied_.length() == data.frameCount)
            return false;

        previousFrameCount_ = data.frameCount;
        head_ = snapshotRegion(data, {0, applied_.from});
        tail_ = snapshotRegion(data, {applied_.to, data.frameCount});
        minted_ = true;
    }

    engine::edits::trimTo(data, applied_);

    if (!WavFile::write(assetFilePath(*asset), data))
        return false;

    asset->frameCount = data.frameCount;
    return true;
}

void TrimAssetCommand::undo(Project& project)
{
    project::AudioAsset* asset = nullptr;
    for (project::AudioAsset& candidate : project.audioAssets())
        if (candidate.id == asset_)
            asset = &candidate;

    if (asset == nullptr)
        return;

    AudioFileData trimmed;
    if (!WavFile::read(assetFilePath(*asset), trimmed))
        return;

    // Reassemble head + kept + tail. The kept part is the file as it stands.
    AudioFileData restored;
    restored.sampleRate   = trimmed.sampleRate;
    restored.channelCount = trimmed.channelCount;
    restored.frameCount   = previousFrameCount_;
    restored.channels.assign(restored.channelCount,
                             std::vector<Sample>(static_cast<std::size_t>(previousFrameCount_)));

    for (std::size_t channel = 0; channel < restored.channelCount; ++channel) {
        auto destination = restored.channels[channel].begin();

        if (channel < head_.size())
            destination = std::copy(head_[channel].begin(), head_[channel].end(), destination);

        destination = std::copy(trimmed.channels[channel].begin(),
                                trimmed.channels[channel].end(), destination);

        if (channel < tail_.size())
            std::copy(tail_[channel].begin(), tail_[channel].end(), destination);
    }

    if (WavFile::write(assetFilePath(*asset), restored))
        asset->frameCount = previousFrameCount_;
}

} // namespace incdaw::app
