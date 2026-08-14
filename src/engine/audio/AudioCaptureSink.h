#pragma once

#include "engine/core/Time.h"

#include <cstddef>
#include <cstdint>

namespace incdaw::engine {

/// Receives captured audio from the engine's realtime boundary.
///
/// The engine forwards the platform's capture callback here through one atomic
/// pointer, the same pattern as the render graph swap: the audio thread never
/// takes a lock to find out whether anyone is recording.
///
/// Implementations are bound by the prime directive (docs/AUDIO_ENGINE.md §1):
/// this is called on the input device's realtime thread, which on a two-device
/// rig runs concurrently with rendering.
class AudioCaptureSink {
public:
    virtual ~AudioCaptureSink() = default;

    /// One captured block, planar. `blockHostTimeNanos` is when the first
    /// frame was captured at the device; reported input latency has NOT been
    /// subtracted (see platform::AudioIOCallback::captureAudioBlock).
    virtual void captureAudioBlock(const float* const* inputChannels,
                                   std::size_t         channelCount,
                                   FrameCount          frameCount,
                                   std::uint64_t       blockHostTimeNanos) noexcept = 0;
};

} // namespace incdaw::engine
