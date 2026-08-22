#pragma once

#include "engine/audio/AudioLogger.h"
#include "engine/audio/AuditionPlayer.h"
#include "engine/core/CallbackProfiler.h"
#include "engine/core/SampleRingBuffer.h"
#include "engine/graph/RenderGraph.h"
#include "engine/midi/MidiBuffer.h"
#include "engine/midi/MidiInput.h"
#include "engine/midi/MidiClock.h"
#include "engine/midi/MidiOutput.h"
#include "engine/transport/Transport.h"
#include "platform/AudioDevice.h"

#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::engine {

class AudioCaptureSink;

/// One rendered block's correlation between the host clock and the timeline.
///
/// This is how a recorded take finds its place: the recorder knows the host
/// time its audio was captured; this says which timeline frame was being heard
/// at a given host time. Both advance on the same output clock, so the
/// mapping extrapolates linearly between blocks.
struct TimelineAnchor {
    std::uint64_t hostTimeNanos = 0;   ///< when the block's first frame is heard
    FramePosition timelineFrame = 0;   ///< the timeline position of that frame
    double        sampleRate    = 0.0;
    bool          playing       = false;

    /// The timeline frame heard at `whenNanos`. Meaningless unless `playing`.
    [[nodiscard]] FramePosition frameAt(std::uint64_t whenNanos) const noexcept
    {
        const double deltaSeconds =
            (static_cast<double>(whenNanos) - static_cast<double>(hostTimeNanos)) * 1.0e-9;
        const double frames = deltaSeconds * sampleRate;
        return timelineFrame
             + static_cast<FramePosition>(frames >= 0.0 ? frames + 0.5 : frames - 0.5);
    }
};

/// Connects an audio device to a render graph.
///
/// Owns the realtime boundary: everything below it is realtime-safe, everything
/// above it is not.
class AudioEngine final : public platform::AudioIOCallback {
public:
    AudioEngine();
    ~AudioEngine() override;

    AudioEngine(const AudioEngine&)            = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    [[nodiscard]] std::vector<platform::AudioDeviceInfo> availableDevices() const;

    [[nodiscard]] bool start(const platform::AudioDeviceConfig& config, std::string& error);
    void stop();

    [[nodiscard]] bool isRunning() const noexcept;

    /// Installs a graph. Called from a non-realtime thread.
    ///
    /// The swap itself is a single atomic store, so the audio thread never sees
    /// a half-installed graph. The graph it replaces is not destroyed here —
    /// see `collectRetiredGraphs`.
    void setGraph(std::unique_ptr<CompiledGraph> graph);

    /// Destroys graphs the audio thread has provably finished with.
    ///
    /// Freeing a retired graph immediately would be a use-after-free: the audio
    /// thread may be midway through rendering it. Instead a graph is retired
    /// with the block counter at the moment of the swap, and is only released
    /// once the counter has advanced past it — by which point no callback can
    /// still hold a pointer to it.
    ///
    /// Called from a non-realtime thread; safe to call often.
    void collectRetiredGraphs();

    [[nodiscard]] std::size_t retiredGraphCount() const;

    /// Installs (or clears, with nullptr) the destination for captured audio.
    /// Same single-atomic-store pattern as setGraph. The sink must stay alive
    /// until after the device has stopped or a subsequent clear has had one
    /// block's grace — in practice the recorder outlives the session, and its
    /// own state machine makes calls after stop() harmless no-ops.
    void setCaptureSink(AudioCaptureSink* sink) noexcept
    {
        captureSink_.store(sink, std::memory_order_release);
    }

    [[nodiscard]] AudioCaptureSink* captureSink() const noexcept
    {
        return captureSink_.load(std::memory_order_acquire);
    }

    /// Input monitoring: while enabled, captured blocks are also interleaved
    /// into the monitor ring, which an InputMonitorNode in the graph drains
    /// on the output side. The ring lives as long as the engine (allocated
    /// once, never resized), which is what lets graph nodes keep a raw
    /// pointer across device restarts.
    void setMonitoringEnabled(bool enabled) noexcept
    {
        monitorEnabled_.store(enabled, std::memory_order_release);
    }

    [[nodiscard]] bool isMonitoringEnabled() const noexcept
    {
        return monitorEnabled_.load(std::memory_order_acquire);
    }

    [[nodiscard]] SampleRingBuffer* monitorRing() noexcept { return &monitorRing_; }

    /// The Audio Logger: the master's last minute, continuously (§8 of
    /// docs/AUDIO_ENGINE.md). Fed at the end of every rendered block while
    /// enabled; prepared when the device starts.
    [[nodiscard]] AudioLogger&       logger()       noexcept { return logger_; }
    [[nodiscard]] const AudioLogger& logger() const noexcept { return logger_; }

    /// The Browser's preview source, mixed after the project graph.
    ///
    /// Deliberately not a graph node: a preview must sound with the transport
    /// stopped and must never cost a rebuild (engine/audio/AuditionPlayer.h).
    [[nodiscard]] AuditionPlayer& audition() noexcept { return audition_; }

    /// The most recent block's host-time <-> timeline correlation.
    ///
    /// Published from the audio thread through a seqlock — the audio thread
    /// never blocks, the reader retries on the rare torn read. Returns false
    /// before the first block. `anchor.playing` is false while stopped, in
    /// which case the frame mapping must not be used (the timeline was not
    /// advancing, so no linear relation exists).
    [[nodiscard]] bool latestAnchor(TimelineAnchor& anchor) const noexcept
    {
        for (;;) {
            const std::uint64_t before = anchorVersion_.load(std::memory_order_acquire);
            if (before == 0)
                return false;
            if (before & 1u)
                continue;   // writer mid-update

            anchor = anchor_;

            const std::uint64_t after = anchorVersion_.load(std::memory_order_acquire);
            if (after == before)
                return true;
        }
    }

    /// The single time authority. Nodes read position from the block plan this
    /// produces; nothing else in the engine keeps its own clock.
    /// MIDI input, timestamp-aligned to the audio blocks this engine renders.
    [[nodiscard]] MidiInput&       midiInput()       noexcept { return midiInput_; }
    [[nodiscard]] const MidiInput& midiInput() const noexcept { return midiInput_; }

    /// MIDI output, timestamped so that what INCDAW generates is heard on the
    /// frame it was written for. The device it writes to is chosen by the
    /// shell; the sender thread runs for as long as the audio device does.
    [[nodiscard]] MidiOutput&       midiOutput()       noexcept { return midiOutput_; }
    [[nodiscard]] const MidiOutput& midiOutput() const noexcept { return midiOutput_; }

    /// MIDI beat clock generation. Off unless the user asks for it in
    /// Settings; the pulses it produces join the block's outgoing buffer.
    [[nodiscard]] MidiClockGenerator&       midiClock()       noexcept { return midiClock_; }
    [[nodiscard]] const MidiClockGenerator& midiClock() const noexcept { return midiClock_; }

    /// The messages collected for the block just rendered. Read from the audio
    /// thread by nodes; exposed here for diagnostics and tests.
    [[nodiscard]] const MidiBuffer& lastBlockMidi() const noexcept { return blockMidi_; }

    [[nodiscard]] Transport&       transport()       noexcept { return transport_; }
    [[nodiscard]] const Transport& transport() const noexcept { return transport_; }

    [[nodiscard]] const CallbackProfiler& profiler() const noexcept { return profiler_; }
    [[nodiscard]] CallbackProfiler&       profiler()       noexcept { return profiler_; }

    [[nodiscard]] double       sampleRate() const noexcept;
    [[nodiscard]] std::int64_t bufferSize() const noexcept;

    /// Largest block the device may hand the callback; what the render graph's
    /// buffers must be sized for. See AudioDevice::maxServiceableBlockSize.
    [[nodiscard]] std::int64_t maxServiceableBlockSize() const noexcept;
    [[nodiscard]] std::size_t  outputChannels() const noexcept;
    [[nodiscard]] std::size_t  inputChannels() const noexcept;
    [[nodiscard]] std::int64_t totalOutputLatencyFrames() const noexcept;
    [[nodiscard]] std::int64_t totalInputLatencyFrames() const noexcept;
    [[nodiscard]] std::string  deviceName() const;

    /// Blocks the audio thread has completed since the engine started.
    [[nodiscard]] std::uint64_t blockCount() const noexcept { return blockCounter_.load(std::memory_order_acquire); }

    /// Blocks in which the graph produced a NaN or infinity.
    ///
    /// Counted rather than ignored: a single NaN propagates through every
    /// downstream node and silences the master, and the symptom looks nothing
    /// like the cause (docs/AUDIO_ENGINE.md §10).
    [[nodiscard]] std::uint64_t nonFiniteBlockCount() const noexcept { return nonFiniteBlocks_.load(std::memory_order_relaxed); }

private:
    void renderAudioBlock(float* const* outputChannels, std::size_t channelCount,
                          std::int64_t frameCount, std::uint64_t blockHostTimeNanos) noexcept override;

    void captureAudioBlock(const float* const* inputChannels, std::size_t channelCount,
                           std::int64_t frameCount, std::uint64_t blockHostTimeNanos) noexcept override;

    void audioDeviceAboutToStart(double sampleRate, std::int64_t bufferSize) override;
    void audioDeviceStopped() override;

    struct RetiredGraph {
        std::unique_ptr<CompiledGraph> graph;
        std::uint64_t                  retiredAtBlock = 0;
    };

    std::unique_ptr<platform::AudioDevice> device_;
    Transport                              transport_;
    MidiInput                              midiInput_;
    MidiOutput                             midiOutput_;
    MidiClockGenerator                     midiClock_;
    MidiBuffer                             blockMidi_;
    MidiBuffer                             outputMidi_;   ///< what this block sends out
    MidiBuffer                             segmentMidi_;

    std::atomic<CompiledGraph*>                active_{nullptr};
    std::atomic<AudioCaptureSink*>             captureSink_{nullptr};
    std::unique_ptr<CompiledGraph>             owned_;      ///< the installed graph
    std::vector<RetiredGraph>                  retired_;
    mutable std::mutex                         retiredMutex_;

    CallbackProfiler           profiler_;
    std::atomic<std::uint64_t> blockCounter_{0};
    std::atomic<std::uint64_t> nonFiniteBlocks_{0};

    /// Seqlock around `anchor_`: odd while the audio thread writes, bumped to
    /// even when the payload is consistent. Zero means "never published".
    std::atomic<std::uint64_t> anchorVersion_{0};
    TimelineAnchor             anchor_;

    AuditionPlayer      audition_;
    SampleRingBuffer    monitorRing_;
    std::vector<Sample> monitorScratch_;   ///< interleave scratch, sized on start
    std::atomic<bool>   monitorEnabled_{false};

    AudioLogger logger_;
};

} // namespace incdaw::engine
