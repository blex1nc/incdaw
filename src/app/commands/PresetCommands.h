#pragma once

// Recalling a preset, as ONE undo entry (A5).
//
// A preset sets several parameters at once, and the merge rule that folds a
// slider drag into one entry cannot help here: merging joins commands that
// touch the SAME parameter, and a preset touches many different ones. So it
// gets its own command per target kind.
//
// The two targets are not symmetrical, and the asymmetry is not an oversight:
//
//   · A channel's instrument values live in the MODEL
//     (Channel::instrumentParameters) and the compiler applies them at every
//     build (D-034), so recalling an instrument preset is a plain model edit.
//
//   · An insert's values live only in the LIVE NODE. A builtin effect reaches
//     project.json as a captured state blob at save time, and there is no
//     field in PluginSlot to edit. Its command therefore carries the writer
//     the shell already uses for a slider move, plus the values it overwrote.
//     Extending the model to hold insert parameter values would be a project
//     format change, which is deliberately not part of this work.

#include "app/Command.h"
#include "engine/dsp/effects/BuiltinEffect.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace incdaw::app {

using project::EntityId;

/// Recalls a preset onto a channel's builtin instrument.
///
/// Only the parameters the preset names are written; the rest keep whatever
/// the channel had, which is what makes a preset that carries an envelope and
/// nothing else useful on top of a sound already dialled in. Undo restores the
/// whole stored vector, so a preset that ADDED entries leaves none behind.
class ApplyInstrumentPresetCommand final : public Command {
public:
    ApplyInstrumentPresetCommand(EntityId channel,
                                 std::vector<engine::dsp::PresetValue> values,
                                 std::string presetName)
        : channelId_(channel), values_(std::move(values)),
          presetName_(std::move(presetName))
    {
    }

    [[nodiscard]] const char* id() const noexcept override
    {
        return "channel.applyInstrumentPreset";
    }

    [[nodiscard]] std::string name() const override
    {
        return presetName_.empty() ? std::string{"Recall Preset"}
                                   : "Recall \"" + presetName_ + "\"";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    EntityId                              channelId_;
    std::vector<engine::dsp::PresetValue> values_;
    std::string                           presetName_;

    std::vector<project::ChannelInstrumentParameter> previous_;
};

/// Recalls a preset onto one insert slot.
///
/// `writer` is called with (parameterId, plainValue) for every value the
/// command applies, in both directions. The shell passes the same block a
/// slider move uses, which resolves the sink FRESH on every write — sinks die
/// with their graph, and an undo may arrive many rebuilds later, so a cached
/// pointer would dangle. A command built with no writer does nothing and
/// reports no change rather than pretending.
class ApplyInsertPresetCommand final : public Command {
public:
    using Writer = std::function<void(std::uint32_t parameterId, double plainValue)>;

    /// `before` is what the live effect held for exactly the parameters
    /// `after` names — captured by the caller, because only it can read the
    /// live node.
    ApplyInsertPresetCommand(std::vector<engine::dsp::PresetValue> before,
                             std::vector<engine::dsp::PresetValue> after,
                             std::string                           presetName,
                             Writer                                writer)
        : before_(std::move(before)), after_(std::move(after)),
          presetName_(std::move(presetName)), writer_(std::move(writer))
    {
    }

    [[nodiscard]] const char* id() const noexcept override
    {
        return "insert.applyPreset";
    }

    [[nodiscard]] std::string name() const override
    {
        return presetName_.empty() ? std::string{"Recall Preset"}
                                   : "Recall \"" + presetName_ + "\"";
    }

    [[nodiscard]] bool execute(Project& project) override;
    void undo(Project& project) override;

private:
    void write(const std::vector<engine::dsp::PresetValue>& values) const;

    std::vector<engine::dsp::PresetValue> before_;
    std::vector<engine::dsp::PresetValue> after_;
    std::string                           presetName_;
    Writer                                writer_;
};

} // namespace incdaw::app
