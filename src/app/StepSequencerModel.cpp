#include "app/StepSequencerModel.h"

namespace incdaw::app {
namespace {

[[nodiscard]] Tick divisionOf(const project::Pattern& pattern) noexcept
{
    return pattern.stepDivision > 0 ? pattern.stepDivision : 1;
}

} // namespace

bool StepSequencerModel::ownedBy(const project::MidiEvent& event, EntityId channel) const noexcept
{
    if (event.type != project::MidiEventType::note)
        return false;

    const EntityId owner = event.channelId.isValid() ? event.channelId
                                                     : project_->defaultChannel();
    return owner == channel;
}

int StepSequencerModel::stepCount() const noexcept
{
    if (pattern_ == nullptr)
        return 0;

    return static_cast<int>(pattern_->length / divisionOf(*pattern_));
}

int StepSequencerModel::stepCount(EntityId channel) const noexcept
{
    if (pattern_ == nullptr)
        return 0;

    return static_cast<int>(pattern_->lengthFor(channel) / divisionOf(*pattern_));
}

Tick StepSequencerModel::tickForStep(int step) const noexcept
{
    if (pattern_ == nullptr || step < 0)
        return 0;

    return static_cast<Tick>(step) * divisionOf(*pattern_);
}

int StepSequencerModel::stepForTick(Tick tick) const noexcept
{
    if (pattern_ == nullptr || tick < 0)
        return 0;

    return static_cast<int>(tick / divisionOf(*pattern_));
}

const project::MidiEvent* StepSequencerModel::noteAt(EntityId channel, int step) const noexcept
{
    if (pattern_ == nullptr || step < 0)
        return nullptr;

    const Tick tick = tickForStep(step);

    for (const project::MidiEvent& event : pattern_->events)
        if (event.tick == tick && ownedBy(event, channel))
            return &event;

    return nullptr;
}

int StepSequencerModel::velocityAt(EntityId channel, int step) const noexcept
{
    const project::MidiEvent* note = noteAt(channel, step);
    return note != nullptr ? note->value : 0;
}

double StepSequencerModel::probabilityAt(EntityId channel, int step) const noexcept
{
    const project::MidiEvent* note = noteAt(channel, step);
    return note != nullptr ? note->probability : 0.0;
}

std::size_t StepSequencerModel::offGridNoteCount(EntityId channel) const noexcept
{
    if (pattern_ == nullptr)
        return 0;

    const Tick division = divisionOf(*pattern_);
    std::size_t count = 0;

    for (const project::MidiEvent& event : pattern_->events)
        if (ownedBy(event, channel) && (event.tick % division) != 0)
            ++count;

    return count;
}

} // namespace incdaw::app
