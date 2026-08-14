#pragma once

#include "app/Command.h"
#include "project/Model.h"

#include <memory>
#include <string>
#include <vector>

namespace incdaw::app {

/// Write-mode automation recording, from arming to landed commands.
///
/// UI-thread only, and deliberately dumb: the mixer reports every armed
/// parameter move with the transport tick it happened at; this collects one
/// stream per (target, parameter), thins it on finish — a hand on a fader
/// emits far more points than an envelope needs — and hands back one
/// WriteAutomationCommand per touched parameter. The session never mutates
/// the project itself; the registry executes what it returns, so a recorded
/// pass undoes like everything else.
///
/// Touch and latch modes are deliberately absent: write is the mode whose
/// semantics need no per-parameter return-value tracking, and the other two
/// layer onto this capture path later rather than complicating it now.
class AutomationWriteSession {
public:
    [[nodiscard]] bool isEnabled() const noexcept { return enabled_; }
    void setEnabled(bool enabled) noexcept { enabled_ = enabled; }

    /// One armed parameter move. Ignored while disabled.
    void capture(project::EntityId target, const std::string& parameterKey,
                 project::Tick tick, double normalizedValue);

    /// True when anything has been captured since the last finish.
    [[nodiscard]] bool hasCaptures() const noexcept { return !streams_.empty(); }

    /// Thins every stream and returns the commands that land them. Clears
    /// the session either way.
    [[nodiscard]] std::vector<std::unique_ptr<Command>> finish();

private:
    struct Stream {
        project::EntityId                          target;
        std::string                                key;
        std::vector<project::AutomationPoint>      points;
    };

    std::vector<Stream> streams_;
    bool                enabled_ = false;
};

} // namespace incdaw::app
