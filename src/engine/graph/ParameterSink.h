#pragma once

#include <cstdint>

namespace incdaw::engine {

/// A destination for parameter values that is not a mixer strip: a hosted
/// plugin, or any future node with parameters of its own.
///
/// The contract mirrors the strip setters': `setParameter` is called on the
/// audio thread, once per block per automated parameter, and must be
/// realtime-safe — in practice it pushes onto a preallocated queue the owner
/// drains inside its own processing. `parameterId` and `plainValue` are in
/// the owner's native terms (for CLAP, clap_param_info.id and the plain,
/// un-normalised value): the normalised-to-plain mapping belongs to the
/// registry's applier, not the sink.
class ParameterSink {
public:
    virtual ~ParameterSink() = default;

    virtual void setParameter(std::uint32_t parameterId, double plainValue) noexcept = 0;
};

} // namespace incdaw::engine
