#include "app/commands/PresetCommands.h"

#include <algorithm>

namespace incdaw::app {

// ── ApplyInstrumentPresetCommand ─────────────────────────────────────────────

bool ApplyInstrumentPresetCommand::execute(Project& project)
{
    project::Channel* channel = project.findChannel(channelId_);
    if (channel == nullptr || values_.empty())
        return false;

    std::vector<project::ChannelInstrumentParameter> updated = channel->instrumentParameters;

    for (const engine::dsp::PresetValue& value : values_) {
        const auto existing = std::find_if(
            updated.begin(), updated.end(),
            [&value](const project::ChannelInstrumentParameter& stored) {
                return stored.parameterId == value.parameterId;
            });

        if (existing != updated.end())
            existing->value = value.value;
        else
            updated.push_back({value.parameterId, value.value});
    }

    // An undo entry that changes nothing is worse than no entry at all — and
    // recalling the preset that is already loaded is exactly that.
    if (updated == channel->instrumentParameters)
        return false;

    previous_ = channel->instrumentParameters;
    channel->instrumentParameters = std::move(updated);
    return true;
}

void ApplyInstrumentPresetCommand::undo(Project& project)
{
    if (project::Channel* channel = project.findChannel(channelId_))
        channel->instrumentParameters = previous_;
}

// ── ApplyInsertPresetCommand ─────────────────────────────────────────────────

void ApplyInsertPresetCommand::write(const std::vector<engine::dsp::PresetValue>& values) const
{
    if (!writer_)
        return;

    for (const engine::dsp::PresetValue& value : values)
        writer_(value.parameterId, value.value);
}

bool ApplyInsertPresetCommand::execute(Project& project)
{
    (void)project;   // the values live in the live node, not in the model

    if (!writer_ || after_.empty())
        return false;

    if (before_.size() == after_.size()
        && std::equal(before_.begin(), before_.end(), after_.begin(),
                      [](const engine::dsp::PresetValue& a, const engine::dsp::PresetValue& b) {
                          return a.parameterId == b.parameterId && a.value == b.value;
                      }))
        return false;

    write(after_);
    return true;
}

void ApplyInsertPresetCommand::undo(Project& project)
{
    (void)project;
    write(before_);
}

} // namespace incdaw::app
