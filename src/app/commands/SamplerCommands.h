#pragma once

#include "app/AudioAssetImport.h"
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
    [[nodiscard]] EntityId assetId() const noexcept { return import_.id; }

private:
    EntityId    channelId_;
    std::string path_;

    // Captured on the first execute and replayed on redo, id included —
    // minting a fresh asset id per redo would orphan the zones written by
    // commands above this one in the redo stack.
    bool             minted_ = false;
    AudioAssetImport import_;

    plugins::PluginIdentifier                previousInstrument_;
    std::string                              previousStateFile_;
    std::vector<project::ChannelSamplerZone> previousZones_;
};

/// Appends one full-range zone for a sample file — the zone editor's layer
/// verb, meaningful only on a channel that already is the builtin sampler
/// (turning a channel INTO a sampler is LoadSampleCommand's whole-gesture
/// job). Shares its asset rule: a file already in the project is referenced,
/// never duplicated; undo removes the zone, and the asset only if this
/// command created it.
class AddSamplerZoneCommand final : public Command {
public:
    AddSamplerZoneCommand(EntityId channel, std::string filePath)
        : channelId_(channel), path_(std::move(filePath)) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.addSamplerZone"; }
    [[nodiscard]] std::string name() const override { return "Add Sampler Zone"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    channelId_;
    std::string path_;

    bool                minted_    = false;
    bool                created_   = false;
    EntityId            assetId_;
    project::AudioAsset asset_;
    std::size_t         assetIndex_ = 0;
    std::size_t         zoneIndex_  = 0;
};

/// Replaces one zone wholesale — the zone editor's edit verb. Whole-struct
/// replacement keeps undo trivial and merging honest: consecutive edits of
/// the same zone collapse into one entry, so nudging a field is one undo.
class SetSamplerZoneCommand final : public Command {
public:
    SetSamplerZoneCommand(EntityId channel, std::size_t zoneIndex,
                          project::ChannelSamplerZone zone)
        : channelId_(channel), zoneIndex_(zoneIndex), zone_(zone) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.setSamplerZone"; }
    [[nodiscard]] std::string name() const override { return "Edit Sampler Zone"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId                    channelId_;
    std::size_t                 zoneIndex_ = 0;
    project::ChannelSamplerZone zone_;
    project::ChannelSamplerZone previousZone_;
};

/// Removes one zone. Undo restores it at the same index, so the zones above
/// it keep the positions other commands in the stack refer to.
class RemoveSamplerZoneCommand final : public Command {
public:
    RemoveSamplerZoneCommand(EntityId channel, std::size_t zoneIndex)
        : channelId_(channel), zoneIndex_(zoneIndex) {}

    [[nodiscard]] const char* id() const noexcept override
    {
        return "channel.removeSamplerZone";
    }
    [[nodiscard]] std::string name() const override { return "Remove Sampler Zone"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId                    channelId_;
    std::size_t                 zoneIndex_ = 0;
    project::ChannelSamplerZone removed_;
};

} // namespace incdaw::app
