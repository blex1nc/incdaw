#pragma once

#include "engine/audio/WavFile.h"
#include "project/ProjectGraphCompiler.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace incdaw::project {

/// Offline rendering (docs/ROADMAP.md Phase 17).
///
/// There is no separate offline DSP: the renderer compiles the project with
/// the SAME compiler the audio engine uses and drives the compiled graph
/// block by block, exactly as the audio callback does — which is what makes
/// the exit criterion (offline byte-identical to a realtime capture)
/// possible at all. Assets are always preloaded, never streamed: an offline
/// render must be deterministic, and a disk window's timing is not.
struct RenderOptions {
    engine::SampleRate sampleRate = 48000.0;
    engine::FrameCount blockSize  = 512;

    /// Rendered after the arrangement ends, so reverb and release tails are
    /// not cut off mid-air.
    double tailSeconds = 2.0;

    /// Scales the result so its peak sits at exactly 1.0. Applied before
    /// quantisation, which is the only order that makes dither meaningful.
    bool normalize = false;

    enum class BitDepth { pcm16, pcm24, float32 };
    BitDepth bitDepth = BitDepth::float32;

    /// TPDF dither at one LSB, applied when quantising to pcm16. Determinism
    /// comes from `randomSeed` — the same render is the same file, always.
    bool dither = true;

    /// 0 renders at `sampleRate`. Anything else converts the finished render
    /// with the windowed-sinc resampler (engine/dsp/Resampler.h).
    engine::SampleRate targetSampleRate = 0.0;

    /// A timeline region in frames; length 0 means the whole arrangement.
    engine::FramePosition regionStart  = 0;
    engine::FrameCount    regionLength = 0;

    /// A stem: solo this mixer node before compiling. Invalid means the
    /// master mix.
    EntityId stemMixerNode{};

    /// An individual track: solo this channel before compiling. Invalid
    /// means all channels.
    EntityId soloChannel{};

    std::uint64_t randomSeed = 0;

    engine::SampleCache*     sampleCache = nullptr;
    const ParameterRegistry* parameters  = nullptr;
};

struct RenderResult {
    bool        succeeded = false;
    std::string error;

    /// The rendered audio, at the render rate (or `targetSampleRate` after
    /// conversion), post normalize/dither.
    engine::AudioFileData audio;

    /// Frames the arrangement itself occupied, before the tail.
    engine::FrameCount arrangementFrames = 0;

    std::vector<std::string> warnings;   ///< the compile's warnings, passed through

    [[nodiscard]] explicit operator bool() const noexcept { return succeeded; }
};

/// Renders the project's arrangement into memory.
[[nodiscard]] RenderResult renderProject(const Project& project,
                                         const engine::TempoMap& tempoMap,
                                         const RenderOptions& options);

/// Renders and writes in one step. The container comes from the extension:
/// .wav or .aiff (.aif). BitDepth::float32 in an AIFF is written as pcm24 —
/// AIFF-C float is a compatibility trap, and 24-bit is the format's honest
/// ceiling.
[[nodiscard]] RenderResult renderProjectToFile(const Project& project,
                                               const engine::TempoMap& tempoMap,
                                               const RenderOptions& options,
                                               const std::filesystem::path& path);

/// The end of the last clip in the arrangement, in frames at `sampleRate`.
[[nodiscard]] engine::FrameCount arrangementEndFrames(const Project& project,
                                                      const engine::TempoMap& tempoMap);

} // namespace incdaw::project
