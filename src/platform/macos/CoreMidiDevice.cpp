#include "platform/MidiDevice.h"

#include "platform/HostTime.h"
#include "platform/Platform.h"

#if INCDAW_PLATFORM_MACOS

#include <CoreMIDI/CoreMIDI.h>

#include <atomic>
#include <cstring>

namespace incdaw::platform {
namespace {

std::string stringProperty(MIDIObjectRef object, CFStringRef property)
{
    CFStringRef text = nullptr;
    if (MIDIObjectGetStringProperty(object, property, &text) != noErr || text == nullptr)
        return {};

    const CFIndex length = CFStringGetLength(text);
    const CFIndex bytes  = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

    std::string result(static_cast<std::size_t>(bytes), '\0');
    const Boolean ok = CFStringGetCString(text, result.data(), bytes, kCFStringEncodingUTF8);
    CFRelease(text);

    if (!ok)
        return {};

    result.resize(std::strlen(result.c_str()));
    return result;
}

/// The endpoint's unique id, as text.
///
/// MIDIEndpointRef values are not stable across launches, so a project that
/// stored one would reconnect to whatever endpoint happened to take that slot.
std::string endpointIdentifier(MIDIEndpointRef endpoint)
{
    SInt32 uniqueID = 0;
    if (MIDIObjectGetIntegerProperty(endpoint, kMIDIPropertyUniqueID, &uniqueID) != noErr)
        return {};

    return std::to_string(uniqueID);
}

std::string endpointName(MIDIEndpointRef endpoint)
{
    // The display name includes the device, so "Keystation 61" rather than a
    // bare "Port 1" that means nothing with several devices attached.
    std::string name = stringProperty(endpoint, kMIDIPropertyDisplayName);
    return name.empty() ? stringProperty(endpoint, kMIDIPropertyName) : name;
}

class CoreMidiDevice final : public MidiDevice {
public:
    ~CoreMidiDevice() override { close(); }

    std::vector<MidiDeviceInfo> enumerateInputs() const override;
    std::vector<MidiDeviceInfo> enumerateOutputs() const override;

    bool open(const std::vector<std::string>& inputIdentifiers, MidiInputCallback& callback,
              std::string& error) override;
    void close() override;

    [[nodiscard]] bool isOpen() const noexcept override { return client_ != 0; }

    bool        selectOutput(const std::string& identifier, std::string& error) override;
    std::string selectedOutput() const override { return outputIdentifier_; }

    void sendMessage(const TimestampedMidiMessage& message) noexcept override;

private:
    static void readProc(const MIDIPacketList* packets, void* readContext, void* sourceContext) noexcept;
    void        handlePackets(const MIDIPacketList& packets) noexcept;

    MIDIClientRef   client_      = 0;
    MIDIPortRef     inputPort_   = 0;
    MIDIPortRef     outputPort_  = 0;

    /// Read by the sender thread, written by whoever calls selectOutput.
    /// Atomic rather than mutex-guarded so that a send in flight sees either
    /// the old endpoint or the new one, never a torn value.
    std::atomic<MIDIEndpointRef> outputEndpoint_{0};
    std::string                  outputIdentifier_;

    std::atomic<MidiInputCallback*> callback_{nullptr};
};

std::vector<MidiDeviceInfo> CoreMidiDevice::enumerateInputs() const
{
    std::vector<MidiDeviceInfo> results;
    const ItemCount count = MIDIGetNumberOfSources();

    for (ItemCount index = 0; index < count; ++index) {
        const MIDIEndpointRef endpoint = MIDIGetSource(index);
        if (endpoint == 0)
            continue;

        MidiDeviceInfo info;
        info.identifier = endpointIdentifier(endpoint);
        info.name       = endpointName(endpoint);
        info.isInput    = true;

        if (!info.identifier.empty())
            results.push_back(std::move(info));
    }

    return results;
}

std::vector<MidiDeviceInfo> CoreMidiDevice::enumerateOutputs() const
{
    std::vector<MidiDeviceInfo> results;
    const ItemCount count = MIDIGetNumberOfDestinations();

    for (ItemCount index = 0; index < count; ++index) {
        const MIDIEndpointRef endpoint = MIDIGetDestination(index);
        if (endpoint == 0)
            continue;

        MidiDeviceInfo info;
        info.identifier = endpointIdentifier(endpoint);
        info.name       = endpointName(endpoint);
        info.isInput    = false;

        if (!info.identifier.empty())
            results.push_back(std::move(info));
    }

    return results;
}

bool CoreMidiDevice::open(const std::vector<std::string>& inputIdentifiers, MidiInputCallback& callback,
                          std::string& error)
{
    close();

    CFStringRef clientName = CFSTR("INCDAW");

    if (const OSStatus status = MIDIClientCreate(clientName, nullptr, nullptr, &client_);
        status != noErr) {
        error = "MIDIClientCreate failed (" + std::to_string(status) + ")";
        client_ = 0;
        return false;
    }

    callback_.store(&callback, std::memory_order_release);

    if (const OSStatus status = MIDIInputPortCreate(client_, CFSTR("INCDAW Input"),
                                                    &CoreMidiDevice::readProc, this, &inputPort_);
        status != noErr) {
        error = "MIDIInputPortCreate failed (" + std::to_string(status) + ")";
        close();
        return false;
    }

    const ItemCount sourceCount = MIDIGetNumberOfSources();
    int             connected   = 0;

    for (ItemCount index = 0; index < sourceCount; ++index) {
        const MIDIEndpointRef endpoint = MIDIGetSource(index);
        if (endpoint == 0)
            continue;

        // An empty list means "everything": a user who plugs in a keyboard and
        // plays a note expects to hear it without configuring anything first.
        if (!inputIdentifiers.empty()) {
            const std::string identifier = endpointIdentifier(endpoint);
            bool wanted = false;

            for (const std::string& requested : inputIdentifiers)
                if (requested == identifier)
                    wanted = true;

            if (!wanted)
                continue;
        }

        if (MIDIPortConnectSource(inputPort_, endpoint, nullptr) == noErr)
            ++connected;
    }

    // Output is optional: a project with no external gear is perfectly normal,
    // and failing to open would block MIDI input for no reason. The port is
    // created here; which destination it writes to is `selectOutput`'s
    // decision, and until one is made nothing is sent.
    (void)MIDIOutputPortCreate(client_, CFSTR("INCDAW Output"), &outputPort_);

    if (connected == 0 && !inputIdentifiers.empty()) {
        error = "none of the requested MIDI inputs were found";
        close();
        return false;
    }

    return true;
}

void CoreMidiDevice::close()
{
    callback_.store(nullptr, std::memory_order_release);

    if (inputPort_ != 0) {
        MIDIPortDispose(inputPort_);
        inputPort_ = 0;
    }

    if (outputPort_ != 0) {
        MIDIPortDispose(outputPort_);
        outputPort_ = 0;
    }

    if (client_ != 0) {
        MIDIClientDispose(client_);
        client_ = 0;
    }

    outputEndpoint_.store(0, std::memory_order_release);
    outputIdentifier_.clear();
}

bool CoreMidiDevice::selectOutput(const std::string& identifier, std::string& error)
{
    if (identifier.empty()) {
        outputEndpoint_.store(0, std::memory_order_release);
        outputIdentifier_.clear();
        return true;
    }

    if (client_ == 0 || outputPort_ == 0) {
        error = "MIDI output is not open";
        return false;
    }

    const ItemCount count = MIDIGetNumberOfDestinations();

    for (ItemCount index = 0; index < count; ++index) {
        const MIDIEndpointRef endpoint = MIDIGetDestination(index);
        if (endpoint == 0 || endpointIdentifier(endpoint) != identifier)
            continue;

        outputEndpoint_.store(endpoint, std::memory_order_release);
        outputIdentifier_ = identifier;
        return true;
    }

    // A destination that has been unplugged since it was chosen is the common
    // case here, so the selection is cleared rather than left pointing at a
    // stale endpoint the system may reuse for something else.
    outputEndpoint_.store(0, std::memory_order_release);
    outputIdentifier_.clear();
    error = "MIDI output \"" + identifier + "\" was not found";
    return false;
}

void CoreMidiDevice::readProc(const MIDIPacketList* packets, void* readContext, void*) noexcept
{
    if (packets != nullptr && readContext != nullptr)
        static_cast<CoreMidiDevice*>(readContext)->handlePackets(*packets);
}

void CoreMidiDevice::handlePackets(const MIDIPacketList& packets) noexcept
{
    // Runs on CoreMIDI's own high-priority thread. Nothing here allocates or
    // blocks; the callback pushes into a lock-free queue and returns.
    MidiInputCallback* callback = callback_.load(std::memory_order_acquire);
    if (callback == nullptr)
        return;

    const MIDIPacket* packet = &packets.packet[0];

    for (UInt32 packetIndex = 0; packetIndex < packets.numPackets; ++packetIndex) {
        // A packet timestamp of 0 means "now" — some drivers send that rather
        // than a real time, and passing the zero straight through would place
        // every such event at the start of the timeline.
        const std::uint64_t nanos = packet->timeStamp != 0
                                        ? hostTimeToNanos(packet->timeStamp)
                                        : hostTimeNowNanos();

        // One packet can hold several messages back to back. Walking by status
        // byte length rather than assuming three bytes keeps program change and
        // channel pressure (two bytes) from shifting everything after them.
        UInt16 offset = 0;
        while (offset < packet->length) {
            const std::uint8_t status = packet->data[offset];

            if (status < 0x80) {   // running status is not re-sent by CoreMIDI
                ++offset;
                continue;
            }

            const int type = status & 0xF0;
            UInt16 length = 3;

            if (type == 0xC0 || type == 0xD0) {
                length = 2;
            } else if (status >= 0xF8) {
                length = 1;        // system realtime: clock, start, stop, continue
            } else if (status == 0xF0) {
                break;             // sysex: not handled in this phase
            } else if (type == 0xF0) {
                // System common. Song position pointer is three bytes and the
                // transport-sync path depends on it; quarter frame and song
                // select are two, and reading either as three swallows the
                // message behind it.
                if (status == 0xF1 || status == 0xF3)
                    length = 2;
                else if (status != 0xF2)
                    length = 1;    // tune request, end-of-sysex, undefined
            }

            if (offset + length > packet->length)
                break;

            TimestampedMidiMessage message;
            message.hostTimeNanos = nanos;
            message.status = status;
            message.data1  = length > 1 ? packet->data[offset + 1] : 0;
            message.data2  = length > 2 ? packet->data[offset + 2] : 0;

            callback->midiMessageReceived(message);
            offset = static_cast<UInt16>(offset + length);
        }

        packet = MIDIPacketNext(packet);
    }
}

void CoreMidiDevice::sendMessage(const TimestampedMidiMessage& message) noexcept
{
    const MIDIEndpointRef endpoint = outputEndpoint_.load(std::memory_order_acquire);

    if (outputPort_ == 0 || endpoint == 0)
        return;

    Byte            storage[256];
    MIDIPacketList* list   = reinterpret_cast<MIDIPacketList*>(storage);
    MIDIPacket*     packet = MIDIPacketListInit(list);

    const int  type   = message.status & 0xF0;

    // System realtime (clock, start, stop, continue) is one byte; program
    // change and channel pressure are two. Sending three regardless appends
    // whatever happened to be in the struct, which a device reads as a second
    // message.
    Byte length = 3;
    if (message.status >= 0xF8)
        length = 1;
    else if (type == 0xC0 || type == 0xD0)
        length = 2;

    const Byte data[3] = {message.status, message.data1, message.data2};

    // Nanoseconds are INCDAW's currency; CoreMIDI schedules in mach ticks. The
    // two are not the same unit on Apple silicon, so the conversion is what
    // makes a scheduled message land when it was meant to. Zero keeps its
    // meaning of "as soon as possible" and is passed through untouched.
    const MIDITimeStamp when =
        message.hostTimeNanos != 0 ? nanosToHostTime(message.hostTimeNanos) : 0;

    packet = MIDIPacketListAdd(list, sizeof(storage), packet, when, length, data);

    if (packet != nullptr)
        MIDISend(outputPort_, endpoint, list);
}

} // namespace

std::unique_ptr<MidiDevice> MidiDevice::create()
{
    return std::make_unique<CoreMidiDevice>();
}

} // namespace incdaw::platform

#endif // INCDAW_PLATFORM_MACOS
