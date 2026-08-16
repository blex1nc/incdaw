#include "app/commands/SlicerCommands.h"

#include <algorithm>

namespace incdaw::app {

bool SliceAssetCommand::execute(Project& project)
{
    const project::AudioAsset* asset = nullptr;
    for (const project::AudioAsset& candidate : project.audioAssets())
        if (candidate.id == asset_)
            asset = &candidate;

    project::Pattern* pattern = project.findPattern(pattern_);
    if (asset == nullptr || pattern == nullptr || onsets_.empty())
        return false;

    if (!minted_) {
        project::Channel& created = project.addChannel(
            (asset->relativePath.empty() ? asset->absolutePath : asset->relativePath)
            + " slices");
        created.instrument = plugins::builtinSampler();

        // One zone per slice: its own key, playing exactly its span.
        for (std::size_t slice = 0; slice < onsets_.size(); ++slice) {
            const int key = firstKey_ + static_cast<int>(slice);
            if (key > 127)
                break;

            project::ChannelSamplerZone zone;
            zone.asset   = asset_;
            zone.rootKey = key;
            zone.keyLow  = key;
            zone.keyHigh = key;
            zone.start   = onsets_[slice];
            zone.end     = slice + 1 < onsets_.size() ? onsets_[slice + 1]
                                                      : asset->frameCount;
            created.samplerZones.push_back(zone);
        }

        channel_      = created;
        channelIndex_ = project.channels().size() - 1;
        minted_       = true;
    } else {
        project.insertChannel(channelIndex_, channel_);
    }

    // The pattern replays the loop: each slice's note starts where the slice
    // started, measured through the tempo map, and holds until the next.
    const engine::TempoMap& tempoMap = project.tempoMap();

    std::vector<project::MidiEvent>& events = pattern->contentFor(channel_.id).events;
    firstNoteIndex_                         = events.size();
    noteCount_                              = 0;

    for (std::size_t slice = 0; slice < channel_.samplerZones.size(); ++slice) {
        const project::ChannelSamplerZone& zone = channel_.samplerZones[slice];

        const Tick start = tempoMap.tickForFrame(
            static_cast<project::FramePosition>(zone.start));
        const Tick end = tempoMap.tickForFrame(
            static_cast<project::FramePosition>(zone.end));

        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = start;
        note.duration = std::max<Tick>(1, end - start);
        note.key      = zone.rootKey;
        note.value    = 100;

        events.push_back(note);
        ++noteCount_;
    }

    return noteCount_ > 0;
}

void SliceAssetCommand::undo(Project& project)
{
    if (project::Pattern* pattern = project.findPattern(pattern_)) {
        if (std::vector<project::MidiEvent>* events = pattern->events(channel_.id)) {
            const std::size_t last = std::min(events->size(), firstNoteIndex_ + noteCount_);
            if (firstNoteIndex_ < last)
                events->erase(events->begin() + static_cast<std::ptrdiff_t>(firstNoteIndex_),
                              events->begin() + static_cast<std::ptrdiff_t>(last));
        }
    }

    (void)project.removeChannel(channel_.id);
}

} // namespace incdaw::app
