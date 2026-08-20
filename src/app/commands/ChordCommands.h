#pragma once

#include "app/Command.h"
#include "app/MusicTheory.h"
#include "project/Identity.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::EntityId;
using project::MidiEvent;
using project::Tick;

/// Inserts a batch of notes as one undo step.
///
/// The chord tools (stamp, progression insert) and the note tools built on
/// them (arpeggiate results, strummed chords) all reduce to "these notes
/// appear together and disappear together". One generic command instead of
/// one per tool (CLAUDE.md §34); the display name carries the tool's identity
/// into the undo menu.
class InsertNotesCommand final : public Command {
public:
    InsertNotesCommand(EntityId pattern, EntityId channel, std::vector<MidiEvent> notes,
                       std::string displayName)
        : pattern_(pattern), channel_(channel), notes_(std::move(notes)),
          displayName_(std::move(displayName)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.insertNotes"; }
    [[nodiscard]] std::string name() const override { return displayName_; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Indices the notes landed at, for selection after the stamp.
    [[nodiscard]] std::size_t firstInsertedIndex() const noexcept { return firstIndex_; }
    [[nodiscard]] std::size_t insertedCount() const noexcept { return notes_.size(); }

private:
    EntityId               pattern_;
    EntityId               channel_;
    std::vector<MidiEvent> notes_;
    std::string            displayName_;
    std::size_t            firstIndex_ = 0;
};

/// Replaces a selected chord with a neighbouring diatonic chord.
///
/// FL Studio 2026's Chord Progression tool lets a stamped chord be nudged
/// through the progression by dragging; this is that gesture as a command.
/// The selection's keys are rewritten voice-led from where they are, so a
/// nudge sounds like the same hands moving. Mergeable: dragging through
/// several chords is one undo back to the original.
class NudgeChordCommand final : public Command {
public:
    NudgeChordCommand(EntityId pattern, EntityId channel, std::vector<std::size_t> indices,
                      int keyRootPitchClass, music::Scale scale, int steps)
        : pattern_(pattern), channel_(channel), indices_(std::move(indices)),
          keyRoot_(keyRootPitchClass), scale_(scale), steps_(steps) {}

    [[nodiscard]] const char* id() const noexcept override { return "pianoroll.nudgeChord"; }
    [[nodiscard]] std::string name() const override { return "Nudge Chord"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId                 pattern_;
    EntityId                 channel_;
    std::vector<std::size_t> indices_;
    int                      keyRoot_ = 0;
    music::Scale             scale_   = music::Scale::major;
    int                      steps_   = 0;
    std::vector<int>         previousKeys_;   ///< aligned with indices_, for undo
};

} // namespace incdaw::app
