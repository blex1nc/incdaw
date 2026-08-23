#include "app/AutomationEditorModel.h"

#include <algorithm>
#include <cmath>

namespace incdaw::app {
namespace {

/// The lane's points in the engine's own vocabulary, so that the curve the
/// editor draws is evaluated by the class the audio thread reads. The
/// translation is the compiler's, kept in step by being this short.
engine::AutomationSequence sequenceOf(const std::vector<AutomationPoint>& points)
{
    std::vector<engine::AutomationPoint> translated;
    translated.reserve(points.size());

    for (const AutomationPoint& point : points) {
        engine::AutomationPoint entry;
        entry.tick    = point.tick;
        entry.value   = static_cast<float>(point.value);
        entry.tension = static_cast<float>(point.tension);

        switch (point.curve) {
            case AutomationCurve::hold:        entry.shape = engine::AutomationShape::hold; break;
            case AutomationCurve::smooth:      entry.shape = engine::AutomationShape::smooth; break;
            case AutomationCurve::exponential: entry.shape = engine::AutomationShape::exponential; break;
            case AutomationCurve::linear:      entry.shape = engine::AutomationShape::linear; break;
        }

        translated.push_back(entry);
    }

    engine::AutomationSequence sequence;
    sequence.setPoints(std::move(translated));
    return sequence;
}

void sortByTick(std::vector<AutomationPoint>& points)
{
    std::stable_sort(points.begin(), points.end(),
                     [](const AutomationPoint& a, const AutomationPoint& b) {
                         return a.tick < b.tick;
                     });
}

/// Two points on one tick make a segment of zero length, which no evaluator
/// can read: the later one wins, because it is the one the gesture just put
/// there.
void dropDuplicateTicks(std::vector<AutomationPoint>& points)
{
    points.erase(std::unique(points.begin(), points.end(),
                             [](const AutomationPoint& a, const AutomationPoint& b) {
                                 return a.tick == b.tick;
                             }),
                 points.end());
}

std::vector<std::size_t> sortedUnique(const std::vector<std::size_t>& indices,
                                      std::size_t limit)
{
    std::vector<std::size_t> valid;
    valid.reserve(indices.size());

    for (const std::size_t index : indices)
        if (index < limit)
            valid.push_back(index);

    std::sort(valid.begin(), valid.end());
    valid.erase(std::unique(valid.begin(), valid.end()), valid.end());
    return valid;
}

} // namespace

// ── Viewport ─────────────────────────────────────────────────────────────────

void AutomationEditorModel::setViewport(const Viewport& viewport) noexcept
{
    viewport_ = viewport;

    viewport_.visibleTicks = std::max<Tick>(1, viewport_.visibleTicks);
    viewport_.firstTick    = std::max<Tick>(0, viewport_.firstTick);
    viewport_.width        = std::max(0.0, viewport_.width);
    viewport_.height       = std::max(0.0, viewport_.height);
    viewport_.rulerHeight  = std::clamp(viewport_.rulerHeight, 0.0, viewport_.height);
}

Tick AutomationEditorModel::xToTick(double x) const noexcept
{
    const double scale = pointsPerTick();
    if (scale <= 0.0)
        return viewport_.firstTick;

    return viewport_.firstTick + static_cast<Tick>(std::floor(x / scale));
}

double AutomationEditorModel::valueToY(double value) const noexcept
{
    const double clamped = std::clamp(value, 0.0, 1.0);
    return curveTop() + (1.0 - clamped) * curveHeight();
}

double AutomationEditorModel::yToValue(double y) const noexcept
{
    const double height = curveHeight();
    if (height <= 0.0)
        return 0.0;

    return std::clamp(1.0 - (y - curveTop()) / height, 0.0, 1.0);
}

AutomationEditorModel::Rect AutomationEditorModel::pointRect(
    const AutomationPoint& point) const noexcept
{
    const double x = tickToX(point.tick);
    const double y = valueToY(point.value);

    return {x - handleRadius, y - handleRadius, handleRadius * 2.0, handleRadius * 2.0};
}

// ── The curve ────────────────────────────────────────────────────────────────

double AutomationEditorModel::valueAt(const std::vector<AutomationPoint>& points, Tick tick)
{
    if (points.empty())
        return 0.0;

    return static_cast<double>(sequenceOf(points).valueAt(tick));
}

void AutomationEditorModel::collectCurve(const std::vector<AutomationPoint>& points,
                                         std::vector<double>& out, double xStep) const
{
    out.clear();

    if (points.empty() || viewport_.width <= 0.0 || xStep <= 0.0)
        return;

    // Built once for the whole sweep rather than per sample: the sequence sorts
    // on construction, and rebuilding it per pixel would make drawing a lane
    // quadratic in its point count.
    const engine::AutomationSequence sequence = sequenceOf(points);

    out.reserve(static_cast<std::size_t>(viewport_.width / xStep) + 2);

    for (double x = 0.0; x < viewport_.width + xStep; x += xStep)
        out.push_back(static_cast<double>(sequence.valueAt(xToTick(std::min(x, viewport_.width)))));
}

// ── Hit testing ──────────────────────────────────────────────────────────────

std::size_t AutomationEditorModel::pointAt(const std::vector<AutomationPoint>& points,
                                           double x, double y) const noexcept
{
    std::size_t best         = noPoint;
    double      bestDistance = 0.0;

    for (std::size_t index = 0; index < points.size(); ++index) {
        const Rect rect = pointRect(points[index]);
        if (!rect.contains(x, y))
            continue;

        // Nearest centre wins where two handles overlap, rather than the first
        // or last in the list — which is what the pointer is actually aiming
        // at when points crowd together.
        const double dx       = x - (rect.x + handleRadius);
        const double dy       = y - (rect.y + handleRadius);
        const double distance = dx * dx + dy * dy;

        if (best == noPoint || distance < bestDistance) {
            best         = index;
            bestDistance = distance;
        }
    }

    return best;
}

std::size_t AutomationEditorModel::segmentAt(const std::vector<AutomationPoint>& points,
                                             double x) const noexcept
{
    if (points.size() < 2)
        return noPoint;

    const Tick tick = xToTick(x);

    if (tick < points.front().tick || tick >= points.back().tick)
        return noPoint;

    for (std::size_t index = 0; index + 1 < points.size(); ++index)
        if (tick >= points[index].tick && tick < points[index + 1].tick)
            return index;

    return noPoint;
}

void AutomationEditorModel::pointsInRectangle(const std::vector<AutomationPoint>& points,
                                              double x, double y, double width, double height,
                                              std::vector<std::size_t>& out) const
{
    out.clear();

    const double left   = std::min(x, x + width);
    const double right  = std::max(x, x + width);
    const double top    = std::min(y, y + height);
    const double bottom = std::max(y, y + height);

    for (std::size_t index = 0; index < points.size(); ++index) {
        const double pointX = tickToX(points[index].tick);
        const double pointY = valueToY(points[index].value);

        if (pointX >= left && pointX <= right && pointY >= top && pointY <= bottom)
            out.push_back(index);
    }
}

// ── Grid ─────────────────────────────────────────────────────────────────────

Tick AutomationEditorModel::snapTick(Tick tick) const noexcept
{
    if (snap_ <= 0)
        return tick;

    const Tick remainder = tick % snap_;
    const Tick down      = tick - remainder;

    return remainder * 2 >= snap_ ? down + snap_ : down;
}

// ── Selection ────────────────────────────────────────────────────────────────

void AutomationEditorModel::setSelection(std::vector<std::size_t> points)
{
    selection_ = std::move(points);
    std::sort(selection_.begin(), selection_.end());
    selection_.erase(std::unique(selection_.begin(), selection_.end()), selection_.end());
}

void AutomationEditorModel::addToSelection(std::size_t index)
{
    if (!isSelected(index))
        selection_.insert(std::upper_bound(selection_.begin(), selection_.end(), index), index);
}

void AutomationEditorModel::toggleSelection(std::size_t index)
{
    const auto found = std::find(selection_.begin(), selection_.end(), index);

    if (found == selection_.end())
        addToSelection(index);
    else
        selection_.erase(found);
}

bool AutomationEditorModel::isSelected(std::size_t index) const noexcept
{
    return std::binary_search(selection_.begin(), selection_.end(), index);
}

void AutomationEditorModel::pruneSelection(std::size_t pointCount)
{
    selection_.erase(std::remove_if(selection_.begin(), selection_.end(),
                                    [pointCount](const std::size_t index) {
                                        return index >= pointCount;
                                    }),
                     selection_.end());
}

// ── Edits ────────────────────────────────────────────────────────────────────

std::vector<AutomationPoint> AutomationEditorModel::withPointAdded(
    std::vector<AutomationPoint> points, Tick tick, double value, AutomationCurve curve)
{
    AutomationPoint added;
    added.tick  = std::max<Tick>(0, tick);
    added.value = std::clamp(value, 0.0, 1.0);
    added.curve = curve;

    // The new point wins its tick, so drawing over an existing one replaces it
    // rather than stacking on it.
    points.erase(std::remove_if(points.begin(), points.end(),
                                [tick = added.tick](const AutomationPoint& point) {
                                    return point.tick == tick;
                                }),
                 points.end());

    points.push_back(added);
    sortByTick(points);
    return points;
}

std::vector<AutomationPoint> AutomationEditorModel::withPointsRemoved(
    std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices)
{
    const auto valid = sortedUnique(indices, points.size());

    for (auto index = valid.rbegin(); index != valid.rend(); ++index)
        points.erase(points.begin() + static_cast<std::ptrdiff_t>(*index));

    return points;
}

std::vector<AutomationPoint> AutomationEditorModel::withPointsMoved(
    std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
    Tick tickDelta, double valueDelta)
{
    const auto valid = sortedUnique(indices, points.size());
    if (valid.empty())
        return points;

    // Clamped once for the whole set: a dragged group that hits tick zero or
    // the top of the lane keeps its shape, exactly as a dragged group of clips
    // does.
    Tick   tick  = tickDelta;
    double value = valueDelta;

    for (const std::size_t index : valid) {
        tick  = std::max(tick, -points[index].tick);
        value = std::clamp(value, -points[index].value, 1.0 - points[index].value);
    }

    for (const std::size_t index : valid) {
        points[index].tick  = std::max<Tick>(0, points[index].tick + tick);
        points[index].value = std::clamp(points[index].value + value, 0.0, 1.0);
    }

    sortByTick(points);
    dropDuplicateTicks(points);
    return points;
}

std::vector<AutomationPoint> AutomationEditorModel::withCurve(
    std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
    AutomationCurve curve)
{
    for (const std::size_t index : sortedUnique(indices, points.size()))
        points[index].curve = curve;

    return points;
}

std::vector<AutomationPoint> AutomationEditorModel::withTension(
    std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
    double tension)
{
    const double clamped = std::clamp(tension, -1.0, 1.0);

    for (const std::size_t index : sortedUnique(indices, points.size()))
        points[index].tension = clamped;

    return points;
}

std::vector<AutomationPoint> AutomationEditorModel::withTimeScaled(
    std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
    double factor, Tick anchorTick)
{
    const auto valid = sortedUnique(indices, points.size());
    if (valid.empty() || factor <= 0.0)
        return points;

    for (const std::size_t index : valid) {
        const auto offset = static_cast<double>(points[index].tick - anchorTick);
        points[index].tick =
            std::max<Tick>(0, anchorTick + static_cast<Tick>(std::llround(offset * factor)));
    }

    sortByTick(points);
    dropDuplicateTicks(points);
    return points;
}

std::vector<AutomationPoint> AutomationEditorModel::withValueScaled(
    std::vector<AutomationPoint> points, const std::vector<std::size_t>& indices,
    double factor, double anchorValue)
{
    for (const std::size_t index : sortedUnique(indices, points.size())) {
        const double offset = points[index].value - anchorValue;
        points[index].value = std::clamp(anchorValue + offset * factor, 0.0, 1.0);
    }

    return points;
}

std::vector<AutomationPoint> AutomationEditorModel::copyOf(
    const std::vector<AutomationPoint>& points, const std::vector<std::size_t>& indices)
{
    const auto valid = sortedUnique(indices, points.size());

    std::vector<AutomationPoint> copied;
    copied.reserve(valid.size());

    for (const std::size_t index : valid)
        copied.push_back(points[index]);

    if (copied.empty())
        return copied;

    // Rebased to zero so a paste is a translation and nothing else.
    const Tick origin = copied.front().tick;
    for (AutomationPoint& point : copied)
        point.tick -= origin;

    return copied;
}

std::vector<AutomationPoint> AutomationEditorModel::withPasted(
    std::vector<AutomationPoint> points, const std::vector<AutomationPoint>& clipboard,
    Tick atTick)
{
    if (clipboard.empty())
        return points;

    const Tick at = std::max<Tick>(0, atTick);

    for (const AutomationPoint& source : clipboard) {
        AutomationPoint landed = source;
        landed.tick = std::max<Tick>(0, at + source.tick);

        points.erase(std::remove_if(points.begin(), points.end(),
                                    [tick = landed.tick](const AutomationPoint& point) {
                                        return point.tick == tick;
                                    }),
                     points.end());

        points.push_back(landed);
    }

    sortByTick(points);
    return points;
}

std::vector<std::size_t> AutomationEditorModel::pastedIndices(
    const std::vector<AutomationPoint>& points, const std::vector<AutomationPoint>& clipboard,
    Tick atTick)
{
    std::vector<std::size_t> landed;
    if (clipboard.empty())
        return landed;

    const Tick at = std::max<Tick>(0, atTick);

    for (const AutomationPoint& source : clipboard) {
        const Tick tick = std::max<Tick>(0, at + source.tick);

        for (std::size_t index = 0; index < points.size(); ++index)
            if (points[index].tick == tick)
                landed.push_back(index);
    }

    std::sort(landed.begin(), landed.end());
    landed.erase(std::unique(landed.begin(), landed.end()), landed.end());
    return landed;
}

} // namespace incdaw::app
