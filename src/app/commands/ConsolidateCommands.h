#pragma once

#include "app/AudioAssetImport.h"
#include "app/Command.h"
#include "app/commands/ClipCommands.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::project { class ParameterRegistry; }

namespace incdaw::app {

using project::Clip;
using project::EntityId;

/// Why `clips` cannot be consolidated, or empty when they can.
///
/// Separate from the command because a command that the registry rejects is
/// destroyed before the caller can ask it anything: the UI needs the reason in
/// order to say it, and both it and the command read it from here.
[[nodiscard]] std::string consolidationRefusal(const Project& project, const ClipIds& clips);

/// Renders a selection of clips on one track down to a single audio clip.
///
/// The render goes through project::renderProject — the same compiler and the
/// same graph the engine plays, driven block by block — so a consolidation
/// sounds like what it replaced rather than like a second implementation of
/// it. Nothing in OfflineRender is touched: the command hands it a stripped
/// COPY of the project holding only the selected clips, and asks for exactly
/// their span.
///
/// The copy also flattens every mixer node to unity with no inserts. That is
/// what makes the operation lossless: the result is the clips' own audio —
/// gain, pan, fades, reverse, stretch, normalize and, for pattern clips, the
/// instrument — and dropping it back on the same track through the same strip
/// reproduces what was there. Rendering the strip INTO the clip and then
/// playing it through that strip again would apply it twice.
///
/// One undo entry puts the source clips back, removes the rendered clip, and
/// drops the asset the render created. The FILE is left on disk, exactly as an
/// undone recording leaves its take: undo reverses a decision about the
/// project, not about the user's disk.
class ConsolidateClipsCommand final : public Command {
public:
    ConsolidateClipsCommand(ClipIds clips, std::filesystem::path output,
                            const project::ParameterRegistry* parameters = nullptr)
        : clips_(std::move(clips)), output_(std::move(output)), parameters_(parameters) {}

    [[nodiscard]] const char* id() const noexcept override { return "clip.consolidate"; }
    [[nodiscard]] std::string name() const override { return "Consolidate"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// The clip the render became, valid only after the first execute.
    [[nodiscard]] EntityId consolidatedClip() const noexcept { return created_.id; }

    /// Why the last execute refused, when it did.
    [[nodiscard]] const std::string& error() const noexcept { return error_; }

private:
    struct RemovedClip {
        std::size_t index = 0;
        Clip        clip;
    };

    ClipIds                           clips_;
    std::filesystem::path             output_;
    const project::ParameterRegistry* parameters_ = nullptr;

    std::string              error_;
    std::vector<RemovedClip> removed_;
    AudioAssetImport         asset_;
    Clip                     created_;
    std::size_t              createdIndex_ = 0;
    bool                     minted_       = false;
};

} // namespace incdaw::app
