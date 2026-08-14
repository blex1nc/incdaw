#include "app/commands/PatternCommands.h"

#include <algorithm>
#include <utility>

namespace incdaw::app {

// ── AddPatternCommand ─────────────────────────────────────────────────────────

bool AddPatternCommand::execute(Project& project)
{
    if (!minted_) {
        const Pattern& created = project.addPattern(name_);
        pattern_ = created;
        index_   = project.patterns().size() - 1;
        minted_  = true;
        return true;
    }

    project.insertPattern(index_, pattern_);
    return true;
}

void AddPatternCommand::undo(Project& project)
{
    (void)project.removePattern(pattern_.id);
}

// ── DuplicatePatternCommand ───────────────────────────────────────────────────

bool DuplicatePatternCommand::execute(Project& project)
{
    if (!minted_) {
        const Pattern* source = project.findPattern(source_);
        if (source == nullptr)
            return false;

        // Copy first: addPattern may reallocate the vector the source lives in.
        Pattern copy = *source;

        Pattern& created = project.addPattern(name_.empty() ? source->name + " copy" : name_);
        const EntityId id = created.id;

        copy.id   = id;
        copy.name = created.name;
        created   = std::move(copy);

        pattern_ = created;
        index_   = project.patterns().size() - 1;
        minted_  = true;
        return true;
    }

    project.insertPattern(index_, pattern_);
    return true;
}

void DuplicatePatternCommand::undo(Project& project)
{
    (void)project.removePattern(pattern_.id);
}

// ── RemovePatternCommand ──────────────────────────────────────────────────────

bool RemovePatternCommand::execute(Project& project)
{
    index_ = project.indexOfPattern(patternId_);
    if (index_ == Project::notFound)
        return false;

    pattern_ = project.patterns()[index_];
    return project.removePattern(patternId_);
}

void RemovePatternCommand::undo(Project& project)
{
    project.insertPattern(index_, pattern_);
}

// ── RenamePatternCommand ──────────────────────────────────────────────────────

bool RenamePatternCommand::execute(Project& project)
{
    Pattern* pattern = project.findPattern(patternId_);
    if (pattern == nullptr || pattern->name == name_)
        return false;

    previousName_ = pattern->name;
    pattern->name = name_;
    return true;
}

void RenamePatternCommand::undo(Project& project)
{
    if (Pattern* pattern = project.findPattern(patternId_))
        pattern->name = previousName_;
}

// ── SetPatternLengthCommand ───────────────────────────────────────────────────

bool SetPatternLengthCommand::execute(Project& project)
{
    Pattern* pattern = project.findPattern(patternId_);
    if (pattern == nullptr)
        return false;

    // A pattern of zero length would loop instantly and play nothing; one tick
    // is the shortest thing that still means something.
    const Tick clamped = std::max<Tick>(length_, 1);
    if (pattern->length == clamped)
        return false;

    previousLength_ = pattern->length;
    length_         = clamped;
    pattern->length = clamped;
    return true;
}

void SetPatternLengthCommand::undo(Project& project)
{
    if (Pattern* pattern = project.findPattern(patternId_))
        pattern->length = previousLength_;
}

bool SetPatternLengthCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetPatternLengthCommand*>(&next);
    return other != nullptr && other->patternId_ == patternId_;
}

void SetPatternLengthCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetPatternLengthCommand*>(&next))
        length_ = other->length_;
}

// ── SetPatternSwingCommand ────────────────────────────────────────────────────

bool SetPatternSwingCommand::execute(Project& project)
{
    Pattern* pattern = project.findPattern(patternId_);
    if (pattern == nullptr)
        return false;

    const double clamped = std::clamp(swing_, 0.0, 1.0);
    if (pattern->swing == clamped)
        return false;

    previousSwing_ = pattern->swing;
    swing_         = clamped;
    pattern->swing = clamped;
    return true;
}

void SetPatternSwingCommand::undo(Project& project)
{
    if (Pattern* pattern = project.findPattern(patternId_))
        pattern->swing = previousSwing_;
}

bool SetPatternSwingCommand::canMergeWith(const Command& next) const noexcept
{
    const auto* other = dynamic_cast<const SetPatternSwingCommand*>(&next);
    return other != nullptr && other->patternId_ == patternId_;
}

void SetPatternSwingCommand::mergeWith(const Command& next)
{
    if (const auto* other = dynamic_cast<const SetPatternSwingCommand*>(&next))
        swing_ = other->swing_;
}

} // namespace incdaw::app
