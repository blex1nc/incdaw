#include "app/commands/ArrangementCommands.h"

#include <algorithm>
#include <utility>

namespace incdaw::app {

// ── AddArrangementCommand ─────────────────────────────────────────────────────

bool AddArrangementCommand::execute(Project& project)
{
    previousCurrent_ = project.currentArrangement();

    if (!minted_) {
        const Arrangement* source = copyFrom_.isValid() ? project.findArrangement(copyFrom_)
                                                        : nullptr;
        if (copyFrom_.isValid() && source == nullptr)
            return false;

        Arrangement made;
        made.id   = project.ids().next();
        made.name = name_;

        if (source != nullptr) {
            // Fresh ids throughout: a duplicated arrangement is a separate
            // layout, and two clips sharing an id would make findClip a
            // coin toss the moment both timelines existed.
            made.clips = source->clips;
            for (project::Clip& clip : made.clips)
                clip.id = project.ids().next();

            made.markers = source->markers;
            for (project::TimelineMarker& marker : made.markers)
                marker.id = project.ids().next();
        }

        arrangement_ = std::move(made);
        index_       = project.arrangements().size();
        minted_      = true;
    }

    (void)project.insertArrangement(index_, arrangement_);

    // A new arrangement is made in order to be worked in.
    (void)project.setCurrentArrangement(arrangement_.id);
    return true;
}

void AddArrangementCommand::undo(Project& project)
{
    (void)project.setCurrentArrangement(previousCurrent_);
    (void)project.removeArrangement(arrangement_.id);
}

// ── RemoveArrangementCommand ──────────────────────────────────────────────────

bool RemoveArrangementCommand::execute(Project& project)
{
    if (project.arrangements().size() < 2)
        return false;   // a project with no timeline has nowhere to put a clip

    index_ = project.indexOfArrangement(arrangementId_);
    if (index_ == Project::notFound)
        return false;

    arrangement_     = project.arrangements()[index_];
    previousCurrent_ = project.currentArrangement();

    return project.removeArrangement(arrangementId_);
}

void RemoveArrangementCommand::undo(Project& project)
{
    (void)project.insertArrangement(index_, arrangement_);
    (void)project.setCurrentArrangement(previousCurrent_);
}

// ── RenameArrangementCommand ──────────────────────────────────────────────────

bool RenameArrangementCommand::execute(Project& project)
{
    Arrangement* arrangement = project.findArrangement(arrangementId_);
    if (arrangement == nullptr || arrangement->name == name_)
        return false;

    previousName_     = arrangement->name;
    arrangement->name = name_;
    return true;
}

void RenameArrangementCommand::undo(Project& project)
{
    if (Arrangement* arrangement = project.findArrangement(arrangementId_))
        arrangement->name = previousName_;
}

// ── SetCurrentArrangementCommand ──────────────────────────────────────────────

bool SetCurrentArrangementCommand::execute(Project& project)
{
    if (project.currentArrangement() == arrangementId_)
        return false;

    previous_ = project.currentArrangement();
    return project.setCurrentArrangement(arrangementId_);
}

void SetCurrentArrangementCommand::undo(Project& project)
{
    (void)project.setCurrentArrangement(previous_);
}

} // namespace incdaw::app
