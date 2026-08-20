#include "app/commands/TempoCommands.h"

#include <algorithm>
#include <cmath>

namespace incdaw::app {

double SetTempoCommand::clampTempo(double beatsPerMinute) noexcept
{
    if (!std::isfinite(beatsPerMinute))
        return 120.0;

    return std::clamp(beatsPerMinute, minimumTempo, maximumTempo);
}

bool SetTempoCommand::execute(Project& project)
{
    const double tempo = clampTempo(tempo_);

    // Captured on the FIRST execute only: a redo must restore what the project
    // looked like before the command ever ran, not before its last replay.
    if (!captured_) {
        previous_ = project.tempoMap().tempoEvents();
        captured_ = true;
    }

    if (std::abs(project.tempoMap().tempoAtTick(0) - tempo) < 1.0e-9)
        return false;   // nothing to undo

    std::vector<engine::TempoEvent> events = previous_;

    // The map synthesises an event at tick 0 when none is present, so there is
    // always one to rewrite; later changes are left exactly where they are.
    if (events.empty())
        events.push_back({0, tempo});
    else
        events.front().beatsPerMinute = tempo;

    project.tempoMap().setTempoEvents(std::move(events));
    tempo_ = tempo;
    return true;
}

void SetTempoCommand::undo(Project& project)
{
    if (captured_)
        project.tempoMap().setTempoEvents(previous_);
}

bool SetTempoCommand::canMergeWith(const Command& next) const noexcept
{
    return dynamic_cast<const SetTempoCommand*>(&next) != nullptr;
}

void SetTempoCommand::mergeWith(const Command& next)
{
    // The merged entry keeps the tempo the drag started from and adopts where
    // it is now: one undo returns to before the gesture.
    if (const auto* other = dynamic_cast<const SetTempoCommand*>(&next))
        tempo_ = other->tempo_;
}

bool SetTimeSignatureCommand::execute(Project& project)
{
    const engine::TimeSignature signature{numerator_, denominator_};
    if (!signature.isValid())
        return false;

    if (!captured_) {
        previous_ = project.tempoMap().timeSignatureEvents();
        captured_ = true;
    }

    const engine::TimeSignature current = project.tempoMap().timeSignatureAtTick(0);
    if (current.numerator == numerator_ && current.denominator == denominator_)
        return false;

    std::vector<engine::TimeSignatureEvent> events = previous_;

    if (events.empty())
        events.push_back({0, signature});
    else
        events.front().signature = signature;

    project.tempoMap().setTimeSignatureEvents(std::move(events));
    return true;
}

void SetTimeSignatureCommand::undo(Project& project)
{
    if (captured_)
        project.tempoMap().setTimeSignatureEvents(previous_);
}

} // namespace incdaw::app
