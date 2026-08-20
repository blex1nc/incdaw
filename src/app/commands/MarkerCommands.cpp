#include "app/commands/MarkerCommands.h"

#include <algorithm>

namespace incdaw::app {

// ── AddMarkerCommand ──────────────────────────────────────────────────────────

bool AddMarkerCommand::execute(Project& project)
{
    if (!minted_) {
        TimelineMarker& created = project.addMarker(std::max<Tick>(0, tick_), name_);
        created.length          = std::max<Tick>(0, length_);

        marker_ = created;
        index_  = project.markers().size() - 1;
        minted_ = true;
        return true;
    }

    project.insertMarker(index_, marker_);
    return true;
}

void AddMarkerCommand::undo(Project& project)
{
    (void)project.removeMarker(marker_.id);
}

// ── RemoveMarkerCommand ───────────────────────────────────────────────────────

bool RemoveMarkerCommand::execute(Project& project)
{
    const std::size_t index = project.indexOfMarker(markerId_);
    if (index == Project::notFound)
        return false;

    index_   = index;
    removed_ = project.markers()[index];
    return project.removeMarker(markerId_);
}

void RemoveMarkerCommand::undo(Project& project)
{
    project.insertMarker(index_, removed_);
}

// ── EditMarkerCommand ─────────────────────────────────────────────────────────

bool EditMarkerCommand::execute(Project& project)
{
    TimelineMarker* marker = project.findMarker(markerId_);
    if (marker == nullptr)
        return false;

    // The id is the marker's identity, not an editable field.
    updated_.id = marker->id;
    if (*marker == updated_)
        return false;

    previous_ = *marker;
    *marker   = updated_;
    return true;
}

void EditMarkerCommand::undo(Project& project)
{
    if (TimelineMarker* marker = project.findMarker(markerId_))
        *marker = previous_;
}

bool EditMarkerCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const EditMarkerCommand*>(&next);
    return other != nullptr && other->markerId_ == markerId_;
}

void EditMarkerCommand::mergeWith(const Command& next)
{
    // previous_ keeps the state from before the gesture; only the destination
    // advances.
    if (const auto* other = dynamic_cast<const EditMarkerCommand*>(&next))
        updated_ = other->updated_;
}

} // namespace incdaw::app
