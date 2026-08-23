#include "app/commands/ConsolidateCommands.h"

#include "engine/audio/WavFile.h"
#include "engine/dsp/MixerStripNode.h"
#include "project/OfflineRender.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace incdaw::app {
namespace {

/// The clip's span on the timeline, in frames, whichever time base it is
/// anchored to (docs/DECISIONS.md D-013).
std::pair<project::FramePosition, project::FramePosition> spanOf(
    const Project& project, const Clip& clip)
{
    if (clip.type == project::ClipType::audio)
        return {clip.start, clip.start + static_cast<project::FramePosition>(clip.length)};

    const engine::TempoMap& tempoMap = project.tempoMap();

    const project::Tick start  = project::clipStartTicks(clip, tempoMap);
    const project::Tick length = project::clipLengthTicks(clip, tempoMap);

    return {tempoMap.frameForTick(start), tempoMap.frameForTick(start + length)};
}

} // namespace

std::string consolidationRefusal(const Project& project, const ClipIds& clips)
{
    if (clips.empty())
        return "nothing selected";

    const Clip* first = project.findClip(clips.front());
    if (first == nullptr)
        return "nothing selected";

    project::FramePosition from = std::numeric_limits<project::FramePosition>::max();
    project::FramePosition to   = 0;

    for (const EntityId id : clips) {
        const Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        // One track, because the result is one clip on one track. A selection
        // spanning two has no single answer, and guessing one would throw away
        // whichever the user did not mean.
        if (clip->track != first->track)
            return "select clips on one track";

        const auto [clipFrom, clipTo] = spanOf(project, *clip);
        from = std::min(from, clipFrom);
        to   = std::max(to, clipTo);
    }

    if (to <= from)
        return "the selection covers no time";

    return {};
}

bool ConsolidateClipsCommand::execute(Project& project)
{
    if (minted_) {
        // Redo: everything has already been rendered and imported, so this is
        // the same list surgery the first run finished with.
        for (auto entry = removed_.rbegin(); entry != removed_.rend(); ++entry)
            (void)project.removeClip(entry->clip.id);

        restoreImportedAsset(project, asset_);
        project.insertClip(createdIndex_, created_);
        return true;
    }

    error_ = consolidationRefusal(project, clips_);
    if (!error_.empty())
        return false;

    const Clip*    first = project.findClip(clips_.front());
    const EntityId track = first->track;
    const int      lane  = first->lane;

    project::FramePosition from = std::numeric_limits<project::FramePosition>::max();
    project::FramePosition to   = 0;

    for (const EntityId id : clips_) {
        const Clip* clip = project.findClip(id);
        if (clip == nullptr)
            continue;

        const auto [clipFrom, clipTo] = spanOf(project, *clip);
        from = std::min(from, clipFrom);
        to   = std::max(to, clipTo);
    }

    // A copy holding only these clips, with every mixer node flat. What comes
    // back is then the clips' own audio: put it on the same track, through the
    // same strip, and the arrangement sounds as it did.
    Project stripped = project;

    stripped.clips().erase(
        std::remove_if(stripped.clips().begin(), stripped.clips().end(),
                       [this](const Clip& clip) {
                           return std::find(clips_.begin(), clips_.end(), clip.id)
                               == clips_.end();
                       }),
        stripped.clips().end());

    // Every strip becomes true unity, which is not the same as "all its
    // numbers at their defaults": a strip at centre still costs the pan law's
    // -3 dB, and the master also carries the standing headroom trim every
    // compile applies. Both are folded into the strip's own volume here, so a
    // strip passes its input through unchanged whatever the routing looks
    // like — no counting of how many strips a signal crosses, and nothing to
    // correct after the render.
    engine::Sample centreLeft  = 1.0f;
    engine::Sample centreRight = 1.0f;
    engine::dsp::MixerStripNode::panGains(0.0, centreLeft, centreRight);

    const double neutral = centreLeft > 0.0f ? 1.0 / static_cast<double>(centreLeft) : 1.0;

    const auto headroom = project::GraphCompileOptions{}.masterGain;
    const double masterNeutral =
        headroom > engine::Sample{0} ? neutral / static_cast<double>(headroom) : neutral;

    for (project::MixerNode& node : stripped.mixerNodes()) {
        node.volume           = node.id == stripped.masterMixerNode() ? masterNeutral : neutral;
        node.pan              = 0.0;
        node.muted            = false;
        node.soloed           = false;
        node.polarityFlip     = false;
        node.stereoSeparation = 0.0;
        node.inserts.clear();
    }

    // A send is a path to somewhere else. Consolidating a clip must capture
    // what the clip is, not a copy of it arriving through a reverb bus.
    stripped.routing().erase(
        std::remove_if(stripped.routing().begin(), stripped.routing().end(),
                       [](const project::RoutingConnection& connection) {
                           return connection.isSend;
                       }),
        stripped.routing().end());

    // Track and channel mutes go too: a muted clip in the selection is the
    // user's business, but a mute somewhere else in the project must not
    // silence the render.
    for (project::Track& entry : stripped.tracks()) {
        entry.muted  = false;
        entry.soloed = false;
    }

    for (project::Channel& channel : stripped.channels()) {
        channel.muted  = false;
        channel.soloed = false;
    }

    project::RenderOptions options;
    options.sampleRate = project.tempoMap().sampleRate() > 0.0
                             ? project.tempoMap().sampleRate()
                             : 48000.0;
    options.bitDepth   = project::RenderOptions::BitDepth::float32;
    options.dither     = false;
    options.parameters = parameters_;

    // Exactly the span, and no tail: the replacement has to occupy the same
    // frames as what it replaces, or every clip after it would need moving.
    options.regionStart  = from;
    options.regionLength = static_cast<project::FrameCount>(to - from);
    options.tailSeconds  = 0.0;

    const auto rendered = project::renderProject(stripped, project.tempoMap(), options);
    if (!rendered) {
        error_ = rendered.error.empty() ? "the selection did not render" : rendered.error;
        return false;
    }


    std::error_code code;
    std::filesystem::create_directories(output_.parent_path(), code);

    if (!engine::WavFile::write(output_, rendered.audio)) {
        error_ = "could not write " + output_.string();
        return false;
    }

    if (!importAudioAsset(project, output_.string(), asset_)) {
        error_ = "could not read back " + output_.string();
        return false;
    }

    // Only now is anything removed: up to here every failure has left the
    // project exactly as it was.
    removed_.clear();

    std::vector<std::size_t> indices;
    for (const EntityId id : clips_) {
        const std::size_t index = project.indexOfClip(id);
        if (index != Project::notFound)
            indices.push_back(index);
    }

    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

    for (auto index = indices.rbegin(); index != indices.rend(); ++index) {
        removed_.push_back({*index, project.clips()[*index]});
        project.clips().erase(project.clips().begin() + static_cast<std::ptrdiff_t>(*index));
    }

    Clip& added = project.addClip(project::ClipType::audio, track, asset_.id);

    added.start  = from;
    added.length = std::min(static_cast<project::FrameCount>(to - from),
                            rendered.audio.frameCount);
    added.lane   = lane;
    added.name   = "Consolidated";

    created_      = added;
    createdIndex_ = project.clips().size() - 1;
    minted_       = true;
    return true;
}

void ConsolidateClipsCommand::undo(Project& project)
{
    (void)project.removeClip(created_.id);

    if (asset_.created)
        (void)project.removeAudioAsset(asset_.id);

    for (auto entry = removed_.rbegin(); entry != removed_.rend(); ++entry)
        project.insertClip(entry->index, entry->clip);
}

} // namespace incdaw::app
