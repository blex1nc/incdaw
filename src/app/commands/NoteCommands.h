#pragma once

#include "app/Command.h"
#include "project/Identity.h"

#include <cstddef>
#include <vector>

namespace incdaw::app {

using project::EntityId;
using project::MidiEvent;
using project::Tick;

/// Note edits address one channel's event list inside one pattern, by index.
///
/// The (pattern, channel) pair rather than the pattern alone: a pattern holds
/// content for every channel programmed in it, and an edit always belongs to
/// exactly one of them.
///
/// Indices are safe here specifically because the undo stack is LIFO: when a
/// command is undone, the project is in exactly the state that command left it
/// in, so the indices it captured still mean what they meant. Commands
/// therefore must not reorder events — the only one that does (quantize)
/// captures the whole vector instead.
using NoteIndices = std::vector<std::size_t>;

/// Adds one note.
class AddNoteCommand final : public Command {
public:
    AddNoteCommand(EntityId pattern, EntityId channel, MidiEvent note)
        : pattern_(pattern), channel_(channel), note_(std::move(note)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.addNote"; }
    [[nodiscard]] std::string name() const override { return "Add Note"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] std::size_t insertedIndex() const noexcept { return index_; }

private:
    EntityId    pattern_;
    EntityId    channel_;
    MidiEvent   note_;
    std::size_t index_ = 0;
};

/// Removes notes.
class DeleteNotesCommand final : public Command {
public:
    DeleteNotesCommand(EntityId pattern, EntityId channel, NoteIndices indices)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.deleteNotes"; }
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId               pattern_;
    EntityId               channel_;
    NoteIndices            indices_;
    std::vector<MidiEvent> removed_;   ///< captured in the same order as indices_
};

/// Moves notes in time and pitch. Mergeable, so a drag is one undo.
class MoveNotesCommand final : public Command {
public:
    MoveNotesCommand(EntityId pattern, EntityId channel, NoteIndices indices,
                     Tick tickDelta, int keyDelta)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)),
          tickDelta_(tickDelta), keyDelta_(keyDelta) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.moveNotes"; }
    [[nodiscard]] std::string name() const override { return "Move Notes"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId    pattern_;
    EntityId    channel_;
    NoteIndices indices_;
    Tick        tickDelta_ = 0;
    int         keyDelta_  = 0;

    /// What was actually applied, which can be less than requested when notes
    /// hit the start of the pattern or the ends of the keyboard. Undo must
    /// reverse what happened, not what was asked for.
    Tick        appliedTickDelta_ = 0;
    int         appliedKeyDelta_  = 0;
};

/// Changes note lengths. Mergeable.
class ResizeNotesCommand final : public Command {
public:
    ResizeNotesCommand(EntityId pattern, EntityId channel, NoteIndices indices, Tick durationDelta)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)),
          durationDelta_(durationDelta) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.resizeNotes"; }
    [[nodiscard]] std::string name() const override { return "Resize Notes"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId          pattern_;
    EntityId          channel_;
    NoteIndices       indices_;
    Tick              durationDelta_ = 0;
    std::vector<Tick> previousDurations_;
};

/// Sets velocity on a selection.
class SetVelocityCommand final : public Command {
public:
    SetVelocityCommand(EntityId pattern, EntityId channel, NoteIndices indices, int velocity)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)), velocity_(velocity) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.setVelocity"; }
    [[nodiscard]] std::string name() const override { return "Set Velocity"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId         pattern_;
    EntityId         channel_;
    NoteIndices      indices_;
    int              velocity_ = 100;
    std::vector<int> previousVelocities_;
};

/// Snaps note starts to a grid.
///
/// Captures the whole event vector rather than per-note deltas, because
/// quantizing re-sorts and therefore invalidates indices — the one case the
/// index scheme above does not cover.
class QuantizeNotesCommand final : public Command {
public:
    QuantizeNotesCommand(EntityId pattern, EntityId channel, Tick grid, double strength)
        : pattern_(pattern), channel_(channel), grid_(grid), strength_(strength) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.quantize"; }
    [[nodiscard]] std::string name() const override { return "Quantize"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId               pattern_;
    EntityId               channel_;
    Tick                   grid_     = 0;
    double                 strength_ = 1.0;
    std::vector<MidiEvent> previousEvents_;
};

} // namespace incdaw::app
