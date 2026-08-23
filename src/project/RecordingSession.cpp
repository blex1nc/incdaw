#include "project/RecordingSession.h"

#include "engine/audio/AudioEdits.h"
#include "platform/HostTime.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <system_error>

namespace incdaw::project {
namespace {

/// take-YYYYMMDD-HHMMSS.wav — sortable, readable, and unique enough for a
/// directory of takes; a second take within the same second gets a suffix.
std::filesystem::path takePath(const std::filesystem::path& directory)
{
    const auto now  = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);

    std::tm parts{};
    localtime_r(&time, &parts);

    char stamp[32];
    std::snprintf(stamp, sizeof(stamp), "take-%04d%02d%02d-%02d%02d%02d",
                  parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday,
                  parts.tm_hour, parts.tm_min, parts.tm_sec);

    std::filesystem::path candidate = directory / (std::string{stamp} + ".wav");

    for (int suffix = 2; std::filesystem::exists(candidate); ++suffix)
        candidate = directory / (std::string{stamp} + "-" + std::to_string(suffix) + ".wav");

    return candidate;
}

} // namespace

std::vector<RecordingSession::Slice> RecordingSession::computeSlices(const TakeGeometry& geometry)
{
    std::vector<Slice> slices;

    if (geometry.takeFrames <= 0)
        return slices;

    const engine::FrameCount loopFrames = geometry.loopEnd - geometry.loopStart;

    if (!geometry.loopEnabled || loopFrames <= 0 || geometry.takeStart >= geometry.loopEnd) {
        // No loop in play: the whole take is one straight slice.
        slices.push_back({geometry.takeStart, 0, geometry.takeFrames, false});
    } else {
        // Walk the file against the timeline: play runs to the loop end, the
        // file keeps going, the timeline snaps back. Works unchanged for a
        // take armed before the loop — the first slice is simply longer.
        engine::FramePosition position = geometry.takeStart;
        engine::FrameCount    consumed = 0;

        while (consumed < geometry.takeFrames) {
            const engine::FrameCount room  = geometry.loopEnd - position;
            const engine::FrameCount chunk = room < geometry.takeFrames - consumed
                                                 ? room : geometry.takeFrames - consumed;

            slices.push_back({position, consumed, chunk, false});
            consumed += chunk;
            position = geometry.loopStart;
        }
    }

    // Punch is a placement decision over a continuous capture: trim every
    // slice to the window, keeping the file offset honest.
    if (geometry.punchOut > geometry.punchIn) {
        std::vector<Slice> punched;

        for (Slice slice : slices) {
            const engine::FramePosition from = std::max(slice.startFrame, geometry.punchIn);
            const engine::FramePosition to =
                std::min(slice.startFrame + slice.length, geometry.punchOut);

            if (from >= to)
                continue;

            slice.sourceOffset += from - slice.startFrame;
            slice.length        = to - from;
            slice.startFrame    = from;
            punched.push_back(slice);
        }

        slices = std::move(punched);
    }

    // Stacked takes: every pass but the newest is muted, so the stack is
    // ready for comping instead of playing all at once.
    for (std::size_t index = 0; index + 1 < slices.size(); ++index)
        slices[index].muted = true;

    return slices;
}

bool RecordingSession::arm(engine::AudioEngine& audioEngine,
                           const std::filesystem::path& directory, std::string& error)
{
    if (recorder_.isRecording()) {
        error = "already recording";
        return false;
    }

    if (audioEngine.inputChannels() == 0) {
        error = "no input device is open; recording needs one";
        return false;
    }

    std::error_code code;
    std::filesystem::create_directories(directory, code);
    if (code) {
        error = "cannot create recordings directory: " + directory.string();
        return false;
    }

    engine::AudioRecorder::Options options;
    options.sampleRate    = audioEngine.sampleRate();
    options.channelCount  = audioEngine.inputChannels();
    options.maxBlockSize  = audioEngine.maxServiceableBlockSize();
    options.latencyFrames = audioEngine.totalInputLatencyFrames();

    if (const auto started = recorder_.start(takePath(directory), options); !started) {
        error = started.error;
        return false;
    }

    // The loop-record contract is sampled here and re-checked at finish: if
    // any of it changed mid-take, the wrap arithmetic is void and placement
    // falls back to one straight clip.
    const auto& transport = audioEngine.transport();
    armLoopStart_   = transport.loopStart();
    armLoopEnd_     = transport.loopEnd();
    armLoopEnabled_ = transport.isLoopEnabled();
    armPosition_    = transport.position();
    armSeekCount_   = transport.seekCount();

    audioEngine.setCaptureSink(&recorder_);
    return true;
}

RecordingSession::Placement RecordingSession::finish(engine::AudioEngine& audioEngine)
{
    Placement placement;

    if (!recorder_.isRecording()) {
        placement.error = "not recording";
        return placement;
    }

    // Read the anchor BEFORE stopping: the sink is cleared and the take
    // drained either way, but the anchor of a still-running engine is the
    // freshest correlation available.
    engine::TimelineAnchor anchor;
    const bool haveAnchor = audioEngine.latestAnchor(anchor);

    audioEngine.setCaptureSink(nullptr);

    const auto take = recorder_.stop();
    if (!take.succeeded) {
        placement.error = take.error;
        return placement;
    }

    placement.path          = take.path;
    placement.frameCount    = take.frameCount;
    placement.channelCount  = audioEngine.inputChannels();
    placement.sampleRate    = audioEngine.sampleRate();
    placement.droppedFrames = take.droppedFrames;

    if (haveAnchor && anchor.playing && take.startHostTimeNanos != 0) {
        // Both the anchor and the take's start ride the same clocks: the
        // anchor pairs an output host time with a timeline frame, and the
        // take's start is a host time with input latency already subtracted.
        placement.startFrame            = anchor.frameAt(take.startHostTimeNanos);
        placement.placedAgainstPlayback = true;

        // Loop recording: the linear map above FOLDS a wrapped take (the
        // anchor frame is inside the loop; subtracting a long take walks out
        // of it). Unfold by choosing the loop pass that puts the take's
        // start where the playhead actually was at arm time, then slice per
        // pass. Engaged only when the contract sampled at arm held.
        const auto& transport   = audioEngine.transport();
        const auto  loopFrames  = armLoopEnd_ - armLoopStart_;
        const bool  contractHeld =
            armLoopEnabled_ && transport.isLoopEnabled()
            && transport.loopStart() == armLoopStart_ && transport.loopEnd() == armLoopEnd_
            && transport.seekCount() == armSeekCount_
            && loopFrames >= 256;   // a loop shorter than a block is not a take workflow

        if (contractHeld) {
            engine::FramePosition takeStart = placement.startFrame;

            const auto passes = static_cast<engine::FramePosition>(std::llround(
                static_cast<double>(armPosition_ - takeStart) / static_cast<double>(loopFrames)));
            takeStart += passes * loopFrames;

            TakeGeometry geometry;
            geometry.takeFrames  = take.frameCount;
            geometry.takeStart   = std::max<engine::FramePosition>(0, takeStart);
            geometry.loopStart   = armLoopStart_;
            geometry.loopEnd     = armLoopEnd_;
            geometry.loopEnabled = true;

            if (punchToLoop_) {
                geometry.punchIn  = armLoopStart_;
                geometry.punchOut = armLoopEnd_;
            }

            placement.sliced = true;
            placement.slices = computeSlices(geometry);

            if (!placement.slices.empty())
                placement.startFrame = placement.slices.front().startFrame;
        }
    } else {
        // Stopped transport: the take lands at the playhead, which is where
        // the user parked it. Negative cannot happen here, but a mapped
        // position from a take armed before playback reached frame 0 can be
        // slightly negative — clamp rather than place a clip before time.
        placement.startFrame = audioEngine.transport().position();
    }

    if (placement.startFrame < 0)
        placement.startFrame = 0;

    placement.succeeded = true;
    return placement;
}

RecordingSession::Placement RecordingSession::keepPreRoll(engine::AudioEngine& audioEngine,
                                                          const std::filesystem::path& directory,
                                                          double seconds)
{
    Placement placement;

    const engine::AudioLogger& logger = audioEngine.inputLogger();

    if (!logger.isEnabled() || logger.channelCount() == 0) {
        placement.error = "the input buffer is not running";
        return placement;
    }

    // The anchor first, and before anything slow: it is the correlation
    // between now and the timeline, and every millisecond spent writing a
    // file makes it staler.
    engine::TimelineAnchor anchor;
    const bool          haveAnchor = audioEngine.latestAnchor(anchor);
    const std::uint64_t nowNanos   = platform::hostTimeNowNanos();

    engine::AudioFileData data;
    if (logger.grab(data) <= 0) {
        placement.error = "nothing in the input buffer yet";
        return placement;
    }

    // Trim to the requested span, keeping the END: the interesting part of a
    // buffer nobody armed is always the part that just happened.
    if (seconds > 0.0 && data.sampleRate > 0.0) {
        const auto wanted = static_cast<engine::FrameCount>(seconds * data.sampleRate);

        if (wanted > 0 && wanted < data.frameCount)
            engine::edits::trimTo(data, {data.frameCount - wanted, data.frameCount});
    }

    if (data.frameCount <= 0) {
        placement.error = "nothing in the input buffer yet";
        return placement;
    }

    std::error_code code;
    std::filesystem::create_directories(directory, code);

    const std::filesystem::path path = takePath(directory);

    if (!engine::WavFile::write(path, data)) {
        placement.error = "could not write the take file";
        return placement;
    }

    placement.path         = path;
    placement.frameCount   = data.frameCount;
    placement.channelCount = data.channelCount;
    placement.sampleRate   = data.sampleRate;

    // Placed so that the window ENDS now. Anything else puts the sound the
    // user just played somewhere they did not play it.
    if (haveAnchor && anchor.playing) {
        placement.startFrame            = anchor.frameAt(nowNanos) - data.frameCount;
        placement.placedAgainstPlayback = true;
    } else {
        placement.startFrame = audioEngine.transport().position() - data.frameCount;
    }

    if (placement.startFrame < 0)
        placement.startFrame = 0;

    placement.succeeded = true;
    return placement;
}

} // namespace incdaw::project
