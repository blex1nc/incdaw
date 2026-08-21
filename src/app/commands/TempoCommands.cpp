#include "app/commands/TempoCommands.h"

#include <algorithm>
#include <cmath>

namespace incdaw::app {

using engine::TempoEvent;
using engine::TimeSignatureEvent;

double SetProjectTempoCommand::clamped(double beatsPerMinute) noexcept
{
    if (!std::isfinite(beatsPerMinute))
        return 120.0;

    return std::clamp(beatsPerMinute, minimumTempo, maximumTempo);
}

bool SetProjectTempoCommand::execute(Project& project)
{
    engine::TempoMap& map = project.tempoMap();

    if (!captured_) {
        previous_ = map.tempoEvents();
        captured_ = true;
    }

    const double target = clamped(tempo_);

    std::vector<TempoEvent> events = previous_;
    if (events.empty())
        events.push_back(TempoEvent{});

    // The map always carries an event at tick 0, synthesised if the project did
    // not write one, so the base tempo is the first entry by construction.
    if (std::abs(events.front().beatsPerMinute - target) < 1e-9)
        return false;

    events.front().tick           = 0;
    events.front().beatsPerMinute = target;

    map.setTempoEvents(std::move(events));
    return true;
}

void SetProjectTempoCommand::undo(Project& project)
{
    if (captured_)
        project.tempoMap().setTempoEvents(previous_);
}

bool SetProjectTempoCommand::canMergeWith(const Command& next) const noexcept
{
    return dynamic_cast<const SetProjectTempoCommand*>(&next) != nullptr;
}

void SetProjectTempoCommand::mergeWith(const Command& next)
{
    // Keeps the tempo the field started at and adopts where it is now: undoing
    // a drag returns to before the drag, not to its second-to-last value.
    if (const auto* other = dynamic_cast<const SetProjectTempoCommand*>(&next))
        tempo_ = other->tempo_;
}

// ── Time signature ───────────────────────────────────────────────────────────

bool SetTimeSignatureCommand::isValid(int numerator, int denominator) noexcept
{
    return engine::TimeSignature{numerator, denominator}.isValid()
        && numerator <= maximumNumerator;
}

bool SetTimeSignatureCommand::execute(Project& project)
{
    if (!isValid(numerator_, denominator_))
        return false;

    engine::TempoMap& map = project.tempoMap();

    if (!captured_) {
        previous_ = map.timeSignatureEvents();
        captured_ = true;
    }

    std::vector<TimeSignatureEvent> events = previous_;
    if (events.empty())
        events.push_back(TimeSignatureEvent{});

    if (events.front().signature.numerator == numerator_
        && events.front().signature.denominator == denominator_)
        return false;

    events.front().tick      = 0;
    events.front().signature = engine::TimeSignature{numerator_, denominator_};

    map.setTimeSignatureEvents(std::move(events));
    return true;
}

void SetTimeSignatureCommand::undo(Project& project)
{
    if (captured_)
        project.tempoMap().setTimeSignatureEvents(previous_);
}

} // namespace incdaw::app
