#pragma once

#include "app/Command.h"
#include "engine/transport/TempoMap.h"

#include <vector>

namespace incdaw::app {

/// Tempo and time signature — the two numbers the control bar's display has
/// always shown and never let anyone change.
///
/// Both are ordinary project state: `TempoMap` has carried tempo AND signature
/// events since Phase 3, and `ProjectFile` has serialized both since format
/// 1.0. What was missing was a way to edit them that undo could reverse, which
/// is the only way anything in INCDAW is allowed to change the model
/// (CLAUDE.md §26).
///
/// Both commands edit the event at tick 0 — the project's starting tempo and
/// signature. A tempo MAP (changes partway through a song) is a timeline
/// editing feature and belongs to the ruler, not to a readout in the chrome;
/// these commands deliberately leave every later event alone.

/// Sets the tempo in effect from the start of the project.
///
/// Consecutive edits merge, so dragging the readout is one undo entry rather
/// than one per mouse move.
class SetTempoCommand final : public Command {
public:
    explicit SetTempoCommand(double beatsPerMinute) : tempo_(beatsPerMinute) {}

    /// The range the readout, the typed field and this command all agree on.
    /// Wide enough for a drum-and-bass half-time trick at one end and a
    /// ballad at the other; narrow enough that a slipped drag cannot produce
    /// a tempo the tempo map has to divide by.
    static constexpr double minimumTempo = 20.0;
    static constexpr double maximumTempo = 999.0;

    [[nodiscard]] static double clampTempo(double beatsPerMinute) noexcept;

    [[nodiscard]] const char* id() const noexcept override { return "transport.setTempo"; }
    [[nodiscard]] std::string name() const override { return "Set Tempo"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    double tempo_ = 120.0;

    std::vector<engine::TempoEvent> previous_;
    bool                            captured_ = false;
};

/// Sets the time signature in effect from the start of the project.
class SetTimeSignatureCommand final : public Command {
public:
    SetTimeSignatureCommand(int numerator, int denominator)
        : numerator_(numerator), denominator_(denominator) {}

    [[nodiscard]] const char* id() const noexcept override { return "transport.setTimeSignature"; }
    [[nodiscard]] std::string name() const override { return "Set Time Signature"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    int numerator_   = 4;
    int denominator_ = 4;

    std::vector<engine::TimeSignatureEvent> previous_;
    bool                                    captured_ = false;
};

} // namespace incdaw::app
