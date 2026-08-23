#include "app/commands/MidiMappingCommands.h"

#include <algorithm>

namespace incdaw::app {

bool AddMidiMappingCommand::execute(Project& project)
{
    if (!minted_) {
        // One control drives one parameter: mapping the same (channel, CC)
        // again REPLACES the old binding rather than fanning out — replace
        // is what re-learning a knob means. The removal is part of this
        // command, so undo brings the old mapping back.
        for (const project::MidiMapping& existing : project.midiMappings())
            if (existing.controller == controller_ && existing.midiChannel == midiChannel_)
                return false;   // caller removes the old one first, undoably

        mapping_              = project::MidiMapping{};
        mapping_.id           = project.ids().next();
        mapping_.midiChannel  = midiChannel_;
        mapping_.controller   = controller_;
        mapping_.parameterKey = parameterKey_;
        mapping_.targetEntity = target_;
        minted_               = true;

        project.midiMappings().push_back(mapping_);
        return true;
    }

    project.insertMidiMapping(project.midiMappings().size(), mapping_);
    return true;
}

void AddMidiMappingCommand::undo(Project& project)
{
    project.removeMidiMapping(mapping_.id);
}

bool RemoveMidiMappingCommand::execute(Project& project)
{
    const std::size_t index = project.indexOfMidiMapping(mappingId_);
    if (index == Project::notFound)
        return false;

    index_   = index;
    mapping_ = project.midiMappings()[index];
    project.removeMidiMapping(mappingId_);
    return true;
}

void RemoveMidiMappingCommand::undo(Project& project)
{
    project.insertMidiMapping(index_, mapping_);
}

// ── AddPerformancePadMappingCommand ───────────────────────────────────────────

bool AddPerformancePadMappingCommand::execute(Project& project)
{
    if (pad_ < 0)
        return false;

    replaced_.clear();
    replacedIndices_.clear();

    // Whatever already answers this note stops answering it. A controller pad
    // that pressed two INCDAW pads at once would be unplayable, and nothing in
    // the list would say why.
    for (bool again = true; again;) {
        again = false;

        for (std::size_t index = 0; index < project.midiMappings().size(); ++index) {
            const project::MidiMapping& existing = project.midiMappings()[index];

            if (existing.kind != project::MidiMappingKind::performancePad
                || existing.controller != note_ || existing.midiChannel != midiChannel_)
                continue;

            replaced_.push_back(existing);
            replacedIndices_.push_back(index);

            project.midiMappings().erase(project.midiMappings().begin()
                                         + static_cast<std::ptrdiff_t>(index));
            again = true;
            break;
        }
    }

    if (!minted_) {
        mapping_.id             = project.ids().next();
        mapping_.kind           = project::MidiMappingKind::performancePad;
        mapping_.midiChannel    = midiChannel_;
        mapping_.controller     = note_;
        mapping_.performancePad = pad_;
        minted_                 = true;
    } else {
        project.ids().observe(mapping_.id);
    }

    project.midiMappings().push_back(mapping_);
    return true;
}

void AddPerformancePadMappingCommand::undo(Project& project)
{
    for (std::size_t index = 0; index < project.midiMappings().size(); ++index) {
        if (project.midiMappings()[index].id != mapping_.id)
            continue;

        project.midiMappings().erase(project.midiMappings().begin()
                                     + static_cast<std::ptrdiff_t>(index));
        break;
    }

    // Back in the order they came out, which is the order they were in.
    for (std::size_t entry = replaced_.size(); entry > 0; --entry) {
        const std::size_t at = std::min(replacedIndices_[entry - 1],
                                        project.midiMappings().size());

        project.midiMappings().insert(project.midiMappings().begin()
                                          + static_cast<std::ptrdiff_t>(at),
                                      replaced_[entry - 1]);
    }
}

} // namespace incdaw::app
