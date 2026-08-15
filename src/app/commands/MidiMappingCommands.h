#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <string>

namespace incdaw::app {

using project::EntityId;

/// Binds a hardware control to a parameter — the tail end of MIDI learn.
///
/// Minted once and replayed on redo, id included, like every entity-creating
/// command.
class AddMidiMappingCommand final : public Command {
public:
    AddMidiMappingCommand(int midiChannel, int controller, std::string parameterKey,
                          EntityId target)
        : midiChannel_(midiChannel), controller_(controller),
          parameterKey_(std::move(parameterKey)), target_(target) {}

    [[nodiscard]] const char* id() const noexcept override { return "midi.addMapping"; }
    [[nodiscard]] std::string name() const override { return "Map MIDI Control"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId mappingId() const noexcept { return mapping_.id; }

private:
    int         midiChannel_ = -1;
    int         controller_  = 0;
    std::string parameterKey_;
    EntityId    target_;

    project::MidiMapping mapping_;
    bool                 minted_ = false;
};

class RemoveMidiMappingCommand final : public Command {
public:
    explicit RemoveMidiMappingCommand(EntityId mapping) : mappingId_(mapping) {}

    [[nodiscard]] const char* id() const noexcept override { return "midi.removeMapping"; }
    [[nodiscard]] std::string name() const override { return "Remove MIDI Mapping"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId             mappingId_;
    project::MidiMapping mapping_;
    std::size_t          index_ = 0;
};

} // namespace incdaw::app
