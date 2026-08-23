#include "app/commands/AudioEditCommands.h"

#include "engine/dsp/TimeStretch.h"
#include "engine/audio/WavFile.h"

#include <algorithm>

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
        markersBefore_ = data.markers;
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

    // The markers as they were. Restored rather than un-shifted: a marker the
    // edit removed cannot be derived from the ones it left behind.
    data.markers = markersBefore_;

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
        markersBefore_ = data.markers;
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

    // The markers as they were. Restored rather than un-shifted: a marker the
    // edit removed cannot be derived from the ones it left behind.
    restored.markers = markersBefore_;

    if (WavFile::write(assetFilePath(*asset), restored))
        asset->frameCount = previousFrameCount_;
}

// ── DeleteAudioRegionCommand ──────────────────────────────────────────────────

bool DeleteAudioRegionCommand::execute(Project& project)
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
        applied_ = engine::edits::clampedRegion(data, region_);

        // Deleting nothing is not an edit; deleting everything destroys the
        // asset — both are refusals, like Trim's.
        if (applied_.length() <= 0 || applied_.length() == data.frameCount)
            return false;

        removed_ = snapshotRegion(data, applied_);
        minted_  = true;
        markersBefore_ = data.markers;
    }

    engine::edits::deleteRegion(data, applied_);

    if (!WavFile::write(assetFilePath(*asset), data))
        return false;

    asset->frameCount = data.frameCount;
    return true;
}

void DeleteAudioRegionCommand::undo(Project& project)
{
    project::AudioAsset* asset = nullptr;
    for (project::AudioAsset& candidate : project.audioAssets())
        if (candidate.id == asset_)
            asset = &candidate;

    if (asset == nullptr)
        return;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return;

    AudioFileData piece;
    piece.sampleRate   = data.sampleRate;
    piece.channelCount = data.channelCount;
    piece.frameCount   = applied_.length();
    piece.channels     = removed_;

    if (!engine::edits::insertAudio(data, applied_.from, piece))
        return;

    // The markers as they were. Restored rather than un-shifted: a marker the
    // edit removed cannot be derived from the ones it left behind.
    data.markers = markersBefore_;

    if (WavFile::write(assetFilePath(*asset), data))
        asset->frameCount = data.frameCount;
}

// ── InsertAudioCommand ────────────────────────────────────────────────────────

bool InsertAudioCommand::execute(Project& project)
{
    project::AudioAsset* asset = nullptr;
    for (project::AudioAsset& candidate : project.audioAssets())
        if (candidate.id == asset_)
            asset = &candidate;

    if (asset == nullptr || piece_.frameCount <= 0)
        return false;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return false;

    if (!minted_) {
        insertedAt_ = std::min<engine::FramePosition>(
            at_, static_cast<engine::FramePosition>(data.frameCount));
        minted_ = true;
        markersBefore_ = data.markers;
    }

    // Rate or channel mismatch refuses here, before any write.
    if (!engine::edits::insertAudio(data, insertedAt_, piece_))
        return false;

    if (!WavFile::write(assetFilePath(*asset), data))
        return false;

    asset->frameCount = data.frameCount;
    return true;
}

void InsertAudioCommand::undo(Project& project)
{
    project::AudioAsset* asset = nullptr;
    for (project::AudioAsset& candidate : project.audioAssets())
        if (candidate.id == asset_)
            asset = &candidate;

    if (asset == nullptr)
        return;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return;

    engine::edits::deleteRegion(
        data, {static_cast<FrameCount>(insertedAt_),
               static_cast<FrameCount>(insertedAt_) + piece_.frameCount});

    // The markers as they were. Restored rather than un-shifted: a marker the
    // edit removed cannot be derived from the ones it left behind.
    data.markers = markersBefore_;

    if (WavFile::write(assetFilePath(*asset), data))
        asset->frameCount = data.frameCount;
}

// ── StretchAssetCommand ───────────────────────────────────────────────────────

namespace {

/// The file with [from, from + spanLength) replaced by `replacement`.
AudioFileData withSpanReplaced(const AudioFileData& data, FrameCount from, FrameCount spanLength,
                               const std::vector<std::vector<Sample>>& replacement)
{
    const auto replacementFrames =
        replacement.empty() ? FrameCount{0} : static_cast<FrameCount>(replacement[0].size());

    AudioFileData result;
    result.sampleRate   = data.sampleRate;
    result.channelCount = data.channelCount;
    result.frameCount   = data.frameCount - spanLength + replacementFrames;
    result.channels.assign(result.channelCount,
                           std::vector<Sample>(static_cast<std::size_t>(result.frameCount)));

    for (std::size_t channel = 0; channel < result.channelCount; ++channel) {
        auto destination = result.channels[channel].begin();

        destination = std::copy(data.channels[channel].begin(),
                                data.channels[channel].begin()
                                    + static_cast<std::ptrdiff_t>(from),
                                destination);

        if (channel < replacement.size())
            destination = std::copy(replacement[channel].begin(), replacement[channel].end(),
                                    destination);

        std::copy(data.channels[channel].begin()
                      + static_cast<std::ptrdiff_t>(from + spanLength),
                  data.channels[channel].end(), destination);
    }

    // Markers ride along, shifted by however much the span grew or shrank.
    // Building a fresh AudioFileData and forgetting them is how a stretch
    // used to silently strip every cue in the file.
    result.markers = data.markers;
    engine::edits::shiftMarkers(result, from + spanLength, replacementFrames - spanLength);

    return result;
}

} // namespace

bool StretchAssetCommand::execute(Project& project)
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
        applied_ = engine::edits::clampedRegion(data, region_);
        if (applied_.length() <= 0 || (ratio_ == 1.0 && pitchSemitones_ == 0.0))
            return false;

        before_ = snapshotRegion(data, applied_);

        AudioFileData regionData;
        regionData.sampleRate   = data.sampleRate;
        regionData.channelCount = data.channelCount;
        regionData.frameCount   = applied_.length();
        regionData.channels     = before_;

        engine::dsp::StretchOptions options;
        options.ratio          = ratio_;
        options.pitchSemitones = pitchSemitones_;

        after_  = engine::dsp::timeStretch(regionData, options).channels;
        minted_ = true;
        markersBefore_ = data.markers;
    }

    const AudioFileData spliced =
        withSpanReplaced(data, applied_.from, applied_.length(), after_);

    if (!WavFile::write(assetFilePath(*asset), spliced))
        return false;

    asset->frameCount = spliced.frameCount;
    return true;
}

void StretchAssetCommand::undo(Project& project)
{
    project::AudioAsset* asset = nullptr;
    for (project::AudioAsset& candidate : project.audioAssets())
        if (candidate.id == asset_)
            asset = &candidate;

    if (asset == nullptr)
        return;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return;

    const auto renderedFrames =
        after_.empty() ? FrameCount{0} : static_cast<FrameCount>(after_[0].size());

    AudioFileData restored =
        withSpanReplaced(data, applied_.from, renderedFrames, before_);

    // The markers as they were. Restored rather than un-shifted: a marker the
    // edit removed cannot be derived from the ones it left behind.
    restored.markers = markersBefore_;

    if (WavFile::write(assetFilePath(*asset), restored))
        asset->frameCount = restored.frameCount;
}

// ── DenoiseAssetCommand ───────────────────────────────────────────────────────

bool DenoiseAssetCommand::execute(Project& project)
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
        applied_ = engine::edits::clampedRegion(data, region_);

        if (applied_.length() <= 0 || profile_.isEmpty())
            return false;

        before_ = snapshotRegion(data, applied_);

        AudioFileData working = data;
        if (!engine::dsp::denoise(working, applied_.from, applied_.to, profile_, amount_))
            return false;

        after_  = snapshotRegion(working, applied_);
        minted_ = true;
    }

    // Redo writes the recorded RESULT rather than re-running the pass: the
    // arithmetic is deterministic, but re-running it is seconds of work for a
    // keystroke that should be instant.
    restoreRegion(data, applied_, after_);

    return bool(WavFile::write(assetFilePath(*asset), data));
}

void DenoiseAssetCommand::undo(Project& project)
{
    const project::AudioAsset* asset = findAsset(project, asset_);
    if (asset == nullptr)
        return;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return;

    // The recorded samples, not the noise added back. Spectral subtraction is
    // not invertible, and returning audio that is merely similar to what the
    // user had is not undo.
    restoreRegion(data, applied_, before_);
    (void)WavFile::write(assetFilePath(*asset), data);
}

// ── SetAudioMarkersCommand ────────────────────────────────────────────────────

namespace {

/// Sorted by position, which is the order the file stores them in and the
/// order the editor draws them in.
void sortMarkers(std::vector<engine::AudioMarker>& markers)
{
    std::stable_sort(markers.begin(), markers.end(),
                     [](const engine::AudioMarker& left, const engine::AudioMarker& right) {
                         return left.start < right.start;
                     });
}

} // namespace

bool SetAudioMarkersCommand::execute(Project& project)
{
    const project::AudioAsset* asset = findAsset(project, asset_);
    if (asset == nullptr)
        return false;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return false;

    if (!minted_) {
        markersBefore_ = data.markers;
        sortMarkers(markers_);

        // Setting the list to what it already is is not an edit, and an undo
        // entry that changes nothing is worse than no entry at all.
        if (markers_ == markersBefore_)
            return false;

        minted_ = true;
    }

    data.markers = markers_;
    return bool(WavFile::write(assetFilePath(*asset), data));
}

void SetAudioMarkersCommand::undo(Project& project)
{
    const project::AudioAsset* asset = findAsset(project, asset_);
    if (asset == nullptr)
        return;

    AudioFileData data;
    if (!WavFile::read(assetFilePath(*asset), data))
        return;

    data.markers = markersBefore_;
    (void)WavFile::write(assetFilePath(*asset), data);
}

} // namespace incdaw::app
