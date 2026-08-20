#include "engine/dsp/effects/BuiltinEffect.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace incdaw::engine::dsp {

double dbToGain(double db) noexcept
{
    return std::pow(10.0, db / 20.0);
}

void sumInputsInto(const ProcessContext& context) noexcept
{
    for (std::size_t index = 0; index < context.inputCount; ++index)
        context.output.addFrom(context.input(index));
}

void sumInputsInto(const ProcessContext& context, std::size_t skippedInput) noexcept
{
    for (std::size_t index = 0; index < context.inputCount; ++index)
        if (index != skippedInput)
            context.output.addFrom(context.input(index));
}

namespace {

constexpr std::uint32_t stateVersion = 1;

void appendU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    out.push_back(static_cast<std::uint8_t>(value & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 16) & 0xFFu));
    out.push_back(static_cast<std::uint8_t>((value >> 24) & 0xFFu));
}

void appendF64(std::vector<std::uint8_t>& out, double value)
{
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int shift = 0; shift < 64; shift += 8)
        out.push_back(static_cast<std::uint8_t>((bits >> shift) & 0xFFu));
}

bool readU32(const std::uint8_t*& data, std::size_t& size, std::uint32_t& value)
{
    if (size < 4)
        return false;

    value = static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8)
          | (static_cast<std::uint32_t>(data[2]) << 16)
          | (static_cast<std::uint32_t>(data[3]) << 24);
    data += 4;
    size -= 4;
    return true;
}

bool readF64(const std::uint8_t*& data, std::size_t& size, double& value)
{
    if (size < 8)
        return false;

    std::uint64_t bits = 0;
    for (int index = 7; index >= 0; --index)
        bits = (bits << 8) | data[index];

    std::memcpy(&value, &bits, sizeof(value));
    data += 8;
    size -= 8;
    return true;
}

} // namespace

BuiltinEffect::BuiltinEffect(const EffectParameter* parameters, std::size_t parameterCount)
    : parameters_(parameters), values_(parameterCount)
{
    for (std::size_t index = 0; index < parameterCount; ++index)
        values_[index].store(parameters_[index].defaultValue, std::memory_order_relaxed);
}

void BuiltinEffect::setParameter(std::uint32_t parameterId, double plainValue) noexcept
{
    for (std::size_t index = 0; index < values_.size(); ++index) {
        if (parameters_[index].id != parameterId)
            continue;

        const double clamped =
            std::clamp(plainValue, parameters_[index].minValue, parameters_[index].maxValue);
        values_[index].store(parameters_[index].stepped ? std::round(clamped) : clamped,
                             std::memory_order_relaxed);
        return;
    }
}

double BuiltinEffect::value(std::uint32_t parameterId) const noexcept
{
    for (std::size_t index = 0; index < values_.size(); ++index)
        if (parameters_[index].id == parameterId)
            return values_[index].load(std::memory_order_relaxed);

    return 0.0;
}

bool BuiltinEffect::saveState(std::vector<std::uint8_t>& out) const
{
    out.clear();
    appendU32(out, stateVersion);
    appendU32(out, static_cast<std::uint32_t>(values_.size()));

    for (std::size_t index = 0; index < values_.size(); ++index) {
        appendU32(out, parameters_[index].id);
        appendF64(out, values_[index].load(std::memory_order_relaxed));
    }

    return true;
}

bool BuiltinEffect::loadState(const std::uint8_t* data, std::size_t size)
{
    std::vector<std::pair<std::uint32_t, double>> decoded;
    if (!decodeState(data, size, decoded))
        return false;

    // Unknown ids are skipped, not errors: yesterday's build may have
    // saved a parameter this one no longer has.
    for (const auto& [id, value] : decoded)
        setParameter(id, value);

    return true;
}

bool BuiltinEffect::decodeState(const std::uint8_t* data, std::size_t size,
                                std::vector<std::pair<std::uint32_t, double>>& out)
{
    out.clear();

    std::uint32_t version = 0;
    std::uint32_t count   = 0;
    if (!readU32(data, size, version) || version != stateVersion
        || !readU32(data, size, count))
        return false;

    for (std::uint32_t entry = 0; entry < count; ++entry) {
        std::uint32_t id    = 0;
        double        value = 0.0;
        if (!readU32(data, size, id) || !readF64(data, size, value))
            return false;

        out.emplace_back(id, value);
    }

    return true;
}

} // namespace incdaw::engine::dsp
