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

    /// Oldest entries are discarded beyond this depth. Unbounded history sounds
    /// generous until a long session holds every intermediate state of every
    /// drag in memory.
    void setMaximumDepth(std::size_t depth) noexcept;
    [[nodiscard]] std::size_t maximumDepth() const noexcept { return maximumDepth_; }

    [[nodiscard]] Project& project() noexcept { return *project_; }

private:
    void trimToMaximumDepth();

    Project*                project_;
    std::vector<Entry>      actions_;
    std::vector<CommandPtr> undoStack_;
    std::vector<CommandPtr> redoStack_;
    std::size_t             maximumDepth_ = 256;
};

} // namespace incdaw::app
