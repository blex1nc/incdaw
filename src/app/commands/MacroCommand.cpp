#include "app/commands/MacroCommand.h"

namespace incdaw::app {

void MacroCommand::add(CommandPtr command)
{
    if (command == nullptr)
        return;

    // Queued in the same list as the lazy steps, so that ordering is the same
    // whichever way a child was supplied: a ready-made child still runs in its
    // turn, and the step after it still sees what it did.
    pending_.push_back(Pending{std::move(command), {}});
}

void MacroCommand::addStep(Step step)
{
    if (step)
        pending_.push_back(Pending{nullptr, std::move(step)});
}

bool MacroCommand::execute(Project& project)
{
    if (built_) {
        for (const CommandPtr& child : children_)
            (void)child->execute(project);

        return !children_.empty();
    }

    built_ = true;

    for (Pending& entry : pending_) {
        CommandPtr child = entry.ready != nullptr ? std::move(entry.ready)
                                                  : (entry.build ? entry.build(project) : nullptr);
        if (child == nullptr)
            continue;

        // A child that changed nothing is dropped, not kept: replaying it on
        // redo would be a no-op there too, and keeping it only makes the macro
        // look like it did more than it did.
        if (child->execute(project))
            children_.push_back(std::move(child));
    }

    pending_.clear();
    return !children_.empty();
}

void MacroCommand::undo(Project& project)
{
    for (auto child = children_.rbegin(); child != children_.rend(); ++child)
        (*child)->undo(project);
}

} // namespace incdaw::app
