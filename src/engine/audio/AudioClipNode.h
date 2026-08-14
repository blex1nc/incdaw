#pragma once

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
/// Preloading whole files is correct for recorded takes and ordinary clips;
/// clips longer than memory want the disk streamer, which does not exist yet
/// and is recorded as outstanding Phase 12 work.
class AudioClipNode final : public Node {
public:
    struct PlacedClip {
        std::shared_ptr<const AudioFileData> audio;   ///< released off-RT with the graph

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

    void process(const ProcessContext& context) noexcept override;

    [[nodiscard]] const char* name() const noexcept override { return "AudioClipNode"; }

private:
    std::vector<PlacedClip> clips_;
};

} // namespace incdaw::engine
