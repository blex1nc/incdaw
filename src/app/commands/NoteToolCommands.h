#pragma once

#include "app/Command.h"
#include "project/Identity.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::EntityId;
using project::MidiEvent;
using project::Tick;

/// Selection-shaping tools from the FL Studio piano-roll family: strum,
/// arpeggiate, legato and note labels (docs/FL2026_GAP.md P2). Each is a
/// command because each is a user gesture that must undo as one step.
using NoteIndices = std::vector<std::size_t>;

/// Staggers chord note starts like a strummed guitar chord.
///
/// Notes sharing a start tick form a chord; within each chord the starts
/// spread across `span`, lowest key first (or highest, when `downward`).
/// Ends stay anchored so the harmony still releases together.
class StrumNotesCommand final : public Command {
public:
    StrumNotesCommand(EntityId pattern, EntityId channel, NoteIndices indices,
                      Tick span, bool downward)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)),
          span_(span), downward_(downward) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.strum"; }
    [[nodiscard]] std::string name() const override { return "Strum"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    pattern_;
    EntityId    channel_;
    NoteIndices indices_;
    Tick        span_     = 0;
    bool        downward_ = false;

    std::vector<Tick> previousTicks_;       ///< aligned with indices_
    std::vector<Tick> previousDurations_;
};

/// Replaces each selected chord with a cycling sequence of its notes.
///
/// The chord's span is tiled with `step`-long notes walking the chord's keys
/// in the chosen direction. Rewrites the event list (note counts change), so
/// like quantize it captures the whole vector for undo.
class ArpeggiateNotesCommand final : public Command {
public:
    enum class Direction { up, down, upDown };

    ArpeggiateNotesCommand(EntityId pattern, EntityId channel, NoteIndices indices,
                           Tick step, Direction direction)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)),
          step_(step), direction_(direction) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.arpeggiate"; }
    [[nodiscard]] std::string name() const override { return "Arpeggiate"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    pattern_;
    EntityId    channel_;
    NoteIndices indices_;
    Tick        step_      = 0;
    Direction   direction_ = Direction::up;

    std::vector<MidiEvent> previousEvents_;
};

/// Extends each selected note to the next selected start.
///
/// Notes sharing a start extend together; the final group keeps its length,
/// because there is nothing to reach.
class LegatoNotesCommand final : public Command {
public:
    LegatoNotesCommand(EntityId pattern, EntityId channel, NoteIndices indices)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.legato"; }
    [[nodiscard]] std::string name() const override { return "Legato"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    pattern_;
    EntityId    channel_;
    NoteIndices indices_;

    std::vector<Tick> previousDurations_;   ///< aligned with indices_
};

/// Names the selected notes (FL Studio 2026's renamable notes).
class SetNoteLabelCommand final : public Command {
public:
    SetNoteLabelCommand(EntityId pattern, EntityId channel, NoteIndices indices,
                        std::string label)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)),
          label_(std::move(label)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.setNoteLabel"; }
    [[nodiscard]] std::string name() const override { return "Rename Notes"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    pattern_;
    EntityId    channel_;
    NoteIndices indices_;
    std::string label_;

    std::vector<std::string> previousLabels_;   ///< aligned with indices_
};

} // namespace incdaw::app
