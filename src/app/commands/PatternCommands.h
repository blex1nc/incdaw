#pragma once

#include "app/Command.h"
#include "project/Identity.h"
#include "project/Model.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

using project::Channel;
using project::Clip;
using project::EntityId;
using project::Pattern;
using project::Tick;

// ── Patterns ──────────────────────────────────────────────────────────────────

/// Creates an empty pattern.
///
/// The id is minted on the first execute and reused on every redo. Minting a
/// fresh one would leave any clip that referenced the pattern pointing at
/// nothing after undo/redo — the reference would survive while its target
/// changed identity underneath it.
class AddPatternCommand final : public Command {
public:
    explicit AddPatternCommand(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.add"; }
    [[nodiscard]] std::string name() const override { return "Add Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdPattern() const noexcept { return created_; }

private:
    std::string name_;
    EntityId    created_;
};

/// Copies a pattern, including its notes and per-channel settings.
class DuplicatePatternCommand final : public Command {
public:
    explicit DuplicatePatternCommand(EntityId source) : source_(source) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.duplicate"; }
    [[nodiscard]] std::string name() const override { return "Duplicate Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdPattern() const noexcept { return created_; }

private:
    EntityId source_;
    EntityId created_;
};

/// Deletes a pattern and every clip that placed it.
///
/// The clips have to go: a clip whose pattern no longer exists is a silent hole
/// in the arrangement that nothing in the UI can explain. They are captured so
/// that undo restores the arrangement exactly, not just the pattern.
class DeletePatternCommand final : public Command {
public:
    explicit DeletePatternCommand(EntityId pattern) : pattern_(pattern) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.delete"; }
    [[nodiscard]] std::string name() const override { return "Delete Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId          pattern_;
    Pattern           removed_;
    std::size_t       index_ = 0;
    std::vector<Clip> removedClips_;
};

class RenamePatternCommand final : public Command {
public:
    RenamePatternCommand(EntityId pattern, std::string name)
        : pattern_(pattern), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.rename"; }
    [[nodiscard]] std::string name() const override { return "Rename Pattern"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    pattern_;
    std::string name_;
    std::string previous_;
};

/// Changes a pattern's length, or one channel's length inside it.
///
/// A per-channel length shorter than the pattern is what makes the pattern
/// polymetric: that channel loops on its own cycle while the rest of the
/// pattern carries on.
class SetPatternLengthCommand final : public Command {
public:
    SetPatternLengthCommand(EntityId pattern, Tick length, EntityId channel = {})
        : pattern_(pattern), channel_(channel), length_(length) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.setLength"; }
    [[nodiscard]] std::string name() const override { return "Set Pattern Length"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId pattern_;
    EntityId channel_;
    Tick     length_   = 0;
    Tick     previous_ = 0;
    bool     addedSettings_ = false;
};

/// Shuffle, pattern-wide or for one channel.
class SetPatternSwingCommand final : public Command {
public:
    SetPatternSwingCommand(EntityId pattern, double swing, EntityId channel = {})
        : pattern_(pattern), channel_(channel), swing_(swing) {}

    [[nodiscard]] const char* id() const noexcept override { return "pattern.setSwing"; }
    [[nodiscard]] std::string name() const override { return "Set Swing"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId pattern_;
    EntityId channel_;
    double   swing_    = 0.0;
    double   previous_ = 0.0;
    bool     addedSettings_ = false;
};

// ── Channels ──────────────────────────────────────────────────────────────────

class AddChannelCommand final : public Command {
public:
    explicit AddChannelCommand(std::string name) : name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.add"; }
    [[nodiscard]] std::string name() const override { return "Add Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdChannel() const noexcept { return created_; }

private:
    std::string name_;
    EntityId    created_;
};

/// Deletes a channel and every note in every pattern that played on it.
class DeleteChannelCommand final : public Command {
public:
    explicit DeleteChannelCommand(EntityId channel) : channel_(channel) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.delete"; }
    [[nodiscard]] std::string name() const override { return "Delete Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    struct RemovedNotes {
        EntityId                       pattern;
        std::vector<std::size_t>       indices;   ///< ascending, as removed
        std::vector<project::MidiEvent> events;
    };

    EntityId                  channel_;
    Channel                   removed_;
    std::size_t               index_ = 0;
    std::vector<RemovedNotes> removedNotes_;
};

class RenameChannelCommand final : public Command {
public:
    RenameChannelCommand(EntityId channel, std::string name)
        : channel_(channel), name_(std::move(name)) {}

    [[nodiscard]] const char* id() const noexcept override { return "channel.rename"; }
    [[nodiscard]] std::string name() const override { return "Rename Channel"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    channel_;
    std::string name_;
    std::string previous_;
};

/// Volume and pan, in one command rather than two near-identical ones.
///
/// Mergeable, because both are dragged: a fader move produces a command per
/// mouse event and must still cost exactly one undo.
class SetChannelValueCommand final : public Command {
public:
    enum class Property { volume, pan };

    SetChannelValueCommand(EntityId channel, Property property, double value)
        : channel_(channel), property_(property), value_(value) {}

    [[nodiscard]] const char* id() const noexcept override
    {
        return property_ == Property::volume ? "channel.setVolume" : "channel.setPan";
    }

    [[nodiscard]] std::string name() const override
    {
        return property_ == Property::volume ? "Set Channel Volume" : "Set Channel Pan";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId channel_;
    Property property_ = Property::volume;
    double   value_    = 1.0;
    double   previous_ = 1.0;
};

class SetChannelFlagCommand final : public Command {
public:
    enum class Flag { muted, soloed };

    SetChannelFlagCommand(EntityId channel, Flag flag, bool value)
        : channel_(channel), flag_(flag), value_(value) {}

    [[nodiscard]] const char* id() const noexcept override
    {
        return flag_ == Flag::muted ? "channel.setMuted" : "channel.setSoloed";
    }

    [[nodiscard]] std::string name() const override
    {
        return flag_ == Flag::muted ? "Mute Channel" : "Solo Channel";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId channel_;
    Flag     flag_     = Flag::muted;
    bool     value_    = false;
    bool     previous_ = false;
};

// ── Step sequencer ────────────────────────────────────────────────────────────

/// Turns one step of one channel on or off.
///
/// The step sequencer is not a second data model: a step *is* a note, at the
/// step's tick, on that channel, one step long. That is what lets the same
/// pattern be edited as steps and as notes without either view lying about the
/// other.
class ToggleStepCommand final : public Command {
public:
    ToggleStepCommand(EntityId pattern, EntityId channel, int step, int velocity = 100, int key = 60)
        : pattern_(pattern), channel_(channel), step_(step), velocity_(velocity), key_(key) {}

    [[nodiscard]] const char* id() const noexcept override { return "stepsequencer.toggleStep"; }
    [[nodiscard]] std::string name() const override { return "Toggle Step"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    /// True when the step ended up on. Meaningful only after execute.
    [[nodiscard]] bool turnedOn() const noexcept { return turnedOn_; }

private:
    EntityId            pattern_;
    EntityId            channel_;
    int                 step_     = 0;
    int                 velocity_ = 100;
    int                 key_      = 60;

    bool                turnedOn_ = false;
    std::size_t         removedIndex_ = 0;
    project::MidiEvent  removed_;
};

// ── Arrangement placement ─────────────────────────────────────────────────────

/// Places a pattern on a track.
///
/// The clip references the pattern by id and never copies it — which is the
/// whole point of a pattern: place it twice, edit it once, and both placements
/// change.
class AddPatternClipCommand final : public Command {
public:
    AddPatternClipCommand(EntityId track, EntityId pattern, project::FramePosition start,
                          project::FrameCount length = 0)
        : track_(track), pattern_(pattern), start_(start), length_(length) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.addPatternClip"; }
    [[nodiscard]] std::string name() const override { return "Add Pattern Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] EntityId createdClip() const noexcept { return created_; }

private:
    EntityId               track_;
    EntityId               pattern_;
    project::FramePosition start_  = 0;
    project::FrameCount    length_ = 0;
    EntityId               created_;
};

class DeleteClipCommand final : public Command {
public:
    explicit DeleteClipCommand(EntityId clip) : clip_(clip) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.deleteClip"; }
    [[nodiscard]] std::string name() const override { return "Delete Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId    clip_;
    Clip        removed_;
    std::size_t index_ = 0;
};

/// Moves a clip along the timeline, or to another track. Mergeable.
class MoveClipCommand final : public Command {
public:
    MoveClipCommand(EntityId clip, project::FramePosition start, EntityId track = {})
        : clip_(clip), track_(track), start_(start) {}

    [[nodiscard]] const char* id() const noexcept override { return "playlist.moveClip"; }
    [[nodiscard]] std::string name() const override { return "Move Clip"; }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

    [[nodiscard]] bool canMergeWith(const Command& next) const noexcept override;
    void mergeWith(const Command& next) override;

private:
    EntityId               clip_;
    EntityId               track_;
    project::FramePosition start_ = 0;

    project::FramePosition previousStart_ = 0;
    EntityId               previousTrack_;
};

} // namespace incdaw::app
