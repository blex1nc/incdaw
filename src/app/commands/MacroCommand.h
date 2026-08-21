#pragma once

#include "app/Command.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace incdaw::app {

/// Several commands as one undo entry.
///
/// Some gestures are one action to the user and several to the model: dropping
/// a sample into the rack adds a channel *and* loads the file onto it. Pushing
/// those separately means undoing a single drag twice, which is the kind of
/// detail that makes undo feel untrustworthy.
///
/// Children are built lazily, on the first execute, and each builder sees the
/// project as the previous children left it. That is what lets a step target an
/// entity an earlier step has just minted — the id does not exist until the
/// first command has run. After that first pass the children are kept and
/// replayed verbatim, so redo is exact rather than rebuilt from a project that
/// has since moved on.
class MacroCommand final : public Command {
public:
    /// Builds one child. Returning nullptr skips the step.
    using Step = std::function<CommandPtr(Project&)>;

    MacroCommand(std::string identifier, std::string displayName)
        : id_(std::move(identifier)), name_(std::move(displayName)) {}

    /// Adds a child that is already constructed.
    void add(CommandPtr command);

    /// Adds a child to be constructed when the macro first runs.
    void addStep(Step step);

    [[nodiscard]] const char* id() const noexcept override { return id_.c_str(); }
    [[nodiscard]] std::string name() const override { return name_; }

    /// Runs every step in order. Returns false when nothing changed — a macro
    /// whose children all turned out to be no-ops is itself a no-op, and must
    /// not leave an undo entry behind.
    [[nodiscard]] bool execute(Project& project) override;

    /// Undoes the children in reverse. Anything else would apply an inverse to
    /// a state it was not captured against.
    void undo(Project& project) override;

    [[nodiscard]] std::size_t childCount() const noexcept { return children_.size(); }

private:
    /// A child, either handed over ready or still to be built. Kept in one
    /// list so that both kinds run in the order they were added — and stored
    /// as a struct rather than a std::function holding the command, because a
    /// std::function must be copyable and a CommandPtr is not.
    struct Pending {
        CommandPtr ready;
        Step       build;
    };

    std::string id_;
    std::string name_;

    std::vector<Pending>    pending_;
    std::vector<CommandPtr> children_;
    bool                    built_ = false;
};

} // namespace incdaw::app
