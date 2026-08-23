#pragma once

#include "engine/graph/StateIO.h"
#include "engine/instrument/Instrument.h"
#include "platform/AudioUnitHost.h"
#include "plugins/PluginParameterInfo.h"

#include <memory>
#include <string>
#include <vector>

namespace incdaw::plugins {

/// An Audio Unit instrument on a channel.
///
/// AU effects have hosted since Phase 13; instruments were excluded at the
/// enumeration step, so every AU synth on the machine was invisible to a DAW
/// that could already load its effects. The difference is small and total: an
/// instrument has no input bus, it is fed MIDI, and it is asked for audio.
///
/// Implemented as an `engine::Instrument` rather than as a `HostedPlugin`,
/// because that is the interface a channel's sound source has — which means
/// the base class's block splitting applies unchanged, and an AU note lands on
/// the same frame a builtin one does.
///
/// The cost of that is real and stated: the base class calls `renderRange`
/// once per event, so a dense chord renders in several short calls rather than
/// one. Correct, and cheaper than the alternative of an instrument whose notes
/// quantise to the block.
class AudioUnitInstrument final : public engine::Instrument,
                                  public engine::ParameterSink,
                                  public engine::StateIO {
public:
    /// Instantiates `uid` ("type:subtype:manufacturer") as an instrument.
    /// Returns nullptr with `error` set — a channel whose plugin will not load
    /// is silent, never a failed compile.
    [[nodiscard]] static std::unique_ptr<AudioUnitInstrument> create(const std::string& uid,
                                                                     double        sampleRate,
                                                                     std::uint32_t maxFrames,
                                                                     std::string&  error);

    void prepare(engine::SampleRate sampleRate, engine::FrameCount maxBlockSize) override;

    void allNotesOff() noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return name_.c_str(); }

    /// AUs do not report voice counts. Notes started and not yet ended, which
    /// is what the meter is actually asking about.
    [[nodiscard]] int activeVoiceCount() const noexcept override { return heldNotes_; }

    [[nodiscard]] engine::ParameterSink* parameterSink() noexcept override { return this; }

    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    [[nodiscard]] bool saveState(std::vector<std::uint8_t>& out) const override;
    [[nodiscard]] bool loadState(const std::uint8_t* data, std::size_t size) override;

    [[nodiscard]] const std::vector<PluginParameterInfo>& parameters() const noexcept
    {
        return parameters_;
    }

    /// The unit's own editor, embedded the way an insert's is.
    [[nodiscard]] bool hasEditor() const noexcept;
    [[nodiscard]] bool openEditor(void* parentView, std::uint32_t& width, std::uint32_t& height);
    void closeEditor() noexcept;

protected:
    void handleMessage(const engine::MidiMessage& message) noexcept override;
    void renderRange(const engine::AudioBufferView& output,
                     engine::FrameCount             frameCount) noexcept override;

private:
    AudioUnitInstrument() = default;

    std::unique_ptr<platform::AudioUnitHandle> unit_;
    std::vector<PluginParameterInfo>           parameters_;
    std::string                                name_ = "Audio Unit";

    /// The unit renders into these and the result is ADDED to the output: an
    /// instrument shares its buffer with whatever else feeds the channel, and
    /// AudioUnitRender overwrites.
    std::vector<engine::Sample> left_;
    std::vector<engine::Sample> right_;

    int heldNotes_ = 0;
};

} // namespace incdaw::plugins
