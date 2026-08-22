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

    /// Chooses the destination `sendMessage` writes to.
    ///
    /// An empty identifier means *no output*, which is deliberately asymmetric
    /// with the input list above, where empty means *everything*. Connecting to
    /// every source is what a player expects; sending unrequested notes to
    /// whichever destination happened to enumerate first is a stuck note on
    /// someone else's synthesiser, or a feedback loop through a virtual port.
    ///
    /// Requires an open client, and a selection does not survive `close()` —
    /// the caller re-selects after reopening, the same way it re-connects
    /// inputs.
    [[nodiscard]] virtual bool selectOutput(const std::string& identifier, std::string& error) = 0;

    /// The identifier `selectOutput` last accepted; empty when nothing is
    /// selected, which is the state in which `sendMessage` does nothing.
    [[nodiscard]] virtual std::string selectedOutput() const = 0;

    /// Sends a message to the selected output, timestamped for `hostTimeNanos`
    /// (0 means "as soon as possible").
    ///
    /// NOT for the audio thread. CoreMIDI's send path takes locks and may
    /// allocate; the engine's `MidiOutput` owns a sender thread that calls this
    /// with timestamps far enough ahead that the system schedules them
    /// precisely (engine/midi/MidiOutput.h).
    virtual void sendMessage(const TimestampedMidiMessage& message) noexcept = 0;
};

} // namespace incdaw::platform
