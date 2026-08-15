#pragma once

#include "engine/audio/AudioStream.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

namespace incdaw::engine {

/// The streaming half of one sampler zone.
///
/// The pattern is the classic sampler one: the first `headFrames` of the file
/// live decoded in RAM so a note starts instantly, and a voice that plays past
/// the head continues from a pooled AudioStream whose window has been steered
/// toward the hand-over point since the note began. Pool slots are claimed
/// wait-free at note-on and released when the voice ends. A zone holding more
/// simultaneous long notes than it has slots plays the extras head-only — they
/// fade to silence past the head rather than blocking or allocating, because
/// bounded memory is the contract and the pool size is the bound.
///
/// Streamed zones are forward-only and never sustain-loop: a loop has to be
/// resident to be seamless, so looped and reversed zones are preloaded whole
/// by the compiler instead of streamed.
class SamplerZoneStream {
public:
    static constexpr std::size_t poolSize          = 4;
    static constexpr std::size_t maxSourceChannels = 2;

    /// Opens the file, decodes the head (plus one guard frame so
    /// interpolation at the seam reads real material), opens and prefills the
    /// pool. Build-time only — this allocates and reads the disk. Returns
    /// nullptr with `error` set when the file cannot serve.
    [[nodiscard]] static std::shared_ptr<SamplerZoneStream>
    create(const std::filesystem::path& path, FrameCount headFrames, std::string& error,
           FrameCount segmentFrames = 65536);

    /// The decoded head, `headFrames + 1` frames long (guard included).
    [[nodiscard]] const std::shared_ptr<const AudioFileData>& head() const noexcept
    {
        return head_;
    }

    [[nodiscard]] FrameCount  fileFrames()   const noexcept { return fileFrames_; }
    [[nodiscard]] std::size_t channelCount() const noexcept { return head_->channelCount; }

    /// Wait-free claim of a pool slot; -1 when every slot is taken.
    [[nodiscard]] int claimSlot() noexcept;

    void releaseSlot(int slot) noexcept;

    [[nodiscard]] AudioStream* streamFor(int slot) noexcept;

    /// Hands every pooled stream to the service that keeps windows filled.
    void registerWith(DiskStreamer& streamer);

    /// Underruns across the pool — the exit criterion's measurement.
    [[nodiscard]] std::uint64_t underrunCount() const noexcept;

private:
    std::shared_ptr<const AudioFileData> head_;
    FrameCount                           fileFrames_ = 0;

    struct Slot {
        std::shared_ptr<AudioStream> stream;
        std::atomic<bool>            inUse{false};
    };

    std::array<Slot, poolSize> slots_;
};

} // namespace incdaw::engine
