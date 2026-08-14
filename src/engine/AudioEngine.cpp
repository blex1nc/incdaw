#include "engine/AudioEngine.h"

#include "engine/core/Denormals.h"
#include "engine/core/RealtimeGuard.h"

#include <chrono>

namespace incdaw::engine {
namespace {

/// A retired graph is released once the audio thread has completed this many
/// blocks since the swap.
///
/// One would be enough in theory: after the swap, the next callback necessarily
/// loads the new pointer. Two gives a margin for a callback already in flight at
/// the moment of the store, which costs nothing and removes the need to reason
/// about that race at all.
constexpr std::uint64_t retirementGraceBlocks = 2;

} // namespace

AudioEngine::AudioEngine() : device_(platform::AudioDevice::create()) {}

AudioEngine::~AudioEngine()
{
    stop();

    // Nothing is rendering any more, so everything retired is safe to release.
    active_.store(nullptr, std::memory_order_release);
    owned_.reset();

    const std::lock_guard<std::mutex> lock(retiredMutex_);
    retired_.clear();
}

std::vector<platform::AudioDeviceInfo> AudioEngine::availableDevices() const
{
    return device_ != nullptr ? device_->enumerateDevices() : std::vector<platform::AudioDeviceInfo>{};
}

bool AudioEngine::start(const platform::AudioDeviceConfig& config, std::string& error)
{
    if (device_ == nullptr) {
        error = "no audio backend available on this platform";
        return false;
    }

    if (!device_->open(config, error))
        return false;

    return device_->start(*this, error);
}

void AudioEngine::stop()
{
    if (device_ != nullptr) {
        device_->stop();
        device_->close();
    }
}

bool AudioEngine::isRunning() const noexcept
{
    return device_ != nullptr && device_->isRunning();
}

void AudioEngine::setGraph(std::unique_ptr<CompiledGraph> graph)
{
    CompiledGraph* raw = graph.get();

    // Publish the new graph first, then retire the old one. Doing it the other
    // way round would leave a window in which the audio thread holds a pointer
    // that is already on the retirement list.
    std::unique_ptr<CompiledGraph> previous = std::move(owned_);
    owned_ = std::move(graph);
    active_.store(raw, std::memory_order_release);

    if (previous != nullptr) {
        const std::lock_guard<std::mutex> lock(retiredMutex_);
        retired_.push_back({std::move(previous), blockCounter_.load(std::memory_order_acquire)});
    }
}

void AudioEngine::collectRetiredGraphs()
{
    const std::uint64_t now     = blockCounter_.load(std::memory_order_acquire);
    const bool          stopped = !isRunning();

    const std::lock_guard<std::mutex> lock(retiredMutex_);

    for (auto entry = retired_.begin(); entry != retired_.end();) {
        // When the device is stopped the audio thread cannot be inside a
        // callback at all, so the grace period is unnecessary.
        const bool safe = stopped || now >= entry->retiredAtBlock + retirementGraceBlocks;
        entry = safe ? retired_.erase(entry) : entry + 1;
    }
}

std::size_t AudioEngine::retiredGraphCount() const
{
    const std::lock_guard<std::mutex> lock(retiredMutex_);
    return retired_.size();
}

double AudioEngine::sampleRate() const noexcept
{
    return device_ != nullptr ? device_->actualSampleRate() : 0.0;
}

std::int64_t AudioEngine::bufferSize() const noexcept
{
    return device_ != nullptr ? device_->actualBufferSize() : 0;
}

std::int64_t AudioEngine::maxServiceableBlockSize() const noexcept
{
    return device_ != nullptr ? device_->maxServiceableBlockSize() : 0;
}

std::size_t AudioEngine::outputChannels() const noexcept
{
    return device_ != nullptr ? device_->actualOutputChannels() : 0;
}

std::int64_t AudioEngine::totalOutputLatencyFrames() const noexcept
{
    return device_ != nullptr ? device_->totalOutputLatencyFrames() : 0;
}

std::string AudioEngine::deviceName() const
{
    return device_ != nullptr ? device_->deviceName() : std::string{};
}

void AudioEngine::audioDeviceAboutToStart(double sampleRateHz, std::int64_t blockSize)
{
    profiler_.configure(sampleRateHz, blockSize);
    midiInput_.resetCounters();
    blockMidi_.clear();
    blockMidi_.resetOverflowCount();
    blockCounter_.store(0, std::memory_order_release);
    nonFiniteBlocks_.store(0, std::memory_order_relaxed);
    rt::resetViolations();
}

void AudioEngine::audioDeviceStopped() {}

void AudioEngine::renderAudioBlock(float* const* outputChannels, std::size_t channelCount,
                                   std::int64_t frameCount, std::uint64_t blockHostTimeNanos) noexcept
{
    // Marks this thread realtime for the guard, and flushes denormals before any
    // DSP runs. Both must be the very first things in the callback.
    const rt::ScopedRealtimeContext realtimeScope;
    const ScopedNoDenormals         denormalScope;

    const auto started = std::chrono::steady_clock::now();

    const AudioBufferView output{outputChannels, channelCount, frameCount};
    output.clear();

    // Collected once for the whole block, before it is split: MIDI offsets are
    // relative to the block the device handed us, and each segment re-bases the
    // ones that fall inside it.
    midiInput_.collectForBlock(blockMidi_, blockHostTimeNanos, frameCount,
                               device_ != nullptr ? device_->actualSampleRate() : 0.0);

    if (CompiledGraph* graph = active_.load(std::memory_order_acquire)) {
        // The transport decides how this block is divided. A loop wrap in the
        // middle of a block becomes two segments, each rendered with its own
        // timeline position, so events land on their exact frame rather than
        // being rounded to the block boundary.
        BlockSegment      plan[Transport::maxSegmentsPerBlock];
        const std::size_t segmentCount =
            transport_.processBlock(frameCount, plan, Transport::maxSegmentsPerBlock);

        for (std::size_t index = 0; index < segmentCount; ++index) {
            const BlockSegment& segment = plan[index];
            if (segment.length <= 0)
                continue;

            // MIDI offsets are relative to the whole block; each segment
            // re-bases the ones that fall inside it before rendering.
            segmentMidi_ = blockMidi_;
            segmentMidi_.rebase(-segment.offset, segment.length);

            graph->process(output.subBlock(segment.offset, segment.length),
                           segment.length, segment.startFrame, &segmentMidi_);
        }

        if (output.hasNonFiniteSamples()) {
            // Contain it here rather than letting it reach the speakers or
            // poison every downstream buffer next block.
            output.clear();
            nonFiniteBlocks_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    const auto finished = std::chrono::steady_clock::now();
    profiler_.record(std::chrono::duration<double>(finished - started).count(), frameCount,
                     device_ != nullptr ? device_->actualSampleRate() : 0.0);

    blockCounter_.fetch_add(1, std::memory_order_acq_rel);
}

} // namespace incdaw::engine
