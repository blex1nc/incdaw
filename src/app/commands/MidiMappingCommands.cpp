#include "app/commands/MidiMappingCommands.h"

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

} // namespace incdaw::app
