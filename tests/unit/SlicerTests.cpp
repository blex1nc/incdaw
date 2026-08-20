#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/SlicerCommands.h"
#include "engine/audio/OnsetDetection.h"

#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using namespace incdaw::engine;
using incdaw::engine::audio::detectOnsets;

namespace {

constexpr double sampleRate = 48000.0;

/// A drum-loop stand-in: decaying bursts at the given frame positions over a
/// silence bed.
AudioFileData loopWithHits(const std::vector<std::size_t>& hits, double seconds)
{
    AudioFileData data;
    data.sampleRate   = sampleRate;
    data.channelCount = 1;
    data.frameCount   = static_cast<FrameCount>(sampleRate * seconds);
    data.channels.assign(1, std::vector<Sample>(static_cast<std::size_t>(data.frameCount), 0.0f));

    for (const std::size_t at : hits)
        for (std::size_t frame = 0; frame < 2400 && at + frame < data.channels[0].size(); ++frame)
            data.channels[0][at + frame] = static_cast<Sample>(
                0.8 * std::exp(-static_cast<double>(frame) / 400.0)
                * std::sin(2.0 * 3.14159265358979 * 180.0 * static_cast<double>(frame)
                           / sampleRate));

    return data;
}

} // namespace

TEST_CASE("onset detection finds every hit near its true position")
{
    // Four hits on the beat at 120 BPM: 0, 0.5, 1.0, 1.5 seconds.
    const std::vector<std::size_t> hits = { 0, 24000, 48000, 72000 };
    const AudioFileData            loop = loopWithHits(hits, 2.0);

    const std::vector<FrameCount> onsets = detectOnsets(loop);
    REQUIRE(onsets.size() == 4);

    for (std::size_t hit = 0; hit < hits.size(); ++hit) {
        const auto error = std::llabs(static_cast<long long>(onsets[hit])
                                      - static_cast<long long>(hits[hit]));
        CHECK(error < static_cast<long long>(sampleRate * 0.006));   // within 6 ms
    }
}

TEST_CASE("detection does not invent onsets in steady material")
{
    AudioFileData tone;
    tone.sampleRate   = sampleRate;
    tone.channelCount = 1;
    tone.frameCount   = static_cast<FrameCount>(sampleRate);
    tone.channels.assign(1, std::vector<Sample>(static_cast<std::size_t>(tone.frameCount)));
    for (std::size_t frame = 0; frame < tone.channels[0].size(); ++frame)
        tone.channels[0][frame] = static_cast<Sample>(
            0.5 * std::sin(2.0 * 3.14159265358979 * 220.0 * static_cast<double>(frame)
                           / sampleRate));

    // The tone's own start is one attack; nothing else in it is.
    CHECK(detectOnsets(tone).size() <= 1);
}

TEST_CASE("slicing lands a channel of zones and a pattern of timing")
{
    project::Project     project;
    app::CommandRegistry registry { project };

    project::AudioAsset& asset = project.addAudioAsset("media/loop.wav");
    asset.sampleRate           = sampleRate;
    asset.frameCount           = 96000;
    const project::EntityId patternId = project.addPattern("P1").id;

    // At 120 BPM / 48 kHz a beat is 24000 frames — hits on beats 0..3.
    const std::vector<project::FrameCount> onsets = { 0, 24000, 48000, 72000 };

    auto  command = std::make_unique<app::SliceAssetCommand>(asset.id, patternId, onsets);
    auto* raw     = command.get();
    REQUIRE(registry.execute(std::move(command)));

    const project::EntityId channelId = raw->channelId();
    const project::Channel* channel   = project.findChannel(channelId);
    REQUIRE(channel != nullptr);

    CHECK(channel->instrument == plugins::builtinSampler());
    REQUIRE(channel->samplerZones.size() == 4);

    // Chromatic keys from C3, spans chained hit-to-hit, last to the end.
    for (std::size_t slice = 0; slice < 4; ++slice) {
        const project::ChannelSamplerZone& zone = channel->samplerZones[slice];
        CHECK(zone.rootKey == 48 + static_cast<int>(slice));
        CHECK(zone.keyLow == zone.rootKey);
        CHECK(zone.keyHigh == zone.rootKey);
        CHECK(zone.start == onsets[slice]);
        CHECK(zone.end == (slice + 1 < 4 ? onsets[slice + 1] : asset.frameCount));
    }

    // The pattern replays the loop: notes on consecutive beats.
    const std::vector<project::MidiEvent>* events =
        project.findPattern(patternId)->events(channelId);
    REQUIRE(events != nullptr);
    REQUIRE(events->size() == 4);

    for (std::size_t slice = 0; slice < 4; ++slice) {
        CHECK((*events)[slice].tick
              == static_cast<Tick>(slice) * engine::ticksPerQuarterNote);
        CHECK((*events)[slice].key == 48 + static_cast<int>(slice));
        CHECK((*events)[slice].duration == engine::ticksPerQuarterNote);
    }

    // One undo takes back the channel, its zones and the notes together.
    CHECK(registry.undo());
    CHECK(project.findChannel(channelId) == nullptr);
    const std::vector<project::MidiEvent>* removed =
        project.findPattern(patternId)->events(channelId);
    CHECK((removed == nullptr || removed->empty()));

    // Redo recreates the same channel id, so later references survive.
    CHECK(registry.redo());
    CHECK(project.findChannel(channelId) != nullptr);
    CHECK(project.findChannel(channelId)->samplerZones.size() == 4);
}

TEST_CASE("slicing an unknown asset or empty onset list is refused")
{
    project::Project     project;
    app::CommandRegistry registry { project };

    const project::EntityId patternId = project.addPattern("P1").id;

    CHECK_FALSE(registry.execute(std::make_unique<app::SliceAssetCommand>(
        project::EntityId{999}, patternId, std::vector<project::FrameCount>{ 0 })));

    project::AudioAsset& asset = project.addAudioAsset("media/loop.wav");
    CHECK_FALSE(registry.execute(std::make_unique<app::SliceAssetCommand>(
        asset.id, patternId, std::vector<project::FrameCount>{})));

    CHECK_FALSE(registry.canUndo());
}
