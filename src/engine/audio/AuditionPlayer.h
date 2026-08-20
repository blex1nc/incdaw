#pragma once

#include "engine/audio/WavFile.h"
#include "engine/core/AudioBuffer.h"
#include "engine/core/Time.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace incdaw::engine {

/// The Browser's preview: one sample, played once, beside the project.
///
/// A preview has to sound while the transport is stopped, must never appear in
/// the arrangement, and must not cost a graph rebuild — a browser you have to
/// wait for is a browser nobody auditions with. So it is mixed by the engine
/// after the project graph rather than compiled into it.
///
/// The audio thread only ever reads a raw pointer to decoded audio. The
/// shared_ptr that keeps that audio alive is held on this side and released by
/// `collect`, once the engine's block counter proves no callback can still be
/// inside it — the same grace AudioEngine::collectRetiredGraphs gives a
/// replaced graph, for exactly the same reason.
class AuditionPlayer {
public:
    /// Blocks that must complete before a replaced buffer can be freed.
    static constexpr std::uint64_t retirementGraceBlocks = 2;

    /// Starts `audio` from its first frame. Non-realtime thread only.
    ///
    /// `outputRate` is the device rate: a file recorded at another rate is
    /// repitched by reading it faster or slower, the way a sampler zone with
    /// no root-note transposition does. `blockCounter` is the engine's
    /// completed-block count at the moment of the call.
    void play(std::shared_ptr<const AudioFileData> audio, SampleRate outputRate, Sample gain,
              std::uint64_t blockCounter);

    /// The audio thread stops adding on its next block. Safe from any thread.
    void stop() noexcept { playing_.store(false, std::memory_order_release); }

    [[nodiscard]] bool isPlaying() const noexcept { return playing_.load(std::memory_order_acquire); }

    /// Audio thread: sums the preview into `output`. Allocation-free, lock-free,
    /// and a no-op when nothing is playing.
    void render(const AudioBufferView& output) noexcept;

    /// Non-realtime thread: frees decoded audio the audio thread has provably
    /// finished with. `stopped` skips the grace, because a device that is not
    /// running has no callback to be inside anything.
    void collect(std::uint64_t blockCounter, bool stopped = false);

    /// Decoded buffers still held, the live one included. For tests.
    [[nodiscard]] std::size_t retainedCount() const noexcept
    {
        return retired_.size() + (current_ != nullptr ? 1u : 0u);
    }

private:
    struct Retired {
        std::shared_ptr<const AudioFileData> audio;
        std::uint64_t                        retiredAtBlock = 0;
    };

    // Read by the audio thread.
    std::atomic<const AudioFileData*> source_{nullptr};
    std::atomic<double>               rateRatio_{1.0};
    std::atomic<float>                gain_{1.0F};
    std::atomic<std::uint32_t>        generation_{0};
    std::atomic<bool>                 playing_{false};

    // Owned by the audio thread.
    double        position_       = 0.0;
    std::uint32_t seenGeneration_ = 0;

    // Owned by the calling (UI) thread.
    std::shared_ptr<const AudioFileData> current_;
    std::vector<Retired>                 retired_;
};

} // namespace incdaw::engine
