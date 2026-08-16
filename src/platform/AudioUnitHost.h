#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace incdaw::platform {

/// One Audio Unit as the rest of INCDAW sees it.
///
/// No CoreAudio type crosses this header, deliberately: AudioUnit hosting is
/// operating-system code and belongs in platform/, while plugins/ must keep
/// compiling with no idea which OS it is on (docs/ARCHITECTURE.md §2, enforced
/// by tools/check_layering.py).
struct AudioUnitDescription {
    /// "type:subtype:manufacturer", each a four-character code — the identity
    /// a project file stores. Not a path: an AU is found through the system's
    /// component registry, so moving the bundle changes nothing.
    std::string uid;
    std::string name;
    std::string manufacturer;
    bool        isInstrument = false;
};

struct AudioUnitParameterDescription {
    std::uint32_t id      = 0;
    std::string   name;
    double        minimum = 0.0;
    double        maximum = 1.0;
    double        defaultValue = 0.0;
};

/// Every Audio Unit the system knows about.
///
/// Enumeration reads the component registry and never instantiates anything,
/// so it does not run plugin code — unlike a CLAP scan, which must load a
/// binary and is therefore done out of process (docs/PLUGIN_HOST.md §3).
[[nodiscard]] std::vector<AudioUnitDescription> scanAudioUnits();

/// One instantiated, initialised Audio Unit.
///
/// Effects only for now: two in, two out, processed in place, which is the
/// shape the render graph's insert chain already has.
class AudioUnitHandle {
public:
    virtual ~AudioUnitHandle() = default;

    AudioUnitHandle(const AudioUnitHandle&)            = delete;
    AudioUnitHandle& operator=(const AudioUnitHandle&) = delete;

    /// Instantiates `uid` at `sampleRate`, ready for blocks up to `maxFrames`.
    /// Returns nullptr with `error` set — an AU that refuses to initialise is
    /// a slot the UI explains, never a failed compile.
    [[nodiscard]] static std::unique_ptr<AudioUnitHandle> open(const std::string& uid,
                                                               double             sampleRate,
                                                               std::uint32_t      maxFrames,
                                                               std::string&       error);

    /// Audio thread. Processes one stereo block in place. Allocation-free:
    /// the buffer lists and the input scratch are built at open time.
    [[nodiscard]] virtual bool process(float* left, float* right, std::uint32_t frames) noexcept = 0;

    /// Audio thread. AudioUnitSetParameter is the format's own realtime path.
    virtual void setParameter(std::uint32_t parameterId, double value) noexcept = 0;

    [[nodiscard]] virtual const std::vector<AudioUnitParameterDescription>&
    parameters() const noexcept = 0;

    /// Reported processing delay, in frames, read once at open time.
    [[nodiscard]] virtual std::uint32_t latencyFrames() const noexcept = 0;

    /// The unit's full state as an opaque blob (kAudioUnitProperty_ClassInfo,
    /// serialised as a binary property list). Empty on refusal.
    [[nodiscard]] virtual bool saveState(std::vector<std::byte>& out) const = 0;
    [[nodiscard]] virtual bool restoreState(const std::byte* data, std::size_t size) = 0;

    /// The generic parameter editor every AU can provide. `parentView` is an
    /// NSView*; the size the view wants comes back in `width`/`height`.
    [[nodiscard]] virtual bool hasEditor() const noexcept = 0;
    [[nodiscard]] virtual bool openEditor(void* parentView, std::uint32_t& width,
                                          std::uint32_t& height) = 0;
    virtual void closeEditor() noexcept = 0;

protected:
    AudioUnitHandle() = default;
};

} // namespace incdaw::platform
