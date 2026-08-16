#pragma once

#include "app/Command.h"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace incdaw::app {

/// The single mutation path into the project model.
///
/// docs/ARCHITECTURE.md §6: anything that mutates the model outside
/// `CommandRegistry::execute` is a bug. That rule is what makes undo total
/// rather than a feature each editor has to remember to implement.
class CommandRegistry {
public:
    /// A registered action, addressable by id.
    struct Entry {
        std::string                       id;
        std::string                       displayName;
        std::string                       category;
        std::string                       defaultShortcut;   ///< e.g. "Cmd+Z"
        std::function<CommandPtr(void)>   create;
    };

    explicit CommandRegistry(Project& project) noexcept : project_(&project) {}

    /// Registers an action so it can be invoked by id from a menu, a shortcut,
    /// a controller mapping, or a script.
    void registerAction(Entry entry);

    [[nodiscard]] const std::vector<Entry>& actions() const noexcept { return actions_; }

    /// Actions whose display name or id contains `query`, case-insensitively.
    /// This is command search; it exists because the registry exists.
    [[nodiscard]] std::vector<const Entry*> search(const std::string& query) const;

    [[nodiscard]] const Entry* findAction(const std::string& id) const noexcept;

    /// Creates and runs a registered action by id. Returns false if the id is
    /// unknown or the command turned out to be a no-op.
    [[nodiscard]] bool invoke(const std::string& id);

    /// Runs an already-constructed command.
    ///
    /// Executing anything clears the redo stack: once history has diverged,
    /// the old future no longer applies to this project.
    [[nodiscard]] bool execute(CommandPtr command);

    /// Merges into the previous command instead of pushing a new entry, when
    /// the previous one accepts it. Used for continuous gestures.
    [[nodiscard]] bool executeMerging(CommandPtr command);

    [[nodiscard]] bool canUndo() const noexcept { return !undoStack_.empty(); }
    [[nodiscard]] bool canRedo() const noexcept { return !redoStack_.empty(); }

    /// "Undo Move Notes" / "Redo Move Notes", or empty when nothing applies.
    [[nodiscard]] std::string undoName() const;
    [[nodiscard]] std::string redoName() const;

    bool undo();
    bool redo();

    void clearHistory() noexcept;

    [[nodiscard]] std::size_t undoDepth() const noexcept { return undoStack_.size(); }
    [[nodiscard]] std::size_t redoDepth() const noexcept { return redoStack_.size(); }

    /// Identifies the model state the undo stack currently represents. Every
    /// executed command gets a fresh serial, and merging reassigns the top
    /// entry's — so two equal serials mean the model has not diverged, even
    /// though a merge never changes `undoDepth()`. 0 is the empty stack.
    [[nodiscard]] std::size_t stateSerial() const noexcept
    {
        return undoStack_.empty() ? 0 : undoStack_.back().serial;
    }

    /// Records the current state as "what is on disk". Undoing back to this
    /// point — or redoing forward to it — reads as unmodified again.
    void markSavePoint() noexcept { savedSerial_ = stateSerial(); }

    /// True when the model no longer matches the last save point. After
    /// `clearHistory` (a wholesale load replaced the model) this stays true
    /// until the shell marks a new save point.
    [[nodiscard]] bool modifiedSinceSavePoint() const noexcept
    {
        return stateSerial() != savedSerial_;
    }

    /// Oldest entries are discarded beyond this depth. Unbounded history sounds
    /// generous until a long session holds every intermediate state of every
    /// drag in memory.
    void setMaximumDepth(std::size_t depth) noexcept;
    [[nodiscard]] std::size_t maximumDepth() const noexcept { return maximumDepth_; }

    [[nodiscard]] Project& project() noexcept { return *project_; }

private:
    /// A stack entry is the command plus the serial of the state it produced.
    /// The serial travels with the entry through undo and redo, which is what
    /// lets a round trip back to the save point read as unmodified.
    struct StackEntry {
        CommandPtr  command;
        std::size_t serial;
    };

    void trimToMaximumDepth();

    Project*                project_;
    std::vector<Entry>      actions_;
    std::vector<StackEntry> undoStack_;
    std::vector<StackEntry> redoStack_;
    std::size_t             maximumDepth_ = 256;

    std::size_t nextSerial_  = 0;
    std::size_t savedSerial_ = 0;   ///< a fresh registry over a fresh project is clean

    /// `clearHistory` parks the save point here: no reachable state can match
    /// it, so a wholesale-replaced model reads modified until re-marked.
    static constexpr std::size_t unreachableSerial = static_cast<std::size_t>(-1);
};

} // namespace incdaw::app
