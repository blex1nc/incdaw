#include "app/commands/NoteCommands.h"

#include "project/MidiCapture.h"

#include <algorithm>

namespace incdaw::app {
namespace {

project::Pattern* findPattern(Project& project, EntityId id) noexcept
{
    for (project::Pattern& pattern : project.patterns())
        if (pattern.id == id)
            return &pattern;

    return nullptr;
}

/// Drops out-of-range indices and de-duplicates.
///
/// A selection can outlive the notes it referred to — the user deletes some,
/// then undoes something else. Silently ignoring stale indices is right; the
/// alternative is a crash on an edit that looked harmless.
NoteIndices sanitise(NoteIndices indices, std::size_t count)
{
    indices.erase(std::remove_if(indices.begin(), indices.end(),
                                 [count](std::size_t index) { return index >= count; }),
                  indices.end());

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    return indices;
}

constexpr int minimumKey = 0;
constexpr int maximumKey = 127;

/// A note shorter than this is invisible in the editor and silent on playback,
/// which reads to the user as a note that vanished.
constexpr Tick minimumDuration = 1;

} // namespace

// ── AddNoteCommand ────────────────────────────────────────────────────────────

bool AddNoteCommand::execute(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return false;

    index_ = pattern->events.size();
    pattern->events.push_back(note_);
    return true;
}

void AddNoteCommand::undo(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr || index_ >= pattern->events.size())
        return;

    pattern->events.erase(pattern->events.begin() + static_cast<std::ptrdiff_t>(index_));
}

// ── DeleteNotesCommand ────────────────────────────────────────────────────────

std::string DeleteNotesCommand::name() const
{
    return indices_.size() == 1 ? "Delete Note" : "Delete Notes";
}

bool DeleteNotesCommand::execute(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return false;

    indices_ = sanitise(std::move(indices_), pattern->events.size());
    if (indices_.empty())
        return false;

    removed_.clear();
    removed_.reserve(indices_.size());

    for (const std::size_t index : indices_)
        removed_.push_back(pattern->events[index]);

    // Erase back to front so that each removal leaves the earlier indices valid.
    for (auto index = indices_.rbegin(); index != indices_.rend(); ++index)
        pattern->events.erase(pattern->events.begin() + static_cast<std::ptrdiff_t>(*index));

    return true;
}

void DeleteNotesCommand::undo(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return;

    // Front to back, so each insertion restores its note to the position it
    // originally occupied.
    for (std::size_t position = 0; position < indices_.size(); ++position) {
        const std::size_t index = indices_[position];
        if (index > pattern->events.size())
            continue;

        pattern->events.insert(pattern->events.begin() + static_cast<std::ptrdiff_t>(index),
                               removed_[position]);
    }
}

// ── MoveNotesCommand ──────────────────────────────────────────────────────────

bool MoveNotesCommand::execute(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return false;

    indices_ = sanitise(std::move(indices_), pattern->events.size());
    if (indices_.empty() || (tickDelta_ == 0 && keyDelta_ == 0))
        return false;

    // Clamp the whole selection by the most constrained note, so that a drag
    // preserves the shape of a chord instead of flattening it against the edge.
    Tick allowedTickDelta = tickDelta_;
    int  allowedKeyDelta  = keyDelta_;

    for (const std::size_t index : indices_) {
        const MidiEvent& note = pattern->events[index];
        if (note.type != project::MidiEventType::note)
            continue;

        allowedTickDelta = std::max(allowedTickDelta, -note.tick);
        allowedKeyDelta  = std::clamp(allowedKeyDelta, minimumKey - note.key, maximumKey - note.key);
    }

    if (allowedTickDelta == 0 && allowedKeyDelta == 0)
        return false;

    for (const std::size_t index : indices_) {
        MidiEvent& note = pattern->events[index];
        note.tick += allowedTickDelta;
        if (note.type == project::MidiEventType::note)
            note.key += allowedKeyDelta;
    }

    appliedTickDelta_ = allowedTickDelta;
    appliedKeyDelta_  = allowedKeyDelta;
    return true;
}

void MoveNotesCommand::undo(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return;

    for (const std::size_t index : indices_) {
        if (index >= pattern->events.size())
            continue;

        MidiEvent& note = pattern->events[index];
        note.tick -= appliedTickDelta_;
        if (note.type == project::MidiEventType::note)
            note.key -= appliedKeyDelta_;
    }
}

bool MoveNotesCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const MoveNotesCommand*>(&next);
    return other != nullptr && other->pattern_ == pattern_ && other->indices_ == indices_;
}

void MoveNotesCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const MoveNotesCommand*>(&next)) {
        appliedTickDelta_ += other->appliedTickDelta_;
        appliedKeyDelta_  += other->appliedKeyDelta_;
    }
}

// ── ResizeNotesCommand ────────────────────────────────────────────────────────

bool ResizeNotesCommand::execute(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return false;

    indices_ = sanitise(std::move(indices_), pattern->events.size());
    if (indices_.empty() || durationDelta_ == 0)
        return false;

    previousDurations_.clear();
    previousDurations_.reserve(indices_.size());

    bool changed = false;

    for (const std::size_t index : indices_) {
        MidiEvent& note = pattern->events[index];
        previousDurations_.push_back(note.duration);

        if (note.type != project::MidiEventType::note)
            continue;

        const Tick updated = std::max(minimumDuration, note.duration + durationDelta_);
        if (updated != note.duration) {
            note.duration = updated;
            changed = true;
        }
    }

    return changed;
}

void ResizeNotesCommand::undo(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return;

    for (std::size_t position = 0; position < indices_.size(); ++position) {
        const std::size_t index = indices_[position];
        if (index < pattern->events.size())
            pattern->events[index].duration = previousDurations_[position];
    }
}

bool ResizeNotesCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const ResizeNotesCommand*>(&next);
    return other != nullptr && other->pattern_ == pattern_ && other->indices_ == indices_;
}

void ResizeNotesCommand::mergeWith(const Command& next)
{
    // The earlier command already holds the durations from before the gesture
    // began, which is what undo has to restore. Only the requested delta
    // accumulates.
    if (const auto* other = dynamic_cast<const ResizeNotesCommand*>(&next))
        durationDelta_ += other->durationDelta_;
}

// ── SetVelocityCommand ────────────────────────────────────────────────────────

bool SetVelocityCommand::execute(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return false;

    indices_ = sanitise(std::move(indices_), pattern->events.size());
    if (indices_.empty())
        return false;

    // Velocity 0 on a note means note-off; a note in a pattern must never carry
    // it, or it becomes silent with no visible reason.
    const int clamped = std::clamp(velocity_, 1, 127);

    previousVelocities_.clear();
    previousVelocities_.reserve(indices_.size());

    bool changed = false;

    for (const std::size_t index : indices_) {
        MidiEvent& note = pattern->events[index];
        previousVelocities_.push_back(note.value);

        if (note.type == project::MidiEventType::note && note.value != clamped) {
            note.value = clamped;
            changed = true;
        }
    }

    return changed;
}

void SetVelocityCommand::undo(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr)
        return;

    for (std::size_t position = 0; position < indices_.size(); ++position) {
        const std::size_t index = indices_[position];
        if (index < pattern->events.size())
            pattern->events[index].value = previousVelocities_[position];
    }
}

bool SetVelocityCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetVelocityCommand*>(&next);
    return other != nullptr && other->pattern_ == pattern_ && other->indices_ == indices_;
}

void SetVelocityCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetVelocityCommand*>(&next))
        velocity_ = other->velocity_;
}

// ── QuantizeNotesCommand ──────────────────────────────────────────────────────

bool QuantizeNotesCommand::execute(Project& project)
{
    project::Pattern* pattern = findPattern(project, pattern_);
    if (pattern == nullptr || grid_ <= 0)
        return false;

    previousEvents_ = pattern->events;
    project::quantizeNoteStarts(*pattern, grid_, strength_);

    // Quantizing an already-quantized pattern changes nothing and must not
    // leave an undo entry.
    return pattern->events != previousEvents_;
}

void QuantizeNotesCommand::undo(Project& project)
{
    if (project::Pattern* pattern = findPattern(project, pattern_))
        pattern->events = previousEvents_;
}

} // namespace incdaw::app
