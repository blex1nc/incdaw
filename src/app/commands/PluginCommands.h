#pragma once

#include "app/Command.h"
#include "plugins/PluginIdentifier.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>

namespace incdaw::app {

using project::EntityId;
using project::PluginSlot;

/// Insert-chain edits.
///
/// A slot is project data; the graph compiles whatever the chain says on the
/// next rebuild, and the slot's stateFile rides along untouched — which is
/// what makes remove-then-undo keep the plugin's saved state (D-030).

class AddInsertCommand final : public Command {
public:
    /// Appends to the chain.
    AddInsertCommand(EntityId mixerNode, plugins::PluginIdentifier plugin)
        : mixerNode_(mixerNode), plugin_(std::move(plugin)) {}

    /// Inserts AT `index`, which is what dropping a plugin onto a particular
    /// slot means. An index past the end appends, so a drop below the chain
    /// does the obvious thing rather than failing. Redo replays the same
    /// index: an insert that landed third must land third again, or the undo
    /// stack above it would be describing a different chain.
    AddInsertCommand(EntityId mixerNode, plugins::PluginIdentifier plugin, std::size_t index)
        : mixerNode_(mixerNode), plugin_(std::move(plugin)), index_(index) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.insert.add"; }
    [[nodiscard]] std::string name() const override { return "Add Insert"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute. Stable across redo: the slot id is
    /// minted once, so automation lanes written against it stay valid.
    [[nodiscard]] EntityId slotId() const noexcept { return slot_.id; }

private:
    static constexpr std::size_t append = static_cast<std::size_t>(-1);

    EntityId                  mixerNode_;
    plugins::PluginIdentifier plugin_;
    std::size_t               index_ = append;
    PluginSlot                slot_;
    bool                      minted_ = false;
};

class RemoveInsertCommand final : public Command {
public:
    RemoveInsertCommand(EntityId mixerNode, EntityId slot)
        : mixerNode_(mixerNode), slotId_(slot) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.insert.remove"; }
    [[nodiscard]] std::string name() const override { return "Remove Insert"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    mixerNode_;
    EntityId    slotId_;
    PluginSlot  removed_;   ///< the whole slot, stateFile included
    std::size_t index_ = 0;
};

/// Moves one insert within its chain — chain order is signal order, and
/// until this existed the order could only be built by add order. Addressed
/// by slot id, direction as an offset (-1 up toward the signal's entry, +1
/// down toward the fader).
class MoveInsertCommand final : public Command {
public:
    MoveInsertCommand(EntityId mixerNode, EntityId slot, int direction)
        : mixerNode_(mixerNode), slotId_(slot), direction_(direction) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.insert.move"; }
    [[nodiscard]] std::string name() const override { return "Move Insert"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    mixerNode_;
    EntityId    slotId_;
    int         direction_ = 0;
    std::size_t movedFrom_ = 0;
};

class SetInsertBypassedCommand final : public Command {
public:
    SetInsertBypassedCommand(EntityId mixerNode, EntityId slot, bool bypassed)
        : mixerNode_(mixerNode), slotId_(slot), bypassed_(bypassed) {}

    [[nodiscard]] const char* id() const noexcept override { return "mixer.insert.bypass"; }
    [[nodiscard]] std::string name() const override
    {
        return bypassed_ ? "Bypass Insert" : "Enable Insert";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId mixerNode_;
    EntityId slotId_;
    bool     bypassed_;
    bool     previous_ = false;
};

} // namespace incdaw::app
