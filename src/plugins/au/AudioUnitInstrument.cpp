#include "plugins/au/AudioUnitInstrument.h"

#include <algorithm>
#include <cstring>

namespace incdaw::plugins {

std::unique_ptr<AudioUnitInstrument> AudioUnitInstrument::create(const std::string& uid,
                                                                 double        sampleRate,
                                                                 std::uint32_t maxFrames,
                                                                 std::string&  error)
{
    auto unit = platform::AudioUnitHandle::openInstrument(uid, sampleRate, maxFrames, error);
    if (unit == nullptr)
        return nullptr;

    std::unique_ptr<AudioUnitInstrument> instrument{new AudioUnitInstrument};

    instrument->name_ = uid;

    for (const platform::AudioUnitParameterDescription& parameter : unit->parameters()) {
        PluginParameterInfo info;
        info.id           = parameter.id;
        info.name         = parameter.name;
        info.minValue     = parameter.minimum;
        info.maxValue     = parameter.maximum;
        info.defaultValue = parameter.defaultValue;
        instrument->parameters_.push_back(std::move(info));
    }

    instrument->unit_ = std::move(unit);
    instrument->left_.assign(static_cast<std::size_t>(maxFrames), 0.0f);
    instrument->right_.assign(static_cast<std::size_t>(maxFrames), 0.0f);

    return instrument;
}

void AudioUnitInstrument::prepare(engine::SampleRate, engine::FrameCount maxBlockSize)
{
    // The unit was opened for a rate and a block size and cannot be told
    // otherwise without being rebuilt, which is what a graph rebuild does. The
    // scratch grows if the caller now wants larger blocks, because rendering
    // into a buffer that is too small is silence at best.
    const auto frames = static_cast<std::size_t>(std::max<engine::FrameCount>(maxBlockSize, 0));

    if (left_.size() < frames) {
        left_.assign(frames, 0.0f);
        right_.assign(frames, 0.0f);
    }
}

void AudioUnitInstrument::allNotesOff() noexcept
{
    if (unit_ == nullptr)
        return;

    for (int channel = 0; channel < 16; ++channel) {
        // Sustain first, then all-notes-off: the second is ignored by anything
        // that honours the first while the pedal is down.
        (void)unit_->sendMidi(static_cast<std::uint8_t>(0xB0 | channel), 64, 0, 0);
        (void)unit_->sendMidi(static_cast<std::uint8_t>(0xB0 | channel), 123, 0, 0);
    }

    heldNotes_ = 0;
}

void AudioUnitInstrument::handleMessage(const engine::MidiMessage& message) noexcept
{
    if (unit_ == nullptr || message.isSystemMessage())
        return;

    // Offset zero: the base class has already split the block, so this message
    // belongs to the first frame of the range about to be rendered.
    (void)unit_->sendMidi(message.status, message.data1, message.data2, 0);

    if (message.isNoteOn())
        ++heldNotes_;
    else if (message.isNoteOff())
        heldNotes_ = std::max(0, heldNotes_ - 1);
}

void AudioUnitInstrument::renderRange(const engine::AudioBufferView& output,
                                      engine::FrameCount             frameCount) noexcept
{
    if (unit_ == nullptr || frameCount <= 0 || output.channelCount() == 0)
        return;

    const auto frames = static_cast<std::size_t>(frameCount);
    if (frames > left_.size())
        return;   // a block larger than what was prepared for; silence beats a stray write

    std::fill_n(left_.begin(), frames, 0.0f);
    std::fill_n(right_.begin(), frames, 0.0f);

    if (!unit_->process(left_.data(), right_.data(), static_cast<std::uint32_t>(frames)))
        return;

    // Added, not written: the instrument shares the channel's buffer, and
    // AudioUnitRender overwrites whatever was there.
    for (std::size_t channel = 0; channel < output.channelCount(); ++channel) {
        const engine::Sample* source = channel == 0 ? left_.data() : right_.data();
        engine::Sample*       target = output.channel(channel);

        for (std::size_t frame = 0; frame < frames; ++frame)
            target[frame] += source[frame];
    }
}

void AudioUnitInstrument::setParameter(std::uint32_t parameterId, double plainValue) noexcept
{
    if (unit_ != nullptr)
        unit_->setParameter(parameterId, plainValue);
}

bool AudioUnitInstrument::saveState(std::vector<std::uint8_t>& out) const
{
    if (unit_ == nullptr)
        return false;

    std::vector<std::byte> blob;
    if (!unit_->saveState(blob))
        return false;

    out.resize(blob.size());
    if (!blob.empty())
        std::memcpy(out.data(), blob.data(), blob.size());

    return true;
}

bool AudioUnitInstrument::loadState(const std::uint8_t* data, std::size_t size)
{
    if (unit_ == nullptr || data == nullptr)
        return false;

    return unit_->restoreState(reinterpret_cast<const std::byte*>(data), size);
}

bool AudioUnitInstrument::hasEditor() const noexcept
{
    return unit_ != nullptr && unit_->hasEditor();
}

bool AudioUnitInstrument::openEditor(void* parentView, std::uint32_t& width, std::uint32_t& height)
{
    return unit_ != nullptr && unit_->openEditor(parentView, width, height);
}

void AudioUnitInstrument::closeEditor() noexcept
{
    if (unit_ != nullptr)
        unit_->closeEditor();
}

} // namespace incdaw::plugins
