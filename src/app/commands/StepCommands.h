#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>

namespace incdaw::app {

using project::EntityId;
using project::MidiEvent;
using project::Tick;

/// Turns one step sequencer cell on or off.
///
/// A step IS a note. There is deliberately no step data type: the step grid and
/// the Piano Roll edit the same `MidiEvent`s in the same pattern, so the two
/// editors cannot drift apart, and a step programmed in the rack can be given a
/// different length, velocity or probability in the Piano Roll without becoming
/// a different kind of object. FL Studio's rack and Piano Roll behave this way
/// for the same reason.
///
/// The cell is a half-open tick range, not an exact position: a note nudged
/// off the grid in the Piano Roll still belongs to the step it sits inside, so
/// the rack keeps showing it rather than appearing to have lost it.
class ToggleStepCommand final : public Command {
public:
    struct Step {
        EntityId pattern;
        EntityId channel;
        Tick     start    = 0;
        Tick     length   = 0;
        int      key      = 60;
        int      velocity = 100;
    };

    explicit ToggleStepCommand(Step step) : step_(step) {}

    [[nodiscard]] const char* id() const noexcept override { return "step.toggle"; }
    [[nodiscard]] std::string name() const override { return added_ ? "Add Step" : "Clear Step"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// True when the toggle turned the step on.
    [[nodiscard]] bool added() const noexcept { return added_; }

private:
    Step        step_;
    bool        added_ = false;

    /// True when this command was the first to program the channel in this
    /// pattern. Undo then removes the content block it created as well as the
    /// note, or the project would not be byte-for-byte what it was.
    bool        createdContent_ = false;

    std::size_t index_ = 0;    ///< where the note was added, or removed from
    MidiEvent   note_;          ///< the note removed, for undo
};

/// Index of the note occupying a step, or `noStep`.
///
/// Shared by the command and the rack's hit testing so that "is this step on?"
/// has exactly one answer in the codebase.
inline constexpr std::size_t noStep = static_cast<std::size_t>(-1);

[[nodiscard]] std::size_t noteAtStep(const std::vector<MidiEvent>& events,
                                     Tick start, Tick length, int key) noexcept;

} // namespace incdaw::app
