#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::platform {

/// Deliberately expressed in plain types rather than engine types: `platform/`
/// sits below `engine/` and must not know it exists (docs/ARCHITECTURE.md §2).

struct AudioDeviceInfo {
    std::string         identifier;      ///< stable across launches; what a project stores
    std::string         name;            ///< for display
    std::size_t         inputChannels  = 0;
    std::size_t         outputChannels = 0;
    std::vector<double> sampleRates;
    bool                isDefaultInput  = false;
    bool                isDefaultOutput = false;
};

struct AudioDeviceConfig {
    /// Input and output are selected separately.
    ///
    /// Not a convenience: assuming one duplex device is wrong on any rig with a
    /// separate interface and monitor path, and it is a limitation FL Studio
    /// removed on macOS in 2026 for the same reason.
    std::string outputDeviceIdentifier;   ///< empty selects the system default
    std::string inputDeviceIdentifier;    ///< empty means no input

    double      sampleRate     = 48000.0;
    std::int64_t bufferSize    = 128;
    std::size_t outputChannels = 2;
};

/// Implemented by the engine. Called on the device's realtime thread.
class AudioIOCallback {
public:
    virtual ~AudioIOCallback() = default;

    /// Renders one block. Planar: `outputChannels[c]` is a contiguous array of
    /// `frameCount` samples.
    ///
    /// `blockHostTimeNanos` is when the first frame of this block will be heard,
    /// on the same clock as MIDI input timestamps (platform/HostTime.h). It is
    /// what lets an incoming note be placed on an exact sample rather than at
    /// the start of whichever block happened to notice it.
    ///
    /// Bound by the prime directive (docs/AUDIO_ENGINE.md §1). Everything
    /// reachable from here must be allocation-free and lock-free.
    virtual void renderAudioBlock(float* const* outputChannels,
                                  std::size_t   channelCount,
                                  std::int64_t  frameCount,
                                  std::uint64_t blockHostTimeNanos) noexcept = 0;

    /// Called off the realtime thread before the device starts, and again if the
    /// format changes. May allocate.
    virtual void audioDeviceAboutToStart(double sampleRate, std::int64_t bufferSize) { (void)sampleRate; (void)bufferSize; }
    virtual void audioDeviceStopped() {}
};

/// An audio interface.
class AudioDevice {
public:
    virtual ~AudioDevice() = default;

    /// The backend for this platform. CoreAudio on macOS.
    [[nodiscard]] static std::unique_ptr<AudioDevice> create();

    [[nodiscard]] virtual std::vector<AudioDeviceInfo> enumerateDevices() const = 0;

    /// Opens the device. On failure returns false and fills `error`.
    [[nodiscard]] virtual bool open(const AudioDeviceConfig& config, std::string& error) = 0;
    virtual void close() = 0;

    [[nodiscard]] virtual bool start(AudioIOCallback& callback, std::string& error) = 0;
    virtual void stop() = 0;

    [[nodiscard]] virtual bool isOpen()    const noexcept = 0;
    [[nodiscard]] virtual bool isRunning() const noexcept = 0;

    /// The rate and block size actually granted. A device may not honour the
    /// request, and every latency figure depends on which one is true.
    [[nodiscard]] virtual double       actualSampleRate() const noexcept = 0;
    [[nodiscard]] virtual std::int64_t actualBufferSize() const noexcept = 0;
    [[nodiscard]] virtual std::size_t  actualOutputChannels() const noexcept = 0;

    /// Latency in frames, as reported by the device.
    ///
    /// Real round-trip latency is larger than the buffer size: the safety
    /// offset and the stream's own latency both count. Guessing here is what
    /// makes recorded audio land in the wrong place — silently, and only
    /// noticed much later (docs/AUDIO_ENGINE.md §2).
    [[nodiscard]] virtual std::int64_t outputLatencyFrames() const noexcept = 0;
    [[nodiscard]] virtual std::int64_t inputLatencyFrames()  const noexcept = 0;
    [[nodiscard]] virtual std::int64_t safetyOffsetFrames()  const noexcept = 0;

    /// Total output latency: buffer + safety offset + stream latency.
    [[nodiscard]] virtual std::int64_t totalOutputLatencyFrames() const noexcept = 0;

    [[nodiscard]] virtual std::string deviceName() const = 0;
};

} // namespace incdaw::platform
