#pragma once

#include "app/Command.h"
#include "project/Model.h"

#include <memory>
#include <string>
#include <vector>

namespace incdaw::app {

/// Automation recording — write, touch and latch — from arming to landed
/// commands.
///
/// UI-thread only, and deliberately dumb: the mixer reports every armed
/// parameter move with the transport tick it happened at; this collects one
/// stream per (target, parameter), thins on finish — a hand on a fader
/// emits far more points than an envelope needs — and hands back
/// WriteAutomationCommands. The session never mutates the project itself;
/// the registry executes what it returns, so a recorded pass undoes like
/// everything else.
///
/// The three modes differ only in how segments close, because the landing
/// command replaces exactly the written range and keeps everything outside:
///  - write: one segment from the first move to the last — everything the
///    pass spanned is the pass, gaps between drags included.
///  - touch: every drag is its OWN segment (gestureEnded closes it), so the
///    lane's existing envelope survives between drags.
///  - latch: like write, plus finish() extends the segment to the end tick
///    at the last value — releasing the fader keeps writing where it was.
class AutomationWriteSession {
public:
    enum class WriteMode { write, touch, latch };

    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    [[nodiscard]] WriteMode mode() const noexcept { return mode_; }
    void setMode(WriteMode mode) noexcept { mode_ = mode; }

    /// One armed parameter move. Ignored while disabled.
    void capture(project::EntityId target, const std::string& parameterKey,
                 project::Tick tick, double normalizedValue);

    /// The armed control was released. Touch mode closes the segment here;
    /// the other modes do not care.
    void gestureEnded(project::EntityId target, const std::string& parameterKey);

    /// True when anything has been captured since the last finish.
    [[nodiscard]] bool hasCaptures() const noexcept
    {
        return !streams_.empty() || !closedSegments_.empty();
    }

    /// Thins every segment and returns the commands that land them. Clears
    /// the session either way. `endTick` is where the pass stopped — latch
    /// mode holds each parameter's last value out to it; the default skips
    /// the hold (and is what the write-mode tests predate).
    [[nodiscard]] std::vector<std::unique_ptr<Command>> finish(project::Tick endTick = -1);

private:
    struct Stream {
        project::EntityId                          target;
        std::string                                key;
        std::vector<project::AutomationPoint>      points;
    };

    std::vector<Stream> streams_;
    std::vector<Stream> closedSegments_;   ///< touch mode's finished drags
    bool                enabled_ = false;
    WriteMode           mode_    = WriteMode::write;
};

} // namespace incdaw::app
