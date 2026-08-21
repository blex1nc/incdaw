#pragma once

#include "app/Command.h"
#include "engine/transport/TempoMap.h"

#include <vector>

namespace incdaw::app {

/// Sets the project's base tempo — the event at tick 0.
///
/// Later tempo changes are left alone: a project with a ritardando written into
/// bar 40 must not lose it because someone corrected the starting tempo. Undo
/// restores the whole event list, not just the first entry, because a map with
/// no event at tick 0 is synthesised into one and that synthesis is not
/// reversible field by field.
class SetProjectTempoCommand final : public Command {
public:
    explicit SetProjectTempoCommand(double beatsPerMinute) : tempo_(beatsPerMinute) {}

    /// The range the transport and the UI agree on. A tempo of zero is a
    /// timeline that never advances, and one of 10,000 is a rounding error in
    /// every frame conversion downstream.
    static constexpr double minimumTempo = 20.0;
    static constexpr double maximumTempo = 400.0;

    [[nodiscard]] static double clamped(double beatsPerMinute) noexcept;

    [[nodiscard]] const char* id() const noexcept override { return "project.setTempo"; }
    [[nodiscard]] std::string name() const override { return "Set Tempo"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Dragging a tempo field is one gesture, not forty undo entries.
    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    double                          tempo_;
    std::vector<engine::TempoEvent> previous_;
    bool                            captured_ = false;
};

/// Sets the project's base time signature — the event at tick 0.
class SetTimeSignatureCommand final : public Command {
public:
    SetTimeSignatureCommand(int numerator, int denominator)
        : numerator_(numerator), denominator_(denominator) {}

    /// Denominators are powers of two by definition; numerators beyond this are
    /// a typo rather than a metre.
    static constexpr int maximumNumerator = 32;

    [[nodiscard]] static bool isValid(int numerator, int denominator) noexcept;

    [[nodiscard]] const char* id() const noexcept override { return "project.setTimeSignature"; }
    [[nodiscard]] std::string name() const override { return "Set Time Signature"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    int numerator_;
    int denominator_;

    std::vector<engine::TimeSignatureEvent> previous_;
    bool                                    captured_ = false;
};

} // namespace incdaw::app
