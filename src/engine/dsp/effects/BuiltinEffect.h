#pragma once

#include "engine/graph/Node.h"
#include "engine/graph/ParameterSink.h"
#include "engine/graph/StateIO.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace incdaw::engine::dsp {

/// Decibels to linear gain. 0 dB is exactly 1.0, which is what lets a
/// defaulted effect null against its input.
[[nodiscard]] double dbToGain(double db) noexcept;

/// Sums every graph input into the context's output — what "insert" means:
/// the effect sees the mixed signal. Every builtin effect's process starts
/// here, exactly as the hosted PluginNode's does.
void sumInputsInto(const ProcessContext& context) noexcept;

/// One parameter of a builtin effect, in the same format-agnostic terms a
/// hosted plugin's discovery produces — which is exactly the point: the
/// registry, the automation system and the UI treat a builtin and a hosted
/// insert identically (Phase 15's exit criterion forbids special-casing).
struct EffectParameter {
    std::uint32_t id           = 0;
    const char*   name         = "";
    double        minValue     = 0.0;
    double        maxValue     = 1.0;
    double        defaultValue = 0.0;
    bool          stepped      = false;
};

/// The shared base of every builtin effect.
///
/// A builtin effect is an engine Node that also carries the two capability
/// interfaces inserts already use: a ParameterSink (automation writes plain
/// values, audio-thread, once per block per parameter) and StateIO (its
/// parameter values persist through the same plugins/<slot>.state files a
/// hosted plugin's opaque blob rides in). Values live in atomics — a store
/// is the whole hand-off, so there is no queue to drain and nothing to lock.
class BuiltinEffect : public Node, public ParameterSink, public StateIO {
public:
    /// `parameters` must point at static storage — the descriptor table of
    /// the concrete effect — because the base keeps the pointer.
    BuiltinEffect(const EffectParameter* parameters, std::size_t parameterCount);

    // ── Node capabilities ────────────────────────────────────────────────
    [[nodiscard]] ParameterSink* parameterSink() noexcept override { return this; }
    [[nodiscard]] StateIO*       stateIO() noexcept override { return this; }

    // ── ParameterSink ────────────────────────────────────────────────────
    /// Clamps into the parameter's range and stores. Realtime-safe.
    void setParameter(std::uint32_t parameterId, double plainValue) noexcept override;

    // ── StateIO ──────────────────────────────────────────────────────────
    /// The blob is id-keyed (version, count, {id, value}...), so an effect
    /// that gains parameters still loads yesterday's state.
    [[nodiscard]] bool saveState(std::vector<std::uint8_t>& out) const override;
    [[nodiscard]] bool loadState(const std::uint8_t* data, std::size_t size) override;

    // ── Introspection (build/UI side) ────────────────────────────────────
    [[nodiscard]] const EffectParameter* parameters() const noexcept { return parameters_; }
    [[nodiscard]] std::size_t parameterCount() const noexcept { return values_.size(); }

    /// Current plain value; the audio thread reads these relaxed per block.
    [[nodiscard]] double value(std::uint32_t parameterId) const noexcept;

protected:
    /// Value by table INDEX — the per-block read every effect's process uses.
    [[nodiscard]] double valueAt(std::size_t index) const noexcept
    {
        return values_[index].load(std::memory_order_relaxed);
    }

private:
    const EffectParameter*           parameters_;
    std::vector<std::atomic<double>> values_;
};

} // namespace incdaw::engine::dsp
