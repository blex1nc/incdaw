#include "engine/instrument/SamplerStream.h"

#include "engine/audio/WavStreamReader.h"

#include <algorithm>
#include <vector>

namespace incdaw::engine {

std::shared_ptr<SamplerZoneStream> SamplerZoneStream::create(const std::filesystem::path& path,
                                                             FrameCount   headFrames,
                                                             std::string& error,
                                                             FrameCount   segmentFrames)
{
    WavStreamReader reader;
    if (const auto opened = reader.open(path); !opened) {
        error = opened.error;
        return nullptr;
    }

    if (reader.channelCount() == 0 || reader.channelCount() > maxSourceChannels) {
        error = "streamed zones support 1.." + std::to_string(maxSourceChannels)
              + " channels; file has " + std::to_string(reader.channelCount());
        return nullptr;
    }

    auto zoneStream         = std::make_shared<SamplerZoneStream>();
    zoneStream->fileFrames_ = reader.frameCount();

    // One guard frame past the head: the last head frame interpolates against
    // real material instead of itself, so the RAM-to-stream seam is exact.
    const FrameCount decodeFrames =
        std::min<FrameCount>(headFrames + 1, reader.frameCount());

    auto head          = std::make_shared<AudioFileData>();
    head->sampleRate   = reader.sampleRate();
    head->channelCount = reader.channelCount();
    head->frameCount   = decodeFrames;
    head->channels.assign(head->channelCount,
                          std::vector<Sample>(static_cast<std::size_t>(decodeFrames)));

    std::vector<Sample*> pointers;
    pointers.reserve(head->channelCount);
    for (auto& channel : head->channels)
        pointers.push_back(channel.data());

    if (!reader.readAt(0, decodeFrames, pointers.data(), head->channelCount)) {
        error = "could not read the head of " + path.string();
        return nullptr;
    }

    zoneStream->head_ = std::move(head);

    for (Slot& slot : zoneStream->slots_) {
        slot.stream = std::make_shared<AudioStream>();
        if (const auto opened = slot.stream->open(path, segmentFrames); !opened) {
            error = opened.error;
            return nullptr;
        }

        // Park each window at the hand-over point: the head plays from RAM,
        // so the stream's first useful frames are the ones after it.
        slot.stream->prefill(std::max<FrameCount>(0, decodeFrames - 1));
    }

    return zoneStream;
}

int SamplerZoneStream::claimSlot() noexcept
{
    for (std::size_t index = 0; index < poolSize; ++index) {
        bool expected = false;
        if (slots_[index].inUse.compare_exchange_strong(expected, true,
                                                        std::memory_order_acquire))
            return static_cast<int>(index);
    }

    return -1;
}

void SamplerZoneStream::releaseSlot(int slot) noexcept
{
    if (slot >= 0 && slot < static_cast<int>(poolSize))
        slots_[static_cast<std::size_t>(slot)].inUse.store(false, std::memory_order_release);
}

AudioStream* SamplerZoneStream::streamFor(int slot) noexcept
{
    if (slot < 0 || slot >= static_cast<int>(poolSize))
        return nullptr;

    return slots_[static_cast<std::size_t>(slot)].stream.get();
}

void SamplerZoneStream::registerWith(DiskStreamer& streamer)
{
    for (Slot& slot : slots_)
        streamer.add(slot.stream);
}

std::uint64_t SamplerZoneStream::underrunCount() const noexcept
{
    std::uint64_t total = 0;
    for (const Slot& slot : slots_)
        if (slot.stream != nullptr)
            total += slot.stream->underrunCount();

    return total;
}

} // namespace incdaw::engine
