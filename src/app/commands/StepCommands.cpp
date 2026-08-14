#include "app/commands/StepCommands.h"

#include <algorithm>

namespace incdaw::app {

std::size_t noteAtStep(const std::vector<MidiEvent>& events, Tick start, Tick length, int key) noexcept
{
    if (length <= 0)
        return noStep;

    for (std::size_t index = 0; index < events.size(); ++index) {
        const MidiEvent& event = events[index];

        if (event.type != project::MidiEventType::note || event.key != key)
            continue;

        if (event.tick >= start && event.tick < start + length)
            return index;
    }

    return noStep;
}

bool ToggleStepCommand::execute(Project& project)
{
    project::Pattern* pattern = project.findPattern(step_.pattern);
    if (pattern == nullptr || step_.length <= 0)
        return false;

    createdContent_ = pattern->content(step_.channel) == nullptr;

    std::vector<MidiEvent>& events = pattern->contentFor(step_.channel).events;

    const std::size_t existing = noteAtStep(events, step_.start, step_.length, step_.key);

    if (existing != noStep) {
        added_ = false;
        index_ = existing;
        note_  = events[existing];
        events.erase(events.begin() + static_cast<std::ptrdiff_t>(existing));
        return true;
    }

    MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.tick     = step_.start;
    note.duration = step_.length;
    note.key      = std::clamp(step_.key, 0, 127);
    note.value    = std::clamp(step_.velocity, 1, 127);

    added_ = true;
    index_ = events.size();
    note_  = note;
    events.push_back(note);
    return true;
}

void ToggleStepCommand::undo(Project& project)
{
    project::Pattern* pattern = project.findPattern(step_.pattern);
    if (pattern == nullptr)
        return;

    std::vector<MidiEvent>* events = pattern->events(step_.channel);
    if (events == nullptr)
        return;

    if (added_) {
        if (index_ < events->size())
            events->erase(events->begin() + static_cast<std::ptrdiff_t>(index_));

        if (createdContent_ && events->empty()) {
            for (std::size_t index = 0; index < pattern->channels.size(); ++index) {
                if (pattern->channels[index].channel != step_.channel)
                    continue;

                pattern->channels.erase(pattern->channels.begin()
                                        + static_cast<std::ptrdiff_t>(index));
                break;
            }
        }

        return;
    }

    events->insert(events->begin() + static_cast<std::ptrdiff_t>(std::min(index_, events->size())),
                   note_);
}

} // namespace incdaw::app
