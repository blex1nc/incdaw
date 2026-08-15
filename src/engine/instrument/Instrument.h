#pragma once

#include "engine/core/AudioBuffer.h"
#include "engine/graph/ParameterSink.h"
#include "engine/midi/MidiBuffer.h"

namespace incdaw::engine {

/// Base class for everything that turns MIDI into audio.
///
/// CLAUDE.md §13 is explicit: build the instrument API before any instrument.
/// This is that API, and it is deliberately small — an instrument answers two
/// questions (what does this message mean, what do the next N frames sound
/// like) and nothing else.
///
/// The block-splitting is done HERE, in `processBlock`, not in each instrument.
/// Sample-accurate event timing is the single easiest thing for an instrument
/// author to get wrong, and the failure is quiet: notes land on block
/// boundaries, timing sounds slightly loose, and nobody can point at why. Doing
/// it once in the base class means no instrument can get it wrong.
class Instrument {
public:
    Instrument() = default;
    virtual ~Instrument() = default;

    Instrument(const Instrument&)            = delete;
    Instrument& operator=(const Instrument&) = delete;

    /// Off the audio thread. May allocate.
    virtual void prepare(SampleRate sampleRate, FrameCount maxBlockSize) = 0;

    /// Renders one block, applying each message on the exact frame it falls on.
    ///
    /// Realtime-safe. Final: the splitting contract is not something a subclass
    /// may reinterpret.
    void processBlock(const AudioBufferView& output, const MidiBuffer& midi) noexcept;

    /// Silences everything immediately. Called on transport stop and on panic.
    virtual void allNotesOff() noexcept = 0;

    [[nodiscard]] virtual const char* name() const noexcept = 0;

    /// Voices currently sounding. Exposed for metering and for tests that need
    /// to assert a note actually started or stopped.
    [[nodiscard]] virtual int activeVoiceCount() const noexcept = 0;

    /// The instrument's parameters as a sink, or nullptr when it has none.
    /// The same capability pattern nodes use: automation and MIDI mapping
    /// reach an instrument's parameters through here and nothing else.
    [[nodiscard]] virtual ParameterSink* parameterSink() noexcept { return nullptr; }

protected:
    /// Applies one message. Called at the exact frame it belongs to.
    virtual void handleMessage(const MidiMessage& message) noexcept = 0;

    /// Renders `frameCount` frames into `output`, which is already silenced.
    /// The instrument adds to it rather than overwriting, so several sources
    /// can share a buffer.
    virtual void renderRange(const AudioBufferView& output, FrameCount frameCount) noexcept = 0;
};

} // namespace incdaw::engine
