#include "plugins/au/AudioUnitInstance.h"

namespace incdaw::plugins {

std::unique_ptr<AudioUnitInstance> AudioUnitInstance::create(const std::string& uid,
                                                             double sampleRate,
                                                             std::uint32_t maxFrames,
                                                             std::string& error)
{
    auto handle = platform::AudioUnitHandle::open(uid, sampleRate, maxFrames, error);

    if (handle == nullptr)
        return nullptr;

    auto instance = std::unique_ptr<AudioUnitInstance>(new AudioUnitInstance());

    for (const platform::AudioUnitParameterDescription& parameter : handle->parameters()) {
        PluginParameterInfo info;
        info.id           = parameter.id;
        info.name         = parameter.name;
        info.minValue     = parameter.minimum;
        info.maxValue     = parameter.maximum;
        info.defaultValue = parameter.defaultValue;
        instance->parameters_.push_back(std::move(info));
    }

    instance->unit_ = std::move(handle);
    return instance;
}

bool AudioUnitInstance::process(float* left, float* right, std::uint32_t frames) noexcept
{
    return unit_ != nullptr && unit_->process(left, right, frames);
}

void AudioUnitInstance::setParameter(std::uint32_t parameterId, double plainValue) noexcept
{
    if (unit_ != nullptr)
        unit_->setParameter(parameterId, plainValue);
}

std::uint32_t AudioUnitInstance::latencyFrames() const noexcept
{
    return unit_ != nullptr ? unit_->latencyFrames() : 0;
}

bool AudioUnitInstance::saveState(std::vector<std::uint8_t>& out) const
{
    out.clear();

    if (unit_ == nullptr)
        return false;

    std::vector<std::byte> blob;

    if (!unit_->saveState(blob))
        return false;

    out.reserve(blob.size());

    for (const std::byte value : blob)
        out.push_back(static_cast<std::uint8_t>(value));

    return true;
}

bool AudioUnitInstance::loadState(const std::uint8_t* data, std::size_t size)
{
    return unit_ != nullptr
        && unit_->restoreState(static_cast<const std::byte*>(static_cast<const void*>(data)), size);
}

bool AudioUnitInstance::hasEditor() const noexcept
{
    return unit_ != nullptr && unit_->hasEditor();
}

bool AudioUnitInstance::openEditor(void* parentView, std::uint32_t& width, std::uint32_t& height)
{
    return unit_ != nullptr && unit_->openEditor(parentView, width, height);
}

void AudioUnitInstance::closeEditor() noexcept
{
    if (unit_ != nullptr)
        unit_->closeEditor();
}

} // namespace incdaw::plugins
