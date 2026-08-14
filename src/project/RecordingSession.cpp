#include "project/RecordingSession.h"

#include <chrono>
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

} // namespace incdaw::project
