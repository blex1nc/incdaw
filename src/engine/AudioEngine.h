#pragma once

#include "engine/core/CallbackProfiler.h"
#include "engine/graph/RenderGraph.h"
#include "engine/transport/Transport.h"
#include "platform/AudioDevice.h"

#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::engine {

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

    /// The single time authority. Nodes read position from the block plan this
    /// produces; nothing else in the engine keeps its own clock.
    [[nodiscard]] Transport&       transport()       noexcept { return transport_; }
    [[nodiscard]] const Transport& transport() const noexcept { return transport_; }

    [[nodiscard]] const CallbackProfiler& profiler() const noexcept { return profiler_; }
    [[nodiscard]] CallbackProfiler&       profiler()       noexcept { return profiler_; }

    [[nodiscard]] double       sampleRate() const noexcept;
    [[nodiscard]] std::int64_t bufferSize() const noexcept;
    [[nodiscard]] std::size_t  outputChannels() const noexcept;
    [[nodiscard]] std::int64_t totalOutputLatencyFrames() const noexcept;
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
                          std::int64_t frameCount) noexcept override;

    void audioDeviceAboutToStart(double sampleRate, std::int64_t bufferSize) override;
    void audioDeviceStopped() override;

    struct RetiredGraph {
        std::unique_ptr<CompiledGraph> graph;
        std::uint64_t                  retiredAtBlock = 0;
    };

    std::unique_ptr<platform::AudioDevice> device_;
    Transport                              transport_;

    std::atomic<CompiledGraph*>                active_{nullptr};
    std::unique_ptr<CompiledGraph>             owned_;      ///< the installed graph
    std::vector<RetiredGraph>                  retired_;
    mutable std::mutex                         retiredMutex_;

    CallbackProfiler           profiler_;
    std::atomic<std::uint64_t> blockCounter_{0};
    std::atomic<std::uint64_t> nonFiniteBlocks_{0};
};

} // namespace incdaw::engine
