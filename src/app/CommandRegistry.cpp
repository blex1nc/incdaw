#include "app/CommandRegistry.h"

#include <algorithm>
#include <cctype>

namespace incdaw::app {
namespace {

std::string toLower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text;
}

} // namespace

void CommandRegistry::registerAction(Entry entry)
{
    // Re-registering an id replaces it rather than shadowing it, so a plugin or
    // a later module cannot leave two actions answering to the same name.
    for (Entry& existing : actions_) {
        if (existing.id == entry.id) {
            existing = std::move(entry);
            return;
        }
    }

    actions_.push_back(std::move(entry));
}

const CommandRegistry::Entry* CommandRegistry::findAction(const std::string& id) const noexcept
{
    for (const Entry& entry : actions_)
        if (entry.id == id)
            return &entry;

    return nullptr;
}

std::vector<const CommandRegistry::Entry*> CommandRegistry::search(const std::string& query) const
{
    const std::string needle = toLower(query);
    std::vector<const Entry*> results;

    if (needle.empty()) {
        for (const Entry& entry : actions_)
            results.push_back(&entry);

        return results;
    }

    for (const Entry& entry : actions_) {
        if (toLower(entry.displayName).find(needle) != std::string::npos
            || toLower(entry.id).find(needle) != std::string::npos
            || toLower(entry.category).find(needle) != std::string::npos)
            results.push_back(&entry);
    }

    return results;
}

bool CommandRegistry::invoke(const std::string& id)
{
    const Entry* entry = findAction(id);
    if (entry == nullptr || !entry->create)
        return false;

    return execute(entry->create());
}

bool CommandRegistry::execute(CommandPtr command)
{
    if (command == nullptr)
        return false;

    if (!command->execute(*project_))
        return false;   // a no-op must not leave an undo entry that does nothing

    undoStack_.push_back({std::move(command), ++nextSerial_});

    // History has diverged; the old future no longer describes this project.
    redoStack_.clear();

    trimToMaximumDepth();
    return true;
}

bool CommandRegistry::executeMerging(CommandPtr command)
{
    if (command == nullptr)
        return false;

    if (!command->execute(*project_))
        return false;

    if (!undoStack_.empty() && undoStack_.back().command->canMergeWith(*command)) {
        // Folded into the previous entry: one drag gesture is one undo, not
        // one per mouse move. The fold changed what the entry produces, so it
        // gets a fresh serial — the save point it may have matched is gone.
        undoStack_.back().command->mergeWith(*command);
        undoStack_.back().serial = ++nextSerial_;
        redoStack_.clear();
        return true;
    }

    undoStack_.push_back({std::move(command), ++nextSerial_});
    redoStack_.clear();
    trimToMaximumDepth();
    return true;
}

std::string CommandRegistry::undoName() const
{
    return undoStack_.empty() ? std::string{} : undoStack_.back().command->name();
}

std::string CommandRegistry::redoName() const
{
    return redoStack_.empty() ? std::string{} : redoStack_.back().command->name();
}

bool CommandRegistry::undo()
{
    if (undoStack_.empty())
        return false;

    StackEntry entry = std::move(undoStack_.back());
    undoStack_.pop_back();

    entry.command->undo(*project_);
    redoStack_.push_back(std::move(entry));
    return true;
}

bool CommandRegistry::redo()
{
    if (redoStack_.empty())
        return false;

    StackEntry entry = std::move(redoStack_.back());
    redoStack_.pop_back();

    // Re-executing must succeed: it succeeded once already, on the same state.
    // If it does not, the command is not a faithful inverse of its own undo and
    // the history is no longer trustworthy — so the entry is dropped rather
    // than left on a stack that would desynchronise on the next undo.
    if (!entry.command->execute(*project_))
        return false;

    undoStack_.push_back(std::move(entry));
    return true;
}

void CommandRegistry::clearHistory() noexcept
{
    undoStack_.clear();
    redoStack_.clear();

    // Whatever replaced the model is not what was saved. The shell marks a
    // new save point once it knows what the model now is (e.g. just loaded).
    savedSerial_ = unreachableSerial;
}

void CommandRegistry::setMaximumDepth(std::size_t depth) noexcept
{
    maximumDepth_ = depth == 0 ? 1 : depth;
    trimToMaximumDepth();
}

void CommandRegistry::trimToMaximumDepth()
{
    if (undoStack_.size() <= maximumDepth_)
        return;

    const std::size_t excess = undoStack_.size() - maximumDepth_;
    undoStack_.erase(undoStack_.begin(), undoStack_.begin() + static_cast<std::ptrdiff_t>(excess));
}

} // namespace incdaw::app
