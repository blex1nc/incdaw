#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::platform {

struct MidiDeviceInfo {
    std::string identifier;   ///< stable across launches
    std::string name;
    bool        isInput = true;
};

/// One MIDI message as it arrives from the system.
///
/// Timestamped in nanoseconds on the same clock as the audio callback
/// (platform/HostTime.h), which is what lets the engine place it on an exact
/// sample rather than at the start of whichever block happened to notice it.
struct TimestampedMidiMessage {
    std::uint64_t hostTimeNanos = 0;
    std::uint8_t  status = 0;
    std::uint8_t  data1  = 0;
    std::uint8_t  data2  = 0;
};

/// Implemented by the engine. Called on the system's MIDI thread — a high
/// priority thread that is NOT the audio thread, and must be treated with the
/// same care: no blocking, no allocation.
class MidiInputCallback {
public:
    virtual ~MidiInputCallback() = default;
    virtual void midiMessageReceived(const TimestampedMidiMessage& message) noexcept = 0;
};

class MidiDevice {
public:
    virtual ~MidiDevice() = default;

    [[nodiscard]] static std::unique_ptr<MidiDevice> create();

    [[nodiscard]] virtual std::vector<MidiDeviceInfo> enumerateInputs() const = 0;
    [[nodiscard]] virtual std::vector<MidiDeviceInfo> enumerateOutputs() const = 0;

    /// Opens the client and connects to `inputIdentifiers`. An empty list
    /// connects to every available source, which is what a user expects when
    /// they plug in a keyboard and press a key without configuring anything.
    [[nodiscard]] virtual bool open(const std::vector<std::string>& inputIdentifiers,
                                    MidiInputCallback&              callback,
                                    std::string&                    error) = 0;
    virtual void close() = 0;

    [[nodiscard]] virtual bool isOpen() const noexcept = 0;

    /// Sends a message to the opened output, timestamped for `hostTimeNanos`
    /// (0 means "as soon as possible").
    virtual void sendMessage(const TimestampedMidiMessage& message) noexcept = 0;
};

} // namespace incdaw::platform
