#pragma once

#include "engine/core/Time.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace incdaw::engine {

/// Curve shape of the segment following a point. Mirrors the model's enum;
/// engine/ cannot see project/, so the compiler translates.
enum class AutomationShape : int { linear, hold, smooth, exponential };

struct AutomationPoint {
    Tick            tick    = 0;
    float           value   = 0.0f;   ///< normalised 0..1
    AutomationShape shape   = AutomationShape::linear;
    float           tension = 0.0f;   ///< -1..1, bends the segment after this point
};

/// One parameter's compiled envelope, evaluated on the audio thread.
///
/// Points are sorted by tick at set time, so evaluation is a binary search plus
/// segment arithmetic: no allocation, no unbounded work, safe per block.
class AutomationSequence {
public:
    void setPoints(std::vector<AutomationPoint> points)
    {
        points_ = std::move(points);
        std::stable_sort(points_.begin(), points_.end(),
                         [](const AutomationPoint& a, const AutomationPoint& b) {
                             return a.tick < b.tick;
                         });
    }

    [[nodiscard]] const std::vector<AutomationPoint>& points() const noexcept { return points_; }
    [[nodiscard]] bool isEmpty() const noexcept { return points_.empty(); }

    /// Value at `tick`. Before the first point it holds the first value, after
    /// the last it holds the last — an envelope, not a loop.
    [[nodiscard]] float valueAt(Tick tick) const noexcept
    {
        if (points_.empty())
            return 0.0f;

        if (tick <= points_.front().tick)
            return points_.front().value;

        if (tick >= points_.back().tick)
            return points_.back().value;

        // First point strictly after `tick`; its predecessor starts the segment.
        const auto after = std::upper_bound(points_.begin(), points_.end(), tick,
                                            [](Tick value, const AutomationPoint& point) {
                                                return value < point.tick;
                                            });

        const AutomationPoint& from = *(after - 1);
        const AutomationPoint& to   = *after;

        if (from.shape == AutomationShape::hold)
            return from.value;

        const auto span = static_cast<float>(to.tick - from.tick);
        if (span <= 0.0f)
            return to.value;

        float position = static_cast<float>(tick - from.tick) / span;

        switch (from.shape) {
            case AutomationShape::smooth:
                // Smoothstep: zero slope at both ends, which is what makes a
                // hand-drawn ramp sound like a move rather than two corners.
                position = position * position * (3.0f - 2.0f * position);
                break;

            case AutomationShape::exponential:
                position = position * position;
                break;

            case AutomationShape::linear:
            case AutomationShape::hold:
                break;
        }

        // Tension bends the segment toward its start (+1) or end (-1), the
        // same convention FL's tension handles use.
        if (from.tension != 0.0f) {
            const float amount = std::clamp(from.tension, -1.0f, 1.0f);
            const float bent   = amount > 0.0f ? std::pow(position, 1.0f + 3.0f * amount)
                                               : 1.0f - std::pow(1.0f - position, 1.0f - 3.0f * amount);
            position = bent;
        }

        return from.value + (to.value - from.value) * position;
    }

private:
    std::vector<AutomationPoint> points_;
};

} // namespace incdaw::engine
