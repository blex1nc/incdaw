#pragma once

#include "project/Model.h"

#include <cstddef>

namespace incdaw::app {

using project::EntityId;
using project::Project;
using project::Tick;

/// A read-only view of one pattern as a step grid.
///
/// The step sequencer and the piano roll are two views of the same notes, not
/// two data models: a step is a note at the step's tick on that channel. This
/// class only *interprets* the pattern; every change goes through
/// ToggleStepCommand like everything else.
///
/// Holds references, not copies. It is built per query, costs nothing, and can
/// never go stale — a cached grid would need invalidating on every edit, and
/// the edit that forgets is the one that leaves a step drawn after it was
/// deleted.
class StepSequencerModel {
public:
    StepSequencerModel(const Project& project, EntityId pattern) noexcept
        : project_(&project), pattern_(project.findPattern(pattern)) {}

    [[nodiscard]] bool isValid() const noexcept { return pattern_ != nullptr; }

    [[nodiscard]] const project::Pattern* pattern() const noexcept { return pattern_; }

    /// Steps in the grid. A channel with a shorter length of its own has fewer
    /// — that is the visible face of a polymetric pattern.
    [[nodiscard]] int stepCount() const noexcept;
    [[nodiscard]] int stepCount(EntityId channel) const noexcept;

    [[nodiscard]] Tick tickForStep(int step) const noexcept;
    [[nodiscard]] int  stepForTick(Tick tick) const noexcept;

    /// The note occupying a step, or nullptr. Notes off the step grid — moved
    /// in the piano roll, or recorded — do not appear as steps, which is the
    /// honest answer: the grid cannot represent them.
    [[nodiscard]] const project::MidiEvent* noteAt(EntityId channel, int step) const noexcept;

    [[nodiscard]] bool isStepOn(EntityId channel, int step) const noexcept
    {
        return noteAt(channel, step) != nullptr;
    }

    [[nodiscard]] int    velocityAt(EntityId channel, int step) const noexcept;
    [[nodiscard]] double probabilityAt(EntityId channel, int step) const noexcept;

    /// Notes this channel owns that the grid cannot show, because they do not
    /// land on a step boundary. The UI needs this to say so rather than
    /// silently hiding them.
    [[nodiscard]] std::size_t offGridNoteCount(EntityId channel) const noexcept;

private:
    [[nodiscard]] bool ownedBy(const project::MidiEvent& event, EntityId channel) const noexcept;

    const Project*          project_ = nullptr;
    const project::Pattern* pattern_ = nullptr;
};

} // namespace incdaw::app
