#include "app/commands/ChordCommands.h"

#include <algorithm>

namespace incdaw::app {
namespace {

std::vector<MidiEvent>* findEvents(Project& project, EntityId pattern, EntityId channel) noexcept
{
    project::Pattern* found = project.findPattern(pattern);
    return found != nullptr ? found->events(channel) : nullptr;
}

constexpr int minimumKey = 0;
constexpr int maximumKey = 127;

} // namespace

// ── InsertNotesCommand ────────────────────────────────────────────────────────

bool InsertNotesCommand::execute(Project& project)
{
    project::Pattern* pattern = project.findPattern(pattern_);
    if (pattern == nullptr || notes_.empty())
        return false;

    // A stamped note outside the keyboard would be inaudible and unreachable;
    // dropping it here keeps the command's inventory equal to what undo must
    // remove.
    notes_.erase(std::remove_if(notes_.begin(), notes_.end(),
                                [](const MidiEvent& note) {
                                    return note.type == project::MidiEventType::note
                                        && (note.key < minimumKey || note.key > maximumKey);
                                }),
                 notes_.end());
    if (notes_.empty())
        return false;

    std::vector<MidiEvent>& events = pattern->contentFor(channel_).events;
    firstIndex_                    = events.size();
    events.insert(events.end(), notes_.begin(), notes_.end());
    return true;
}

void InsertNotesCommand::undo(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr)
        return;

    const std::size_t last = std::min(events->size(), firstIndex_ + notes_.size());
    if (firstIndex_ >= last)
        return;

    events->erase(events->begin() + static_cast<std::ptrdiff_t>(firstIndex_),
                  events->begin() + static_cast<std::ptrdiff_t>(last));
}

// ── NudgeChordCommand ─────────────────────────────────────────────────────────

bool NudgeChordCommand::execute(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr || steps_ == 0)
        return false;

    // Note-type events only, in range, deduplicated — a CC in the selection is
    // not part of the chord.
    std::vector<std::size_t> selection;
    for (const std::size_t index : indices_) {
        if (index < events->size()
            && (*events)[index].type == project::MidiEventType::note)
            selection.push_back(index);
    }
    std::sort(selection.begin(), selection.end());
    selection.erase(std::unique(selection.begin(), selection.end()), selection.end());
    if (selection.empty())
        return false;
    indices_ = selection;

    std::vector<int> currentKeys;
    currentKeys.reserve(indices_.size());
    for (const std::size_t index : indices_)
        currentKeys.push_back((*events)[index].key);

    // The chord's root names its degree; an unmatched cluster falls back to
    // its lowest note, snapped into the scale.
    const music::ChordDetection detection = music::detectChord(currentKeys);
    const int rootClass = detection.matched
                              ? detection.rootPitchClass
                              : *std::min_element(currentKeys.begin(), currentKeys.end()) % 12;

    const int fromDegree = music::nearestDegree(keyRoot_, scale_, rootClass);
    const int toDegree   = ((fromDegree + steps_) % 7 + 7) % 7;

    std::vector<int> uniqueKeys = currentKeys;
    std::sort(uniqueKeys.begin(), uniqueKeys.end());
    uniqueKeys.erase(std::unique(uniqueKeys.begin(), uniqueKeys.end()), uniqueKeys.end());

    const std::vector<int> targetClasses =
        music::diatonicChordPitchClasses(keyRoot_, scale_, toDegree, uniqueKeys.size());
    const std::vector<int> newKeys = music::voiceLead(uniqueKeys, targetClasses);
    if (newKeys.empty())
        return false;

    previousKeys_ = currentKeys;

    // Each distinct old key follows its own voice: the nth-lowest distinct key
    // becomes the nth-lowest new one, and doubled notes travel with their
    // voice. When the counts cannot line up (a five-note cluster nudged to a
    // four-note seventh), notes fall to the nearest new chord tone instead.
    bool changed = false;
    if (uniqueKeys.size() == newKeys.size()) {
        for (const std::size_t index : indices_) {
            MidiEvent& note = (*events)[index];
            const auto found =
                std::find(uniqueKeys.begin(), uniqueKeys.end(), note.key);
            const std::size_t voice =
                static_cast<std::size_t>(std::distance(uniqueKeys.begin(), found));
            if (note.key != newKeys[voice]) {
                note.key = newKeys[voice];
                changed  = true;
            }
        }
    } else {
        for (const std::size_t index : indices_) {
            MidiEvent& note = (*events)[index];
            int best        = newKeys.front();
            for (const int candidate : newKeys)
                if (std::abs(candidate - note.key) < std::abs(best - note.key))
                    best = candidate;
            if (note.key != best) {
                note.key = best;
                changed  = true;
            }
        }
    }

    return changed;
}

void NudgeChordCommand::undo(Project& project)
{
    std::vector<MidiEvent>* events = findEvents(project, pattern_, channel_);
    if (events == nullptr)
        return;

    for (std::size_t position = 0; position < indices_.size(); ++position) {
        const std::size_t index = indices_[position];
        if (index < events->size() && position < previousKeys_.size())
            (*events)[index].key = previousKeys_[position];
    }
}

bool NudgeChordCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const NudgeChordCommand*>(&next);
    return other != nullptr && other->pattern_ == pattern_ && other->channel_ == channel_
        && other->indices_ == indices_ && other->keyRoot_ == keyRoot_
        && other->scale_ == scale_;
}

void NudgeChordCommand::mergeWith(const Command& next)
{
    // The earlier command keeps previousKeys_ — undo returns to before the
    // gesture. Only the walked distance accumulates.
    if (const auto* other = dynamic_cast<const NudgeChordCommand*>(&next))
        steps_ += other->steps_;
}

} // namespace incdaw::app
