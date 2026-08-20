#include "platform/AudioUnitHost.h"

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import <Cocoa/Cocoa.h>

#include <array>
#include <cstring>

namespace incdaw::platform {

namespace {

constexpr std::size_t stereo = 2;

std::string fourCharCode(std::uint32_t code)
{
    const char characters[5] = {static_cast<char>((code >> 24) & 0xFFu),
                                static_cast<char>((code >> 16) & 0xFFu),
                                static_cast<char>((code >> 8) & 0xFFu),
                                static_cast<char>(code & 0xFFu), '\0'};
    return std::string{characters};
}

bool parseFourCharCode(const std::string& text, std::uint32_t& out)
{
    if (text.size() != 4)
        return false;

    out = (static_cast<std::uint32_t>(static_cast<unsigned char>(text[0])) << 24)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(text[1])) << 16)
        | (static_cast<std::uint32_t>(static_cast<unsigned char>(text[2])) << 8)
        | static_cast<std::uint32_t>(static_cast<unsigned char>(text[3]));

    return true;
}

/// "type:subtype:manufacturer" -> the description the component registry wants.
bool describeFromUid(const std::string& uid, AudioComponentDescription& out)
{
    const std::size_t first  = uid.find(':');
    const std::size_t second = first == std::string::npos ? std::string::npos : uid.find(':', first + 1);

    if (first == std::string::npos || second == std::string::npos)
        return false;

    std::uint32_t type = 0;
    std::uint32_t subtype = 0;
    std::uint32_t manufacturer = 0;

    if (!parseFourCharCode(uid.substr(0, first), type)
        || !parseFourCharCode(uid.substr(first + 1, second - first - 1), subtype)
        || !parseFourCharCode(uid.substr(second + 1), manufacturer))
        return false;

    out                    = AudioComponentDescription{};
    out.componentType      = type;
    out.componentSubType   = subtype;
    out.componentManufacturer = manufacturer;

    return true;
}

std::string stringFrom(CFStringRef text)
{
    if (text == nullptr)
        return {};

    const CFIndex length = CFStringGetLength(text);
    const CFIndex bytes  = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

    std::string out(static_cast<std::size_t>(bytes), '\0');

    if (!CFStringGetCString(text, out.data(), bytes, kCFStringEncodingUTF8))
        return {};

    out.resize(std::strlen(out.c_str()));
    return out;
}

/// The macOS implementation. Everything CoreAudio lives behind this class.
class MacAudioUnitHandle final : public AudioUnitHandle {
public:
    ~MacAudioUnitHandle() override
    {
        closeEditor();

        if (unit_ != nullptr) {
            AudioUnitUninitialize(unit_);
            AudioComponentInstanceDispose(unit_);
        }
    }

    [[nodiscard]] bool openWith(const AudioComponentDescription& description, double sampleRate,
                                std::uint32_t maxFrames, std::string& error)
    {
        AudioComponent component = AudioComponentFindNext(nullptr, &description);

        if (component == nullptr) {
            error = "no such Audio Unit on this system";
            return false;
        }

        if (AudioComponentInstanceNew(component, &unit_) != noErr || unit_ == nullptr) {
            error = "the Audio Unit could not be instantiated";
            return false;
        }

        AudioStreamBasicDescription format{};
        format.mSampleRate       = sampleRate;
        format.mFormatID         = kAudioFormatLinearPCM;
        // Non-interleaved float is the graph's own layout, so no conversion
        // sits between INCDAW's buffers and the plugin's.
        format.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked
                                 | kAudioFormatFlagIsNonInterleaved;
        format.mChannelsPerFrame = static_cast<UInt32>(stereo);
        format.mFramesPerPacket  = 1;
        format.mBitsPerChannel   = 32;
        format.mBytesPerFrame    = sizeof(float);
        format.mBytesPerPacket   = sizeof(float);

        if (AudioUnitSetProperty(unit_, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0,
                                 &format, sizeof(format)) != noErr
            || AudioUnitSetProperty(unit_, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0,
                                    &format, sizeof(format)) != noErr) {
            error = "the Audio Unit refused stereo float at this sample rate";
            return false;
        }

        UInt32 slice = maxFrames;
        AudioUnitSetProperty(unit_, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global,
                             0, &slice, sizeof(slice));

        AURenderCallbackStruct callback{};
        callback.inputProc       = &MacAudioUnitHandle::supplyInput;
        callback.inputProcRefCon = this;

        if (AudioUnitSetProperty(unit_, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input,
                                 0, &callback, sizeof(callback)) != noErr) {
            error = "the Audio Unit refused an input callback";
            return false;
        }

        if (AudioUnitInitialize(unit_) != noErr) {
            error = "the Audio Unit failed to initialise";
            return false;
        }

        // Everything the render path needs, allocated once: the audio thread
        // may not allocate, and AudioUnitRender wants a buffer list per call.
        scratch_[0].assign(static_cast<std::size_t>(maxFrames), 0.0F);
        scratch_[1].assign(static_cast<std::size_t>(maxFrames), 0.0F);
        maxFrames_ = maxFrames;

        renderStorage_.list.mNumberBuffers = stereo;

        readParameters();
        readLatency(sampleRate);

        return true;
    }

    [[nodiscard]] bool process(float* left, float* right, std::uint32_t frames) noexcept override
    {
        if (unit_ == nullptr || frames == 0 || frames > maxFrames_)
            return false;

        // The input is copied aside first: an AU that reads its input after
        // writing its output would otherwise read what it just wrote, because
        // INCDAW processes in place.
        std::memcpy(scratch_[0].data(), left, static_cast<std::size_t>(frames) * sizeof(float));
        std::memcpy(scratch_[1].data(), right, static_cast<std::size_t>(frames) * sizeof(float));

        pending_[0] = scratch_[0].data();
        pending_[1] = scratch_[1].data();

        const auto bytes = static_cast<UInt32>(frames * sizeof(float));

        renderStorage_.list.mNumberBuffers            = stereo;
        renderStorage_.list.mBuffers[0].mNumberChannels = 1;
        renderStorage_.list.mBuffers[0].mDataByteSize   = bytes;
        renderStorage_.list.mBuffers[0].mData           = left;
        renderStorage_.second.mNumberChannels           = 1;
        renderStorage_.second.mDataByteSize             = bytes;
        renderStorage_.second.mData                     = right;

        AudioUnitRenderActionFlags flags = 0;

        AudioTimeStamp timestamp{};
        timestamp.mFlags      = kAudioTimeStampSampleTimeValid;
        timestamp.mSampleTime = sampleTime_;

        const OSStatus status =
            AudioUnitRender(unit_, &flags, &timestamp, 0, frames, &renderStorage_.list);

        sampleTime_ += static_cast<Float64>(frames);

        return status == noErr;
    }

    void setParameter(std::uint32_t parameterId, double value) noexcept override
    {
        if (unit_ != nullptr)
            AudioUnitSetParameter(unit_, parameterId, kAudioUnitScope_Global, 0,
                                  static_cast<AudioUnitParameterValue>(value), 0);
    }

    [[nodiscard]] const std::vector<AudioUnitParameterDescription>& parameters() const noexcept override
    {
        return parameters_;
    }

    [[nodiscard]] std::uint32_t latencyFrames() const noexcept override { return latency_; }

    [[nodiscard]] bool saveState(std::vector<std::byte>& out) const override
    {
        out.clear();

        if (unit_ == nullptr)
            return false;

        CFPropertyListRef classInfo = nullptr;
        UInt32            size      = sizeof(classInfo);

        if (AudioUnitGetProperty(unit_, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0,
                                 &classInfo, &size) != noErr
            || classInfo == nullptr)
            return false;

        CFDataRef data = CFPropertyListCreateData(kCFAllocatorDefault, classInfo,
                                                  kCFPropertyListBinaryFormat_v1_0, 0, nullptr);
        CFRelease(classInfo);

        if (data == nullptr)
            return false;

        const auto* bytes  = static_cast<const std::byte*>(static_cast<const void*>(CFDataGetBytePtr(data)));
        const auto  length = static_cast<std::size_t>(CFDataGetLength(data));

        out.assign(bytes, bytes + length);
        CFRelease(data);

        return true;
    }

    [[nodiscard]] bool restoreState(const std::byte* data, std::size_t size) override
    {
        if (unit_ == nullptr || data == nullptr || size == 0)
            return false;

        CFDataRef blob = CFDataCreate(kCFAllocatorDefault,
                                      static_cast<const UInt8*>(static_cast<const void*>(data)),
                                      static_cast<CFIndex>(size));
        if (blob == nullptr)
            return false;

        CFPropertyListRef classInfo =
            CFPropertyListCreateWithData(kCFAllocatorDefault, blob, kCFPropertyListImmutable,
                                         nullptr, nullptr);
        CFRelease(blob);

        if (classInfo == nullptr)
            return false;

        const OSStatus status = AudioUnitSetProperty(unit_, kAudioUnitProperty_ClassInfo,
                                                     kAudioUnitScope_Global, 0, &classInfo,
                                                     sizeof(classInfo));
        CFRelease(classInfo);

        return status == noErr;
    }

    [[nodiscard]] bool hasEditor() const noexcept override { return unit_ != nullptr; }

    [[nodiscard]] bool openEditor(void* parentView, std::uint32_t& width,
                                  std::uint32_t& height) override
    {
        if (unit_ == nullptr || parentView == nullptr)
            return false;

        closeEditor();

        // The format's own generic editor: every parameter the unit publishes,
        // laid out by the system. A custom Cocoa view (kAudioUnitProperty_
        // CocoaUI) is the next step, and this is what makes the format usable
        // before it exists.
        NSView* parent = (__bridge NSView*)parentView;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        AUGenericView* view = [[AUGenericView alloc] initWithAudioUnit:unit_];
#pragma clang diagnostic pop

        if (view == nil)
            return false;

        view.frame = NSMakeRect(0, 0, 420, 320);
        [parent addSubview:view];

        editor_ = view;
        width   = static_cast<std::uint32_t>(view.frame.size.width);
        height  = static_cast<std::uint32_t>(view.frame.size.height);

        return true;
    }

    void closeEditor() noexcept override
    {
        if (editor_ != nil) {
            [editor_ removeFromSuperview];
            editor_ = nil;
        }
    }

private:
    /// The AU pulls its input through this. It hands back the buffers the
    /// caller copied aside in process(), so nothing is allocated or converted.
    static OSStatus supplyInput(void* refCon, AudioUnitRenderActionFlags*, const AudioTimeStamp*,
                                UInt32, UInt32 frames, AudioBufferList* data) noexcept
    {
        auto* self = static_cast<MacAudioUnitHandle*>(refCon);

        if (self == nullptr || data == nullptr)
            return kAudioUnitErr_InvalidParameter;

        for (UInt32 index = 0; index < data->mNumberBuffers; ++index) {
            const std::size_t channel = index < stereo ? index : stereo - 1;

            data->mBuffers[index].mNumberChannels = 1;
            data->mBuffers[index].mDataByteSize   = static_cast<UInt32>(frames * sizeof(float));
            data->mBuffers[index].mData           = self->pending_[channel];
        }

        return noErr;
    }

    void readParameters()
    {
        UInt32 size = 0;

        if (AudioUnitGetPropertyInfo(unit_, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global,
                                     0, &size, nullptr) != noErr
            || size == 0)
            return;

        std::vector<AudioUnitParameterID> ids(size / sizeof(AudioUnitParameterID));

        if (AudioUnitGetProperty(unit_, kAudioUnitProperty_ParameterList, kAudioUnitScope_Global, 0,
                                 ids.data(), &size) != noErr)
            return;

        for (const AudioUnitParameterID id : ids) {
            AudioUnitParameterInfo info{};
            UInt32                 infoSize = sizeof(info);

            // Parameter metadata is a property keyed by the parameter id in
            // the element slot, not a call of its own.
            if (AudioUnitGetProperty(unit_, kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global,
                                     id, &info, &infoSize) != noErr)
                continue;

            AudioUnitParameterDescription parameter;
            parameter.id           = id;
            parameter.minimum      = static_cast<double>(info.minValue);
            parameter.maximum      = static_cast<double>(info.maxValue);
            parameter.defaultValue = static_cast<double>(info.defaultValue);

            if ((info.flags & kAudioUnitParameterFlag_HasCFNameString) != 0 && info.cfNameString != nullptr) {
                parameter.name = stringFrom(info.cfNameString);

                if ((info.flags & kAudioUnitParameterFlag_CFNameRelease) != 0)
                    CFRelease(info.cfNameString);
            } else {
                parameter.name = std::string{info.name};
            }

            parameters_.push_back(std::move(parameter));
        }
    }

    void readLatency(double sampleRate)
    {
        Float64 seconds = 0.0;
        UInt32  size    = sizeof(seconds);

        if (AudioUnitGetProperty(unit_, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0,
                                 &seconds, &size) == noErr
            && seconds > 0.0 && sampleRate > 0.0)
            latency_ = static_cast<std::uint32_t>(seconds * sampleRate + 0.5);
    }

    AudioUnit     unit_      = nullptr;
    std::uint32_t maxFrames_ = 0;
    std::uint32_t latency_   = 0;
    Float64       sampleTime_ = 0.0;

    std::array<std::vector<float>, stereo> scratch_;
    std::array<float*, stereo>             pending_{nullptr, nullptr};

    // AudioBufferList carries one AudioBuffer inline; a stereo list needs a
    // second one directly behind it, which is what this layout provides.
    struct StereoBufferList {
        AudioBufferList list;
        AudioBuffer     second;
    };

    StereoBufferList renderStorage_{};

    std::vector<AudioUnitParameterDescription> parameters_;

    AUGenericView* editor_ = nil;
};

} // namespace

std::vector<AudioUnitDescription> scanAudioUnits()
{
    std::vector<AudioUnitDescription> found;

    const std::array<OSType, 3> types{kAudioUnitType_Effect, kAudioUnitType_MusicEffect,
                                      kAudioUnitType_MusicDevice};

    for (const OSType type : types) {
        AudioComponentDescription wanted{};
        wanted.componentType = type;

        AudioComponent component = nullptr;

        while ((component = AudioComponentFindNext(component, &wanted)) != nullptr) {
            AudioComponentDescription description{};

            if (AudioComponentGetDescription(component, &description) != noErr)
                continue;

            CFStringRef name = nullptr;

            if (AudioComponentCopyName(component, &name) != noErr || name == nullptr)
                continue;

            const std::string full = stringFrom(name);
            CFRelease(name);

            AudioUnitDescription entry;
            entry.uid = fourCharCode(description.componentType) + ":"
                      + fourCharCode(description.componentSubType) + ":"
                      + fourCharCode(description.componentManufacturer);

            // AudioComponentCopyName gives "Manufacturer: Name".
            if (const std::size_t separator = full.find(": "); separator != std::string::npos) {
                entry.manufacturer = full.substr(0, separator);
                entry.name         = full.substr(separator + 2);
            } else {
                entry.name = full;
            }

            entry.isInstrument = description.componentType == kAudioUnitType_MusicDevice;
            found.push_back(std::move(entry));
        }
    }

    return found;
}

std::unique_ptr<AudioUnitHandle> AudioUnitHandle::open(const std::string& uid, double sampleRate,
                                                       std::uint32_t maxFrames, std::string& error)
{
    error.clear();

    AudioComponentDescription description{};

    if (!describeFromUid(uid, description)) {
        error = "not an Audio Unit identifier: " + uid;
        return nullptr;
    }

    auto handle = std::make_unique<MacAudioUnitHandle>();

    if (!handle->openWith(description, sampleRate, maxFrames, error))
        return nullptr;

    return handle;
}

} // namespace incdaw::platform
