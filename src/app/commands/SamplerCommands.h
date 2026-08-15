#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <string>
#include <vector>

namespace incdaw::app {

using project::EntityId;

/// Loads a sample file onto a channel, making it a builtin sampler.
///
/// One command performs the whole gesture — ensure an AudioAsset for the
/// file (probing the header for metadata), give the channel the builtin
/// sampler identity, and write a single full-range zone rooted at middle C —
/// because that is what the user did: "load this sample here". Undo restores
/// the channel's previous instrument and zones, and removes the asset again
/// only if this command created it; a file already in the project is shared,
/// not duplicated.
class LoadSampleCommand final : public Command {
public:
    LoadSampleCommand(EntityId channel, std::string filePath)
        : channelId_(channel), path_(std::move(filePath)) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.loadSample"; }
    [[nodiscard]] std::string name() const override { return "Load Sample"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first successful execute.
    [[nodiscard]] EntityId assetId() const noexcept { return assetId_; }

private:
    EntityId    channelId_;
    std::string path_;

    // Captured on the first execute and replayed on redo, id included —
    // minting a fresh asset id per redo would orphan the zones written by
    // commands above this one in the redo stack.
    bool                minted_  = false;
    bool                created_ = false;
    EntityId            assetId_;
    project::AudioAsset asset_;
    std::size_t         assetIndex_ = 0;

    plugins::PluginIdentifier                previousInstrument_;
    std::string                              previousStateFile_;
    std::vector<project::ChannelSamplerZone> previousZones_;
};

} // namespace incdaw::app
