#include "platform/AudioDevice.h"

#include "platform/HostTime.h"
#include "platform/Platform.h"

#if INCDAW_PLATFORM_MACOS

#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>

#include <atomic>
#include <cstring>
#include <vector>

namespace incdaw::platform {
namespace {

// ── CoreAudio property helpers ────────────────────────────────────────────────
//
// The HAL property API is uniform but verbose; these keep the device code
// readable rather than a wall of AudioObjectGetPropertyData calls.

AudioObjectPropertyAddress address(AudioObjectPropertySelector selector,
                                   AudioObjectPropertyScope    scope = kAudioObjectPropertyScopeGlobal)
{
    return {selector, scope, kAudioObjectPropertyElementMain};
}

template <typename T>
bool getProperty(AudioObjectID object, const AudioObjectPropertyAddress& what, T& out)
{
    UInt32 size = sizeof(T);
    return AudioObjectGetPropertyData(object, &what, 0, nullptr, &size, &out) == noErr;
}

template <typename T>
bool setProperty(AudioObjectID object, const AudioObjectPropertyAddress& what, const T& value)
{
    return AudioObjectSetPropertyData(object, &what, 0, nullptr, sizeof(T), &value) == noErr;
}

std::vector<std::uint8_t> getPropertyBlob(AudioObjectID object, const AudioObjectPropertyAddress& what)
{
    UInt32 size = 0;
    if (AudioObjectGetPropertyDataSize(object, &what, 0, nullptr, &size) != noErr || size == 0)
        return {};

    std::vector<std::uint8_t> blob(size);
    if (AudioObjectGetPropertyData(object, &what, 0, nullptr, &size, blob.data()) != noErr)
        return {};

    blob.resize(size);
    return blob;
}

std::string getStringProperty(AudioObjectID object, AudioObjectPropertySelector selector)
{
    CFStringRef text = nullptr;
    if (!getProperty(object, address(selector), text) || text == nullptr)
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

std::size_t channelCount(AudioObjectID device, AudioObjectPropertyScope scope)
{
    const auto blob = getPropertyBlob(device, address(kAudioDevicePropertyStreamConfiguration, scope));
    if (blob.empty())
        return 0;

    const auto* list  = reinterpret_cast<const AudioBufferList*>(blob.data());
    std::size_t total = 0;

    for (UInt32 index = 0; index < list->mNumberBuffers; ++index)
        total += list->mBuffers[index].mNumberChannels;

    return total;
}

std::vector<AudioObjectID> allDeviceIDs()
{
    const auto blob = getPropertyBlob(kAudioObjectSystemObject, address(kAudioHardwarePropertyDevices));
    const std::size_t count = blob.size() / sizeof(AudioObjectID);

    std::vector<AudioObjectID> devices(count);
    if (count > 0)
        std::memcpy(devices.data(), blob.data(), count * sizeof(AudioObjectID));

    return devices;
}

AudioObjectID defaultDevice(bool forInput)
{
    AudioObjectID device = kAudioObjectUnknown;
    getProperty(kAudioObjectSystemObject,
                address(forInput ? kAudioHardwarePropertyDefaultInputDevice
                                 : kAudioHardwarePropertyDefaultOutputDevice),
                device);
    return device;
}

/// A device's stable identifier. The HAL's AudioObjectID is reassigned across
/// reboots and replugs, so a project that stored one would silently open the
/// wrong interface; the UID is what survives.
std::string deviceUID(AudioObjectID device)
{
    return getStringProperty(device, kAudioDevicePropertyDeviceUID);
}

AudioObjectID findDeviceByUID(const std::string& uid)
{
    if (uid.empty())
        return kAudioObjectUnknown;

    for (const AudioObjectID device : allDeviceIDs())
        if (deviceUID(device) == uid)
            return device;

    return kAudioObjectUnknown;
}

std::int64_t latencyFrames(AudioObjectID device, AudioObjectPropertyScope scope)
{
    UInt32 latency = 0;
    getProperty(device, address(kAudioDevicePropertyLatency, scope), latency);

    // The stream carries its own latency on top of the device's.
    UInt32 streamLatency = 0;
    const auto blob = getPropertyBlob(device, address(kAudioDevicePropertyStreams, scope));

    if (blob.size() >= sizeof(AudioStreamID)) {
        AudioStreamID stream = 0;
        std::memcpy(&stream, blob.data(), sizeof(stream));
        getProperty(stream, address(kAudioStreamPropertyLatency, scope), streamLatency);
    }

    return static_cast<std::int64_t>(latency) + static_cast<std::int64_t>(streamLatency);
}

// ── The device ────────────────────────────────────────────────────────────────

class CoreAudioDevice final : public AudioDevice {
public:
    ~CoreAudioDevice() override { close(); }

    std::vector<AudioDeviceInfo> enumerateDevices() const override;

    bool open(const AudioDeviceConfig& config, std::string& error) override;
    void close() override;

    bool start(AudioIOCallback& callback, std::string& error) override;
    void stop() override;

    [[nodiscard]] bool isOpen()    const noexcept override { return deviceID_ != kAudioObjectUnknown; }
    [[nodiscard]] bool isRunning() const noexcept override { return running_.load(std::memory_order_acquire); }

    [[nodiscard]] double       actualSampleRate()     const noexcept override { return sampleRate_; }
    [[nodiscard]] std::int64_t actualBufferSize()     const noexcept override { return bufferSize_; }
    [[nodiscard]] std::int64_t maxServiceableBlockSize() const noexcept override { return scratchCapacity_; }
    [[nodiscard]] std::size_t  actualOutputChannels() const noexcept override { return outputChannels_; }

    [[nodiscard]] std::int64_t outputLatencyFrames() const noexcept override { return outputLatency_; }
    [[nodiscard]] std::int64_t inputLatencyFrames()  const noexcept override { return inputLatency_; }
    [[nodiscard]] std::int64_t safetyOffsetFrames()  const noexcept override { return safetyOffset_; }

    [[nodiscard]] std::int64_t totalOutputLatencyFrames() const noexcept override
    {
        return bufferSize_ + safetyOffset_ + outputLatency_;
    }

    [[nodiscard]] std::string deviceName() const override { return name_; }

private:
    static OSStatus ioProcTrampoline(AudioObjectID            device,
                                     const AudioTimeStamp*    now,
                                     const AudioBufferList*   inputData,
                                     const AudioTimeStamp*    inputTime,
                                     AudioBufferList*         outputData,
                                     const AudioTimeStamp*    outputTime,
                                     void*                    clientData) noexcept;

    void renderInto(AudioBufferList& outputData, std::uint64_t hostTimeNanos) noexcept;

    AudioObjectID    deviceID_  = kAudioObjectUnknown;
    AudioDeviceIOProcID procID_ = nullptr;
    AudioIOCallback* callback_  = nullptr;
    std::atomic<bool> running_{false};

    std::string  name_;
    double       sampleRate_     = 0.0;
    std::int64_t bufferSize_     = 0;
    std::size_t  outputChannels_ = 0;
    std::int64_t outputLatency_  = 0;
    std::int64_t inputLatency_   = 0;
    std::int64_t safetyOffset_   = 0;

    /// Planar scratch the engine renders into, allocated in `open`.
    ///
    /// CoreAudio hands out interleaved buffers on most devices while the engine
    /// is planar throughout (docs/AUDIO_ENGINE.md — planar is what vectorises).
    /// Converting here keeps that conversion in exactly one place, at the very
    /// edge of the system.
    std::vector<float>  scratch_;
    std::vector<float*> scratchChannels_;

    /// Frames the scratch can hold. Sized from the device's maximum, not its
    /// current setting: a shared device delivers whatever block size CoreAudio
    /// is servicing, which is not always what our property query reported.
    std::int64_t        scratchCapacity_ = 0;

    /// The shared device's buffer size before we changed it; restored in close.
    UInt32              bufferSizeToRestore_ = 0;
};

std::vector<AudioDeviceInfo> CoreAudioDevice::enumerateDevices() const
{
    std::vector<AudioDeviceInfo> results;

    const AudioObjectID defaultOut = defaultDevice(false);
    const AudioObjectID defaultIn  = defaultDevice(true);

    for (const AudioObjectID device : allDeviceIDs()) {
        AudioDeviceInfo info;
        info.identifier      = deviceUID(device);
        info.name            = getStringProperty(device, kAudioObjectPropertyName);
        info.inputChannels   = channelCount(device, kAudioObjectPropertyScopeInput);
        info.outputChannels  = channelCount(device, kAudioObjectPropertyScopeOutput);
        info.isDefaultOutput = (device == defaultOut);
        info.isDefaultInput  = (device == defaultIn);

        if (info.identifier.empty())
            continue;

        const auto blob = getPropertyBlob(device, address(kAudioDevicePropertyAvailableNominalSampleRates));
        const std::size_t rangeCount = blob.size() / sizeof(AudioValueRange);

        for (std::size_t index = 0; index < rangeCount; ++index) {
            AudioValueRange range{};
            std::memcpy(&range, blob.data() + index * sizeof(AudioValueRange), sizeof(range));

            // Continuous-rate devices report a span; the discrete rates inside
            // it are what a user can actually pick.
            if (range.mMinimum == range.mMaximum) {
                info.sampleRates.push_back(range.mMinimum);
            } else {
                for (const double rate : {44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0})
                    if (rate >= range.mMinimum && rate <= range.mMaximum)
                        info.sampleRates.push_back(rate);
            }
        }

        results.push_back(std::move(info));
    }

    return results;
}

bool CoreAudioDevice::open(const AudioDeviceConfig& config, std::string& error)
{
    close();

    deviceID_ = config.outputDeviceIdentifier.empty()
                    ? defaultDevice(false)
                    : findDeviceByUID(config.outputDeviceIdentifier);

    if (deviceID_ == kAudioObjectUnknown) {
        error = config.outputDeviceIdentifier.empty()
                    ? "no default output device"
                    : "output device not found: " + config.outputDeviceIdentifier;
        return false;
    }

    name_ = getStringProperty(deviceID_, kAudioObjectPropertyName);

    const std::size_t available = channelCount(deviceID_, kAudioObjectPropertyScopeOutput);
    if (available == 0) {
        error = "device '" + name_ + "' has no output channels";
        deviceID_ = kAudioObjectUnknown;
        return false;
    }

    outputChannels_ = config.outputChannels < available ? config.outputChannels : available;

    // Requesting a rate the device is already at makes it reject the change, so
    // the current value is read first.
    Float64 currentRate = 0.0;
    getProperty(deviceID_, address(kAudioDevicePropertyNominalSampleRate), currentRate);

    if (config.sampleRate > 0.0 && currentRate != config.sampleRate) {
        const auto requested = static_cast<Float64>(config.sampleRate);
        setProperty(deviceID_, address(kAudioDevicePropertyNominalSampleRate), requested);
    }

    if (config.bufferSize > 0) {
        // The buffer size is a property of the SHARED device, not of this
        // process: forcing it changes it for every application using the
        // device, and a Bluetooth output driven at a size it cannot sustain
        // crackles system-wide. The previous value is recorded here and put
        // back in close(), so quitting INCDAW leaves the machine as it was.
        UInt32 before = 0;
        if (getProperty(deviceID_, address(kAudioDevicePropertyBufferFrameSize,
                                           kAudioObjectPropertyScopeOutput), before))
            bufferSizeToRestore_ = before;

        const auto requested = static_cast<UInt32>(config.bufferSize);
        setProperty(deviceID_, address(kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeOutput),
                    requested);
    }

    // Read back what was actually granted. A device is free to ignore either
    // request, and every latency figure below depends on the real values.
    Float64 grantedRate = 0.0;
    getProperty(deviceID_, address(kAudioDevicePropertyNominalSampleRate), grantedRate);
    sampleRate_ = grantedRate > 0.0 ? grantedRate : config.sampleRate;

    UInt32 grantedBuffer = 0;
    getProperty(deviceID_, address(kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeOutput),
                grantedBuffer);
    bufferSize_ = grantedBuffer > 0 ? static_cast<std::int64_t>(grantedBuffer) : config.bufferSize;

    UInt32 safety = 0;
    getProperty(deviceID_, address(kAudioDevicePropertySafetyOffset, kAudioObjectPropertyScopeOutput), safety);
    safetyOffset_ = static_cast<std::int64_t>(safety);

    outputLatency_ = latencyFrames(deviceID_, kAudioObjectPropertyScopeOutput);

    if (!config.inputDeviceIdentifier.empty()) {
        const AudioObjectID input = findDeviceByUID(config.inputDeviceIdentifier);
        if (input != kAudioObjectUnknown)
            inputLatency_ = latencyFrames(input, kAudioObjectPropertyScopeInput);
    }

    // Planar scratch, allocated once here so the callback never has to.
    //
    // Sized from the device's maximum supported block, because the IOProc is
    // not obliged to hand us the size we asked for — when another process has
    // the device open, CoreAudio services whichever block size it is already
    // running. Allocating only the nominal size would mean either a truncated
    // block or, worse, silence.
    AudioValueRange sizeRange{};
    scratchCapacity_ = bufferSize_;

    if (getProperty(deviceID_, address(kAudioDevicePropertyBufferFrameSizeRange,
                                       kAudioObjectPropertyScopeOutput), sizeRange)
        && sizeRange.mMaximum > static_cast<Float64>(scratchCapacity_))
        scratchCapacity_ = static_cast<std::int64_t>(sizeRange.mMaximum);

    // A floor, in case the range query fails on some driver.
    if (scratchCapacity_ < bufferSize_ * 4)
        scratchCapacity_ = bufferSize_ * 4;

    const auto capacity = static_cast<std::size_t>(scratchCapacity_);

    scratch_.assign(outputChannels_ * capacity, 0.0f);
    scratchChannels_.resize(outputChannels_);

    for (std::size_t channel = 0; channel < outputChannels_; ++channel)
        scratchChannels_[channel] = scratch_.data() + channel * capacity;

    return true;
}

void CoreAudioDevice::close()
{
    stop();

    if (procID_ != nullptr && deviceID_ != kAudioObjectUnknown) {
        AudioDeviceDestroyIOProcID(deviceID_, procID_);
        procID_ = nullptr;
    }

    // Put the shared device's buffer size back the way it was found. Leaving
    // our value behind is how "the DAW broke my computer's sound" happens.
    if (bufferSizeToRestore_ != 0 && deviceID_ != kAudioObjectUnknown
        && bufferSizeToRestore_ != static_cast<UInt32>(bufferSize_)) {
        setProperty(deviceID_, address(kAudioDevicePropertyBufferFrameSize, kAudioObjectPropertyScopeOutput),
                    bufferSizeToRestore_);
    }

    bufferSizeToRestore_ = 0;
    deviceID_ = kAudioObjectUnknown;
    callback_ = nullptr;
    scratch_.clear();
    scratchChannels_.clear();
    scratchCapacity_ = 0;
}

bool CoreAudioDevice::start(AudioIOCallback& callback, std::string& error)
{
    if (!isOpen()) {
        error = "device is not open";
        return false;
    }

    if (isRunning())
        return true;

    callback_ = &callback;
    callback_->audioDeviceAboutToStart(sampleRate_, bufferSize_);

    if (procID_ == nullptr) {
        const OSStatus status = AudioDeviceCreateIOProcID(deviceID_, &CoreAudioDevice::ioProcTrampoline,
                                                          this, &procID_);
        if (status != noErr || procID_ == nullptr) {
            error = "AudioDeviceCreateIOProcID failed (" + std::to_string(status) + ")";
            callback_ = nullptr;
            return false;
        }
    }

    const OSStatus status = AudioDeviceStart(deviceID_, procID_);
    if (status != noErr) {
        error = "AudioDeviceStart failed (" + std::to_string(status) + ")";
        callback_ = nullptr;
        return false;
    }

    running_.store(true, std::memory_order_release);
    return true;
}

void CoreAudioDevice::stop()
{
    if (!running_.exchange(false, std::memory_order_acq_rel))
        return;

    if (deviceID_ != kAudioObjectUnknown && procID_ != nullptr)
        AudioDeviceStop(deviceID_, procID_);

    if (callback_ != nullptr)
        callback_->audioDeviceStopped();
}

OSStatus CoreAudioDevice::ioProcTrampoline(AudioObjectID, const AudioTimeStamp* now,
                                           const AudioBufferList*, const AudioTimeStamp*,
                                           AudioBufferList* outputData, const AudioTimeStamp* outputTime,
                                           void* clientData) noexcept
{
    if (outputData == nullptr || clientData == nullptr)
        return noErr;

    // The OUTPUT timestamp, not `now`: it says when this block will actually be
    // heard, which is what an incoming MIDI note has to be aligned against.
    // Using `now` would bake the output latency into every recorded position.
    std::uint64_t hostTime = 0;

    if (outputTime != nullptr && (outputTime->mFlags & kAudioTimeStampHostTimeValid) != 0)
        hostTime = outputTime->mHostTime;
    else if (now != nullptr && (now->mFlags & kAudioTimeStampHostTimeValid) != 0)
        hostTime = now->mHostTime;

    static_cast<CoreAudioDevice*>(clientData)->renderInto(
        *outputData, hostTime != 0 ? hostTimeToNanos(hostTime) : hostTimeNowNanos());

    return noErr;
}

void CoreAudioDevice::renderInto(AudioBufferList& outputData, std::uint64_t hostTimeNanos) noexcept
{
    // This runs on CoreAudio's realtime thread, which the HAL has already joined
    // to the device's os_workgroup. Any additional worker threads INCDAW spawns
    // must join it explicitly (docs/DECISIONS.md D-004); there are none yet.

    if (!running_.load(std::memory_order_acquire) || callback_ == nullptr || outputData.mNumberBuffers == 0)
        return;

    const bool interleaved = outputData.mNumberBuffers == 1 && outputData.mBuffers[0].mNumberChannels > 1;

    const auto deviceChannels = static_cast<std::size_t>(
        interleaved ? outputData.mBuffers[0].mNumberChannels : outputData.mNumberBuffers);

    auto frames = static_cast<std::int64_t>(
        outputData.mBuffers[0].mDataByteSize
        / (sizeof(float) * (interleaved ? outputData.mBuffers[0].mNumberChannels : 1)));

    if (frames <= 0)
        return;

    // Clamp rather than bail out. Returning here would emit silence for the
    // whole block, turning an unexpected block size into an audible dropout;
    // rendering what we have room for keeps audio flowing.
    if (frames > scratchCapacity_)
        frames = scratchCapacity_;

    callback_->renderAudioBlock(scratchChannels_.data(), outputChannels_, frames, hostTimeNanos);

    if (interleaved) {
        auto* destination = static_cast<float*>(outputData.mBuffers[0].mData);
        if (destination == nullptr)
            return;

        for (std::size_t channel = 0; channel < deviceChannels; ++channel) {
            // Devices with more outputs than we render get silence on the rest,
            // rather than a repeat of channel 0.
            const float* source = channel < outputChannels_ ? scratchChannels_[channel] : nullptr;

            for (std::int64_t frame = 0; frame < frames; ++frame)
                destination[static_cast<std::size_t>(frame) * deviceChannels + channel]
                    = source != nullptr ? source[frame] : 0.0f;
        }
    } else {
        for (UInt32 buffer = 0; buffer < outputData.mNumberBuffers; ++buffer) {
            auto* destination = static_cast<float*>(outputData.mBuffers[buffer].mData);
            if (destination == nullptr)
                continue;

            if (buffer < outputChannels_)
                std::memcpy(destination, scratchChannels_[buffer],
                            static_cast<std::size_t>(frames) * sizeof(float));
            else
                std::memset(destination, 0, static_cast<std::size_t>(frames) * sizeof(float));
        }
    }
}

} // namespace

std::unique_ptr<AudioDevice> AudioDevice::create()
{
    return std::make_unique<CoreAudioDevice>();
}

} // namespace incdaw::platform

#endif // INCDAW_PLATFORM_MACOS
