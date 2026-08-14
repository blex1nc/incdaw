#pragma once

#include "project/Model.h"

#include <memory>
#include <string>
#include <vector>

namespace incdaw::app {

using project::Project;

/// One user-visible action.
///
/// CLAUDE.md §26 and docs/ARCHITECTURE.md §6: *every* action is a command, with
/// no exceptions. This is not ceremony — it is the single mechanism from which
/// undo/redo, keyboard shortcuts, menus, macros, MIDI learn, command search and
/// eventual scripting all fall out. FL Studio's 2026 assistant can reorganise
/// tracks and set levels precisely because FL exposes such a surface; a DAW
/// that mutates its model directly from UI code cannot add one later without
/// rewriting every editor.
///
/// A command must be able to undo itself exactly. That is why commands capture
/// the state they overwrite when they execute, rather than recomputing it: the
/// project may have changed in ways the command cannot re-derive.
class Command {
public:
    virtual ~Command() = default;

    Command(const Command&)            = delete;
    Command& operator=(const Command&) = delete;

    /// Stable identifier, e.g. "pianoroll.quantize". Used by shortcuts, menus,
    /// scripting and command search — never shown to the user directly.
    [[nodiscard]] virtual const char* id() const noexcept = 0;

    /// Shown in the undo menu: "Undo <name>".
    [[nodiscard]] virtual std::string name() const = 0;

    /// Applies the change. Returns false if it turned out to be a no-op, in
    /// which case it is not pushed onto the undo stack — an undo entry that
    /// changes nothing is worse than no entry at all.
    [[nodiscard]] virtual bool execute(Project& project) = 0;

    virtual void undo(Project& project) = 0;

    /// Whether `next` can be folded into this command.
    ///
    /// Dragging a note produces one command per mouse move; without merging,
    /// a single gesture would need dozens of undos to reverse. Commands only
    /// merge when they are the same kind, act on the same target, and arrive
    /// close together in time — which the registry decides, not the command.
    [[nodiscard]] virtual bool canMergeWith(const Command& next) const noexcept
    {
        (void)next;
        return false;
    }

    virtual void mergeWith(const Command& next) { (void)next; }

protected:
    Command() = default;
};

using CommandPtr = std::unique_ptr<Command>;

} // namespace incdaw::app
