#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace incdaw::engine {

/// A node whose internal state can be captured and restored as an opaque
/// blob — a hosted plugin, eventually a stateful built-in.
///
/// Both calls are MAIN-THREAD only and may allocate: state travels at project
/// save and load, never per block. The blob is opaque by contract
/// (docs/PLUGIN_HOST.md §6): the host stores and returns it, and never
/// interprets a byte of it.
class StateIO {
public:
    virtual ~StateIO() = default;

    /// Captures the current state into `out` (replacing its contents).
    /// Returns false on failure — the caller must then KEEP whatever blob it
    /// already had, which is what lets state survive a misbehaving plugin.
    [[nodiscard]] virtual bool saveState(std::vector<std::uint8_t>& out) const = 0;

    /// Restores a previously captured blob. False when the owner rejects it;
    /// the owner is expected to remain usable with its previous state.
    [[nodiscard]] virtual bool loadState(const std::uint8_t* data, std::size_t size) = 0;
};

} // namespace incdaw::engine
