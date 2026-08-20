#include "app/commands/NoteToolCommands.h"

#include <algorithm>
#include <map>

namespace incdaw::app {
namespace {

std::vector<MidiEvent>* findEvents(Project& project, EntityId pattern, EntityId channel) noexcept
{
    project::Pattern* found = project.findPattern(pattern);
    return found != nullptr ? found->events(channel) : nullptr;
}

/// In-range, deduplicated, note-type-only selection, sorted ascending.
NoteIndices noteSelection(const NoteIndices& indices, const std::vector<MidiEvent>& events)
{
    NoteIndices selection;
    for (const std::size_t index : indices)
        if (index < events.size() && events[index].type == project::MidiEventType::note)
            selection.push_back(index);

    std::sort(selection.begin(), selection.end());
    selection.erase(std::unique(selection.begin(), selection.end()), selection.end());
    return selection;
}

/// Selected indices grouped by identical start tick — the piano-roll notion
/// of "a chord". Ordered by tick.
std::map<Tick, NoteIndices> chordGroups(const NoteIndices& selection,
                                        const std::vector<MidiEvent>& events)
{
    std::map<Tick, NoteIndices> groups;
    for (const std::size_t index : selection)
        groups[events[index].tick].push_back(index);
    return groups;
}

constexpr Tick minimumDuration = 1;

} // namespace

// ── StrumNotesCommand ─────────────────────────────────────────────────────────

bool StrumNotesCommand::execute(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr || span_ <= 0)
        return false;

    indices_ = noteSelection(indices_, *events);
    if (indices_.empty())
        return false;

    previousTicks_.clear();
    previousDurations_.clear();
    for (const std::size_t index : indices_) {
        previousTicks_.push_back((*events)[index].tick);
        previousDurations_.push_back((*events)[index].duration);
    }

    bool changed = false;
    for (auto& [tick, group] : chordGroups(indices_, *events)) {
        if (group.size() < 2)
            continue;

        std::sort(group.begin(), group.end(), [&](std::size_t a, std::size_t b) {
            return downward_ ? (*events)[a].key > (*events)[b].key
                             : (*events)[a].key < (*events)[b].key;
        });

        const Tick perNote = span_ / static_cast<Tick>(group.size() - 1);
        for (std::size_t position = 1; position < group.size(); ++position) {
            MidiEvent& note   = (*events)[group[position]];
            const Tick offset = perNote * static_cast<Tick>(position);
            const Tick end    = note.tick + note.duration;

            note.tick     = tick + offset;
            note.duration = std::max(minimumDuration, end - note.tick);
            changed       = true;
        }
    }

    return changed;
}

void StrumNotesCommand::undo(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr)
        return;

    for (std::size_t position = 0; position < indices_.size(); ++position) {
        const std::size_t index = indices_[position];
        if (index < events->size()) {
            (*events)[index].tick     = previousTicks_[position];
            (*events)[index].duration = previousDurations_[position];
        }
    }
}

// ── ArpeggiateNotesCommand ────────────────────────────────────────────────────

bool ArpeggiateNotesCommand::execute(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr || step_ <= 0)
        return false;

    const NoteIndices selection = noteSelection(indices_, *events);
    if (selection.empty())
        return false;

    // Only chords can arpeggiate; a selection of single notes is a no-op and
    // must not leave an undo entry.
    std::map<Tick, NoteIndices> groups = chordGroups(selection, *events);
    const bool hasChord =
        std::any_of(groups.begin(), groups.end(),
                    [](const auto& entry) { return entry.second.size() >= 2; });
    if (!hasChord)
        return false;

    previousEvents_ = *events;

    std::vector<MidiEvent> rebuilt;
    rebuilt.reserve(events->size());

    std::vector<bool> consumed(events->size(), false);
    for (const auto& [tick, group] : groups)
        if (group.size() >= 2)
            for (const std::size_t index : group)
                consumed[index] = true;

    for (std::size_t index = 0; index < events->size(); ++index)
        if (!consumed[index])
            rebuilt.push_back((*events)[index]);

    for (const auto& [tick, group] : groups) {
        if (group.size() < 2)
            continue;

        // The walk order over the chord's keys.
        std::vector<std::size_t> order = group;
        std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
            return (*events)[a].key < (*events)[b].key;
        });
        if (direction_ == Direction::down) {
            std::reverse(order.begin(), order.end());
        } else if (direction_ == Direction::upDown) {
            // Up then back down without repeating the top: C E G → C E G E …
            std::vector<std::size_t> mirrored = order;
            for (std::size_t position = order.size() - 1; position-- > 1;)
                mirrored.push_back(order[position]);
            order = std::move(mirrored);
        }

        // The chord's span is the longest note in it.
        Tick span = 0;
        for (const std::size_t index : group)
            span = std::max(span, (*events)[index].duration);

        std::size_t walk = 0;
        for (Tick offset = 0; offset < span; offset += step_, ++walk) {
            const MidiEvent& source = (*events)[order[walk % order.size()]];

            MidiEvent note = source;
            note.tick      = tick + offset;
            note.duration  = std::min(step_, span - offset);
            rebuilt.push_back(note);
        }
    }

    std::sort(rebuilt.begin(), rebuilt.end(), [](const MidiEvent& a, const MidiEvent& b) {
        return a.tick != b.tick ? a.tick < b.tick : a.key < b.key;
    });

    *events = std::move(rebuilt);
    return *events != previousEvents_;
}

void ArpeggiateNotesCommand::undo(Project& project)
{
    if (std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_))
        *events = previousEvents_;
}

// ── LegatoNotesCommand ────────────────────────────────────────────────────────

bool LegatoNotesCommand::execute(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr)
        return false;

    indices_ = noteSelection(indices_, *events);
    if (indices_.empty())
        return false;

    previousDurations_.clear();
    for (const std::size_t index : indices_)
        previousDurations_.push_back((*events)[index].duration);

    // Distinct selected starts, ascending: each note reaches the next one.
    std::vector<Tick> starts;
    for (const std::size_t index : indices_)
        starts.push_back((*events)[index].tick);
    std::sort(starts.begin(), starts.end());
    starts.erase(std::unique(starts.begin(), starts.end()), starts.end());

    bool changed = false;
    for (const std::size_t index : indices_) {
        MidiEvent& note   = (*events)[index];
        const auto next   = std::upper_bound(starts.begin(), starts.end(), note.tick);
        if (next == starts.end())
            continue;   // the last chord keeps its length

        const Tick duration = std::max(minimumDuration, *next - note.tick);
        if (note.duration != duration) {
            note.duration = duration;
            changed       = true;
        }
    }

    return changed;
}

void LegatoNotesCommand::undo(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr)
        return;

    for (std::size_t position = 0; position < indices_.size(); ++position) {
        const std::size_t index = indices_[position];
        if (index < events->size())
            (*events)[index].duration = previousDurations_[position];
    }
}

// ── SetNoteLabelCommand ───────────────────────────────────────────────────────

bool SetNoteLabelCommand::execute(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr)
        return false;

    indices_ = noteSelection(indices_, *events);
    if (indices_.empty())
        return false;

    previousLabels_.clear();

    bool changed = false;
    for (const std::size_t index : indices_) {
        MidiEvent& note = (*events)[index];
        previousLabels_.push_back(note.label);
        if (note.label != label_) {
            note.label = label_;
            changed    = true;
        }
    }

    return changed;
}

void SetNoteLabelCommand::undo(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr)
        return;

    for (std::size_t position = 0; position < indices_.size(); ++position) {
        const std::size_t index = indices_[position];
        if (index < events->size())
            (*events)[index].label = previousLabels_[position];
    }
}

} // namespace incdaw::app
