#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>

namespace incdaw::app {

using project::Arrangement;
using project::EntityId;

/// Several timelines in one project (docs/PROJECT_FORMAT.md, format 1.11).
///
/// An arrangement holds clips and markers; everything else — patterns,
/// channels, tracks, the mixer, automation lanes — is shared. That sharing is
/// what makes a second arrangement an alternative layout of the same material
/// rather than a second project.
///
/// Switching is a command like any other, and that is load-bearing rather than
/// tidy: the undo stack is one stack across arrangements, so an edit made in
/// one, a switch, and an undo must walk back through the switch before
/// reaching the edit. A switch that bypassed the stack would leave undo
/// pointing at clips the current timeline does not hold.

class AddArrangementCommand final : public Command {
public:
    /// `copyFrom` valid duplicates that arrangement's clips and markers with
    /// fresh ids; invalid makes an empty one.
    explicit AddArrangementCommand(std::string name, EntityId copyFrom = {})
        : name_(std::move(name)), copyFrom_(copyFrom) {}

    [[nodiscard]] const char* id() const noexcept override { return "arrangement.add"; }
    [[nodiscard]] std::string name() const override
    {
        return copyFrom_.isValid() ? "Duplicate Arrangement" : "Add Arrangement";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId arrangementId() const noexcept { return arrangement_.id; }

private:
    std::string name_;
    EntityId    copyFrom_;

    Arrangement arrangement_;
    EntityId    previousCurrent_;
    std::size_t index_  = 0;
    bool        minted_ = false;
};

/// Removes an arrangement and everything laid out in it. Refuses the last one.
class RemoveArrangementCommand final : public Command {
public:
    explicit RemoveArrangementCommand(EntityId arrangement) : arrangementId_(arrangement) {}

    [[nodiscard]] const char* id() const noexcept override { return "arrangement.remove"; }
    [[nodiscard]] std::string name() const override { return "Remove Arrangement"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    arrangementId_;
    Arrangement arrangement_;
    EntityId    previousCurrent_;
    std::size_t index_ = 0;
};

class RenameArrangementCommand final : public Command {
public:
    RenameArrangementCommand(EntityId arrangement, std::string name)
        : arrangementId_(arrangement), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "arrangement.rename"; }
    [[nodiscard]] std::string name() const override { return "Rename Arrangement"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    arrangementId_;
    std::string name_;
    std::string previousName_;
};

class SetCurrentArrangementCommand final : public Command {
public:
    explicit SetCurrentArrangementCommand(EntityId arrangement)
        : arrangementId_(arrangement) {}

    [[nodiscard]] const char* id() const noexcept override { return "arrangement.setCurrent"; }
    [[nodiscard]] std::string name() const override { return "Switch Arrangement"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId arrangementId_;
    EntityId previous_;
};

} // namespace incdaw::app
