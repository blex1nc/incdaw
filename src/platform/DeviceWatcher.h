#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace incdaw::platform {

/// Tells the application when the machine's audio or MIDI hardware changes.
///
/// Settings rescanned only when asked, which meant a keyboard plugged in while
/// INCDAW was open did nothing until the window was reopened, and an interface
/// unplugged mid-session left a device list describing hardware that was no
/// longer there. Both are cases where the answer is known — the system says so
/// — and only nobody was listening.
///
/// Deliberately not part of `AudioDevice` or `MidiDevice`. What changed is a
/// property of the machine, not of an open device: the most interesting moment
/// is exactly the one where the device INCDAW was using has just stopped
/// existing, and an observer that lives inside it would go with it. The watcher
/// therefore keeps its own minimal system handles and outlives every device.
///
/// The callback arrives on a system thread — CoreAudio's notification thread or
/// CoreMIDI's client run loop — and must do nothing but marshal. Re-enumerating
/// from inside it would call back into the very subsystem that is mid-change.
class DeviceWatcher {
public:
    virtual ~DeviceWatcher() = default;

    /// The backend for this platform, or nullptr where there is none.
    [[nodiscard]] static std::unique_ptr<DeviceWatcher> create();

    /// Installs the callback, or clears it with an empty one. Safe to call
    /// while notifications are arriving.
    virtual void setCallback(std::function<void()> callback) = 0;

    /// Notifications seen since construction. Diagnostics: a device that
    /// announces itself repeatedly is a real thing, and a counter is how it
    /// becomes visible rather than merely suspected.
    [[nodiscard]] virtual std::uint64_t changeCount() const = 0;
};

} // namespace incdaw::platform
