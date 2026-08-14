#pragma once

#include "engine/AudioEngine.h"
#include "engine/audio/AudioRecorder.h"

#include <filesystem>
#include <string>

namespace incdaw::project {

/// Drives one recording take from arm to placement.
///
/// Lives in `project/` because it sits exactly on the seam: below it the
/// engine captures samples against the host clock; above it the model wants a
/// clip at a timeline frame. The translation — take start, minus reported
/// input latency (done by the recorder), mapped through the engine's timeline
/// anchor — happens here, in one place.
///
/// The session does NOT mutate the Project. It returns a Placement and the
/// app layer turns that into an undoable command; a recorder that edited the
/// model directly would be the one mutation path that bypassed undo.
class RecordingSession {
public:
    struct Placement {
        bool        succeeded = false;
        std::string error;

        std::filesystem::path path;
        engine::FramePosition startFrame   = 0;
        engine::FrameCount    frameCount   = 0;
        std::size_t           channelCount = 0;
        engine::SampleRate    sampleRate   = 0.0;
        std::uint64_t         droppedFrames = 0;

        /// True when the transport was rolling and the anchor mapped the take
        /// onto the moving timeline; false when it landed at the playhead of a
        /// stopped transport.
        bool placedAgainstPlayback = false;

        explicit operator bool() const noexcept { return succeeded; }
    };

    /// Starts capturing into a fresh take file inside `directory` (created if
    /// missing) and installs the recorder as the engine's capture sink.
    /// Fails when the engine has no input channels open.
    [[nodiscard]] bool arm(engine::AudioEngine& audioEngine,
                           const std::filesystem::path& directory, std::string& error);

    /// Stops, drains to disk, and computes where the take belongs.
    ///
    /// While the transport was playing, the take's latency-compensated start
    /// is mapped through the engine's timeline anchor — both sides of that
    /// mapping advance on the output device's clock, so the extrapolation is
    /// exact. Seeking or loop-wrapping DURING a take is not yet handled (the
    /// map is linear); loop recording is separate, outstanding Phase 12 work.
    [[nodiscard]] Placement finish(engine::AudioEngine& audioEngine);

    [[nodiscard]] bool isRecording() const noexcept { return recorder_.isRecording(); }

    /// Live counters for the UI while recording.
    [[nodiscard]] engine::FrameCount capturedFrames() const noexcept { return recorder_.capturedFrames(); }
    [[nodiscard]] std::uint64_t      droppedFrames()  const noexcept { return recorder_.droppedFrames(); }

private:
    engine::AudioRecorder recorder_;
};

} // namespace incdaw::project
