#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace incdaw::app {

using project::EntityId;
using project::Pattern;
using project::Tick;

/// Pattern list edits.
///
/// A pattern placed several times in an arrangement is one pattern: these
/// commands therefore operate on the pattern itself, and every placement of it
/// follows. That is the whole point of the pattern workflow, and Phase 9's
/// clips reference patterns by id so it keeps holding.

class AddPatternCommand final : public Command {
public:
    explicit AddPatternCommand(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.add"; }
    [[nodiscard]] std::string name() const override { return "Add Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// Valid only after the first execute.
    [[nodiscard]] EntityId patternId() const noexcept { return pattern_.id; }

private:
    std::string name_;
    Pattern     pattern_;
    std::size_t index_  = 0;
    bool        minted_ = false;
};

/// Copies a pattern, content and all.
///
/// A copy, not a reference: duplicating exists precisely so the copy can be
/// edited without touching the original.
class DuplicatePatternCommand final : public Command {
public:
    DuplicatePatternCommand(EntityId source, std::string name)
        : source_(source), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.duplicate"; }
    [[nodiscard]] std::string name() const override { return "Duplicate Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId patternId() const noexcept { return pattern_.id; }

private:
    EntityId    source_;
    std::string name_;
    Pattern     pattern_;
    std::size_t index_  = 0;
    bool        minted_ = false;
};

/// Removes a pattern.
///
/// Clips referencing it are deliberately left alone: there is no arrangement
/// yet (Phase 9), and when there is, removing a pattern must take its clips
/// with it — that belongs in the same command, added there rather than guessed
/// at here.
class RemovePatternCommand final : public Command {
public:
    explicit RemovePatternCommand(EntityId pattern) : patternId_(pattern) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.remove"; }
    [[nodiscard]] std::string name() const override { return "Remove Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    patternId_;
    Pattern     pattern_;
    std::size_t index_ = 0;
};

class RenamePatternCommand final : public Command {
public:
    RenamePatternCommand(EntityId pattern, std::string name)
        : patternId_(pattern), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.rename"; }
    [[nodiscard]] std::string name() const override { return "Rename Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    patternId_;
    std::string name_;
    std::string previousName_;
};

/// Pattern length in ticks. Notes past the new end are kept, not truncated:
/// shortening a pattern is a framing decision, and destroying the material
/// outside the frame makes it irreversible in practice even with undo.
class SetPatternLengthCommand final : public Command {
public:
    SetPatternLengthCommand(EntityId pattern, Tick length)
        : patternId_(pattern), length_(length) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.setLength"; }
    [[nodiscard]] std::string name() const override { return "Set Pattern Length"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId patternId_;
    Tick     length_         = 0;
    Tick     previousLength_ = 0;
};

/// Shuffle, 0..1. Mergeable, so dragging the control is one undo.
/// A pattern's colour, which is also its clips' colour in the playlist.
class SetPatternColourCommand final : public Command {
public:
    SetPatternColourCommand(EntityId pattern, std::uint32_t colour, bool recolourClips = true)
        : patternId_(pattern), colour_(colour), recolourClips_(recolourClips) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.setColour"; }
    [[nodiscard]] std::string name() const override { return "Set Pattern Colour"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct PreviousClip {
        EntityId      id;
        std::uint32_t colour = 0u;
    };

    EntityId                  patternId_;
    std::uint32_t             colour_          = 0u;
    bool                      recolourClips_   = true;
    std::uint32_t             previousColour_  = 0u;
    std::vector<PreviousClip> previousClips_;
};

class SetPatternSwingCommand final : public Command {
public:
    SetPatternSwingCommand(EntityId pattern, double swing)
        : patternId_(pattern), swing_(swing) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.setSwing"; }
    [[nodiscard]] std::string name() const override { return "Set Swing"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId patternId_;
    double   swing_         = 0.0;
    double   previousSwing_ = 0.0;
};

} // namespace incdaw::app
