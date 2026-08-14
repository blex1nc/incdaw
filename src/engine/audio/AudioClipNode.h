#pragma once

#include "engine/audio/AudioStream.h"
#include "engine/audio/WavFile.h"
#include "engine/graph/Node.h"

#include <memory>
#include <vector>

namespace incdaw::engine {

/// Plays preloaded audio clips at their timeline positions.
///
/// One node carries every audio clip of one playlist track, the way an
/// InstrumentNode carries a whole channel's notes. Clips are preloaded planar
/// audio (decoded off the audio thread at graph-compile time) and playback is
/// pure indexing off `playPosition` — no state advances between blocks, so a
/// seek or loop wrap needs no special handling: wherever the transport says
/// the block is, that is what plays.
///
/// Honoured per clip today: placement, source offset, gain, mute, linear
/// fade in/out. Deliberately NOT yet: pan, normalize, reverse, pitch,
/// stretch — those arrive with the playlist's audio-clip polish (9b), and
/// silently half-implementing them here would misrepresent the model.
///
/// A clip's audio comes from one of two places: whole files preloaded at
/// compile time (recorded takes, ordinary clips), or the disk streamer's
/// window for files too long to preload. The compiler decides per asset;
/// this node plays both identically — the streamed path serves silence and
/// counts an underrun when the window cannot keep up, it never blocks.
class AudioClipNode final : public Node {
public:
    struct PlacedClip {
        /// Exactly one of these is set. Both are released off-RT with the
        /// graph, like everything else the reaper handles.
        std::shared_ptr<const AudioFileData> audio;
        std::shared_ptr<AudioStream>         stream;

        FramePosition start        = 0;   ///< timeline frame of the clip's first frame
        FrameCount    length       = 0;   ///< frames of the clip on the timeline
        FrameCount    sourceOffset = 0;   ///< frames into the audio where the clip begins
        Sample        gain         = 1.0f;
        bool          muted        = false;
        FrameCount    fadeInFrames  = 0;
        FrameCount    fadeOutFrames = 0;
    };

    /// Build-time only; never after the graph is running.
    void addClip(PlacedClip clip);

    [[nodiscard]] std::size_t clipCount() const noexcept { return clips_.size(); }

    /// Sizes the fetch scratch the streamed path copies through.
    void prepare(SampleRate sampleRate, FrameCount maxBlockSize) override;

    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "AudioClipNode"; }

private:
    std::vector<PlacedClip> clips_;
    std::vector<Sample>     fetchScratch_;   ///< maxBlockSize samples, allocated in prepare
};

} // namespace incdaw::engine
