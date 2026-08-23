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
    /// One placed piece of the take: a range of the FILE at a timeline
    /// position. A straight take is one slice; a loop-recorded take is one
    /// per pass over the loop, every pass but the last muted — stacked
    /// takes, ready for comping rather than playing all at once.
    struct Slice {
        engine::FramePosition startFrame   = 0;
        engine::FrameCount    sourceOffset = 0;
        engine::FrameCount    length       = 0;
        bool                  muted        = false;
    };

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

        /// The take cut against the loop (and punch range) it was recorded
        /// under. Meaningful only when `sliced`; empty-with-sliced means the
        /// punch window excluded the whole take and nothing lands.
        bool               sliced = false;
        std::vector<Slice> slices;

        explicit operator bool() const noexcept { return succeeded; }
    };

    /// Everything the slicing arithmetic needs, gathered at arm and finish.
    /// Separated out so the algorithm is a pure function tests can hammer
    /// without a device or an engine anywhere in sight.
    struct TakeGeometry {
        engine::FrameCount    takeFrames  = 0;
        engine::FramePosition takeStart   = 0;   ///< timeline frame of the take's first frame, unfolded
        engine::FramePosition loopStart   = 0;
        engine::FramePosition loopEnd     = 0;
        bool                  loopEnabled = false;

        /// Punch window; equal values disable punching.
        engine::FramePosition punchIn  = 0;
        engine::FramePosition punchOut = 0;
    };

    /// Cuts a take into placed slices: at every loop wrap the file continues
    /// but the timeline snaps back, so each pass becomes its own slice; the
    /// punch window then trims every slice (adjusting its source offset), and
    /// every slice but the last audible one is muted.
    [[nodiscard]] static std::vector<Slice> computeSlices(const TakeGeometry& geometry);

    /// Starts capturing into a fresh take file inside `directory` (created if
    /// missing) and installs the recorder as the engine's capture sink.
    /// Fails when the engine has no input channels open.
    ///
    /// The loop state, seek counter and playhead are sampled here: if the
    /// loop range survives the whole take untouched and no explicit seek
    /// happens, a wrapped take is sliced per pass; any interference falls
    /// back to the straight single-clip placement.
    [[nodiscard]] bool arm(engine::AudioEngine& audioEngine,
                           const std::filesystem::path& directory, std::string& error);

    /// Punch: when armed, only audio inside the loop range (as it stands at
    /// arm time) lands in the timeline. The capture itself is continuous —
    /// punching is a placement decision, not a capture gate.
    void setPunchToLoop(bool enabled) noexcept { punchToLoop_ = enabled; }
    [[nodiscard]] bool punchToLoop() const noexcept { return punchToLoop_; }

    /// Stops, drains to disk, and computes where the take belongs.
    ///
    /// While the transport was playing, the take's latency-compensated start
    /// is mapped through the engine's timeline anchor — both sides of that
    /// mapping advance on the output device's clock, so the extrapolation is
    /// exact. Seeking or loop-wrapping DURING a take is not yet handled (the
    /// map is linear); loop recording is separate, outstanding Phase 12 work.
    [[nodiscard]] Placement finish(engine::AudioEngine& audioEngine);

    /// Keeps what the input has been doing, without a recording having been
    /// started — the take nobody armed.
    ///
    /// The engine's input logger holds the last minute of the input whether
    /// or not anything asked for it (engine/audio/AudioLogger.h). This writes
    /// that window to a file in `directory` and places it so that its END is
    /// NOW: the sound just played is the sound that lands under the playhead
    /// it was played against, which is the only placement that makes the
    /// feature worth having.
    ///
    /// `seconds` trims the window to the most recent span; zero keeps all of
    /// it. Fails when the buffer is off, unprepared, or empty.
    [[nodiscard]] Placement keepPreRoll(engine::AudioEngine&         audioEngine,
                                        const std::filesystem::path& directory,
                                        double                       seconds = 0.0);

    [[nodiscard]] bool isRecording() const noexcept { return recorder_.isRecording(); }

    /// Live counters for the UI while recording.
    [[nodiscard]] engine::FrameCount capturedFrames() const noexcept { return recorder_.capturedFrames(); }
    [[nodiscard]] std::uint64_t      droppedFrames()  const noexcept { return recorder_.droppedFrames(); }

private:
    engine::AudioRecorder recorder_;

    bool                  punchToLoop_ = false;

    // Sampled at arm, checked at finish.
    engine::FramePosition armLoopStart_   = 0;
    engine::FramePosition armLoopEnd_     = 0;
    bool                  armLoopEnabled_ = false;
    engine::FramePosition armPosition_    = 0;
    std::uint32_t         armSeekCount_   = 0;
};

} // namespace incdaw::project
