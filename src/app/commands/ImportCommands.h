#pragma once

#include "app/AudioAssetImport.h"
#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <string>

namespace incdaw::app {

using project::EntityId;
using project::Tick;

/// A sample dropped from the Browser onto the Channel Rack's empty space.
///
/// One command for the whole gesture — the channel, the sampler identity and
/// the full-range zone — because that is what the user did: "put this sample
/// in the rack". Dropping onto an existing channel is LoadSampleCommand
/// instead; only the new-channel case needs this.
class ImportSampleAsChannelCommand final : public Command {
public:
    explicit ImportSampleAsChannelCommand(std::string filePath, std::string channelName = {})
        : path_(std::move(filePath)), name_(std::move(channelName)) {}

    [[nodiscard]] const char* id() const noexcept override { return "browser.importSample"; }
    [[nodiscard]] std::string name() const override { return "Add Sampler Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first successful execute.
    [[nodiscard]] EntityId channelId() const noexcept { return channel_.id; }
    [[nodiscard]] EntityId assetId() const noexcept { return import_.id; }

private:
    std::string path_;
    std::string name_;

    bool             minted_ = false;
    AudioAssetImport import_;

    // Captured on the first execute and replayed on redo, id included: a fresh
    // channel id per redo would orphan every edit made against this channel.
    project::Channel channel_;
    std::size_t      channelIndex_ = 0;
};

/// A sample dropped onto a playlist track: the file becomes an audio clip
/// starting where it was dropped, as long as the file itself.
///
/// The drop position arrives in TICKS, because that is what a timeline click
/// means; the clip stores frames, because an audio clip is anchored to the
/// recording rather than to the beat (docs/DECISIONS.md D-013). The conversion
/// belongs here, next to the placement, and not in the view.
class ImportAudioClipCommand final : public Command {
public:
    ImportAudioClipCommand(EntityId track, std::string filePath, Tick startTick)
        : track_(track), path_(std::move(filePath)), startTick_(startTick) {}

    [[nodiscard]] const char* id() const noexcept override { return "browser.importAudioClip"; }
    [[nodiscard]] std::string name() const override { return "Add Audio Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId clipId() const noexcept { return clip_.id; }
    [[nodiscard]] EntityId assetId() const noexcept { return import_.id; }

private:
    EntityId    track_;
    std::string path_;
    Tick        startTick_ = 0;

    bool             minted_ = false;
    AudioAssetImport import_;

    project::Clip clip_;
    std::size_t   clipIndex_ = 0;
};

} // namespace incdaw::app
