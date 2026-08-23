#include "platform/DeviceWatcher.h"

#include "platform/Platform.h"

#if INCDAW_PLATFORM_MACOS

#include <CoreAudio/AudioHardware.h>
#include <CoreMIDI/CoreMIDI.h>

#include <atomic>
#include <mutex>

namespace incdaw::platform {
namespace {

/// The three properties worth watching on the system object.
///
/// The device list is the obvious one. The two defaults matter as well: a user
/// who plugs in headphones has changed which device "System Default" means
/// without changing the list at all, and a session following the default would
/// otherwise carry on addressing the built-in speakers.
constexpr AudioObjectPropertySelector watchedSelectors[] = {
    kAudioHardwarePropertyDevices,
    kAudioHardwarePropertyDefaultOutputDevice,
    kAudioHardwarePropertyDefaultInputDevice,
};

AudioObjectPropertyAddress systemAddress(AudioObjectPropertySelector selector) noexcept
{
    return {selector, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
}

class CoreDeviceWatcher final : public DeviceWatcher {
public:
    CoreDeviceWatcher()
    {
        for (const AudioObjectPropertySelector selector : watchedSelectors) {
            const AudioObjectPropertyAddress address = systemAddress(selector);

            if (AudioObjectAddPropertyListener(kAudioObjectSystemObject, &address,
                                               &CoreDeviceWatcher::audioListener, this) == noErr)
                ++listeners_;
        }

        // A client of our own, with no ports on it. CoreMIDI delivers setup
        // notifications to clients, and the alternative — observing through
        // the client the engine opens — would stop observing at exactly the
        // moment that client is closed and reopened, which is the moment a
        // device change causes.
        (void)MIDIClientCreate(CFSTR("INCDAW Watcher"), &CoreDeviceWatcher::midiNotify, this,
                               &midiClient_);
    }

    ~CoreDeviceWatcher() override
    {
        // Cleared first: a notification in flight must not find a half-torn
        // object to call into.
        setCallback({});

        for (std::size_t index = 0; index < listeners_; ++index) {
            const AudioObjectPropertyAddress address = systemAddress(watchedSelectors[index]);
            (void)AudioObjectRemovePropertyListener(kAudioObjectSystemObject, &address,
                                                    &CoreDeviceWatcher::audioListener, this);
        }

        if (midiClient_ != 0) {
            MIDIClientDispose(midiClient_);
            midiClient_ = 0;
        }
    }

    void setCallback(std::function<void()> callback) override
    {
        const std::lock_guard<std::mutex> guard(mutex_);
        callback_ = std::move(callback);
    }

    [[nodiscard]] std::uint64_t changeCount() const override
    {
        return changes_.load(std::memory_order_relaxed);
    }

private:
    void notifyChanged()
    {
        changes_.fetch_add(1, std::memory_order_relaxed);

        // Copied out under the lock and called outside it: the callback
        // marshals to the main thread, and holding a lock across that is how a
        // teardown on the main thread deadlocks against a notification.
        std::function<void()> callback;
        {
            const std::lock_guard<std::mutex> guard(mutex_);
            callback = callback_;
        }

        if (callback)
            callback();
    }

    static OSStatus audioListener(AudioObjectID, UInt32, const AudioObjectPropertyAddress*,
                                  void* context) noexcept
    {
        if (context != nullptr)
            static_cast<CoreDeviceWatcher*>(context)->notifyChanged();

        return noErr;
    }

    static void midiNotify(const MIDINotification* message, void* context) noexcept
    {
        if (message == nullptr || context == nullptr)
            return;

        // Only the settled message. CoreMIDI also sends one per object added
        // and removed, and a hub with eight ports on it would otherwise cause
        // eight rescans of the same new state.
        if (message->messageID == kMIDIMsgSetupChanged)
            static_cast<CoreDeviceWatcher*>(context)->notifyChanged();
    }

    mutable std::mutex    mutex_;
    std::function<void()> callback_;

    std::atomic<std::uint64_t> changes_{0};

    std::size_t   listeners_  = 0;
    MIDIClientRef midiClient_ = 0;
};

} // namespace

std::unique_ptr<DeviceWatcher> DeviceWatcher::create()
{
    return std::make_unique<CoreDeviceWatcher>();
}

} // namespace incdaw::platform

#endif // INCDAW_PLATFORM_MACOS
