#include "app/PianoRollModel.h"

#include <algorithm>

namespace incdaw::app {

namespace {

/// The key range on screen, as a closed interval.
bool keyIsVisible(const MidiEvent& event, const PianoRollModel::Viewport& viewport) noexcept
{
    return event.key >= viewport.lowestKey
        && event.key <= viewport.lowestKey + viewport.visibleKeys - 1;
}

} // namespace

void PianoRollModel::collectVisibleNotes(const NoteList& notes, std::vector<VisibleNote>& out) const
{
    // Cleared, not reassigned: the capacity earned on previous frames is what
    // keeps a steady-state frame allocation-free.
    out.clear();

    const double scale  = pointsPerTick();
    const double height = keyHeight();

    if (scale <= 0.0 || height <= 0.0)
        return;

    const Tick lastTick = viewport_.firstTick + viewport_.visibleTicks;

    for (std::size_t index = 0; index < notes.size(); ++index) {
        const MidiEvent& event = notes[index];

        if (event.type != project::MidiEventType::note)
            continue;

        if (!keyIsVisible(event, viewport_))
            continue;

        // A note is visible if any part of it is: a long note starting before
        // the viewport still has to be drawn, and dropping it would make notes
        // vanish as you scroll into them.
        const Tick noteEnd = event.tick + std::max<Tick>(1, event.duration);
        if (noteEnd <= viewport_.firstTick || event.tick >= lastTick)
            continue;

        VisibleNote visible;
        visible.index    = index;
        visible.x        = tickToX(event.tick);
        visible.y        = keyToY(event.key);
        visible.width    = static_cast<double>(std::max<Tick>(1, event.duration)) * scale;
        visible.height   = height;
        visible.key      = event.key;
        visible.velocity = event.value;
        visible.selected = isSelected(index);

        out.push_back(visible);
    }
}

// ── Velocity lane ────────────────────────────────────────────────────────────

namespace {

/// Velocity spans 1..127; 0 is note-off and never appears on a note in a
/// pattern (app/commands/NoteCommands.cpp clamps it for the same reason).
constexpr int minimumVelocity = 1;
constexpr int maximumVelocity = 127;

} // namespace

double PianoRollModel::velocityToY(int velocity) const noexcept
{
    // max(1) rather than a branch on a degenerate lane: the mapping stays
    // monotonic and finite at any height, and `hasVelocityLane` is what
    // decides whether the lane exists at all.
    const double usable = std::max(1.0, viewport_.velocityLaneHeight - velocityLanePadding);
    const double scaled = static_cast<double>(std::clamp(velocity, minimumVelocity, maximumVelocity))
                        / static_cast<double>(maximumVelocity);

    return velocityLaneBottom() - usable * scaled;
}

int PianoRollModel::yToVelocity(double y) const noexcept
{
    const double usable = std::max(1.0, viewport_.velocityLaneHeight - velocityLanePadding);
    const double filled = (velocityLaneBottom() - y) / usable;

    const auto velocity = static_cast<int>(filled * static_cast<double>(maximumVelocity) + 0.5);
    return std::clamp(velocity, minimumVelocity, maximumVelocity);
}

void PianoRollModel::collectVelocityBars(const NoteList& notes, std::vector<VelocityBar>& out) const
{
    out.clear();

    if (!hasVelocityLane() || pointsPerTick() <= 0.0)
        return;

    const Tick   lastTick = viewport_.firstTick + viewport_.visibleTicks;
    const double floorY   = velocityLaneBottom();

    for (std::size_t index = 0; index < notes.size(); ++index) {
        const MidiEvent& event = notes[index];

        if (event.type != project::MidiEventType::note)
            continue;

        if (!keyIsVisible(event, viewport_))
            continue;

        if (event.tick < viewport_.firstTick || event.tick >= lastTick)
            continue;

        VelocityBar bar;
        bar.index    = index;
        bar.x        = tickToX(event.tick);
        bar.width    = velocityBarWidth;
        bar.top      = velocityToY(event.value);
        bar.height   = floorY - bar.top;
        bar.velocity = std::clamp(event.value, minimumVelocity, maximumVelocity);
        bar.selected = isSelected(index);

        out.push_back(bar);
    }
}

std::size_t PianoRollModel::barAtPoint(const NoteList& notes, double x, double y) const
{
    if (!isInVelocityLane(y) || pointsPerTick() <= 0.0)
        return noNote;

    const Tick lastTick = viewport_.firstTick + viewport_.visibleTicks;

    // Back to front, like noteAtPoint: two notes starting on the same tick put
    // their bars in the same column, and the one drawn last is on top.
    //
    // The whole column is the target, not just the filled part of it. A bar at
    // velocity 20 is a few points tall, and requiring the user to hit those few
    // points would make quiet notes the hardest ones to make louder.
    for (std::size_t position = notes.size(); position > 0; --position) {
        const std::size_t index = position - 1;
        const MidiEvent&  event = notes[index];

        if (event.type != project::MidiEventType::note)
            continue;

        if (!keyIsVisible(event, viewport_))
            continue;

        if (event.tick < viewport_.firstTick || event.tick >= lastTick)
            continue;

        const double left = tickToX(event.tick);
        if (x >= left && x < left + velocityBarWidth)
            return index;
    }

    return noNote;
}

std::size_t PianoRollModel::noteAtPoint(const NoteList& notes, double x, double y) const
{
    const double scale  = pointsPerTick();
    const double height = keyHeight();

    if (scale <= 0.0 || height <= 0.0)
        return noNote;

    const int  key  = yToKey(y);
    const Tick tick = viewport_.firstTick + static_cast<Tick>(x / scale);

    // Back to front: the note drawn last sits on top where they overlap, and
    // that is the one the user is pointing at.
    for (std::size_t position = notes.size(); position > 0; --position) {
        const std::size_t index = position - 1;
        const MidiEvent&  event = notes[index];

        if (event.type != project::MidiEventType::note || event.key != key)
            continue;

        const Tick noteEnd = event.tick + std::max<Tick>(1, event.duration);
        if (tick >= event.tick && tick < noteEnd)
            return index;
    }

    return noNote;
}

bool PianoRollModel::isOverResizeHandle(const NoteList& notes, std::size_t index,
                                        double x, double y) const
{
    if (index >= notes.size())
        return false;

    const MidiEvent& event = notes[index];
    if (event.type != project::MidiEventType::note)
        return false;

    const double noteRight = tickToX(event.tick + std::max<Tick>(1, event.duration));
    const double noteLeft  = tickToX(event.tick);

    if (yToKey(y) != event.key)
        return false;

    // On a very short note the handle would cover the whole thing, leaving no
    // way to move it. Half the width keeps both gestures reachable.
    const double handle = std::min(resizeHandleWidth, (noteRight - noteLeft) * 0.5);

    return x >= noteRight - handle && x <= noteRight;
}

void PianoRollModel::notesInRectangle(const NoteList& notes, double x, double y,
                                      double width, double height,
                                      std::vector<std::size_t>& out) const
{
    out.clear();

    // A rectangle dragged up or to the left arrives with negative extents;
    // normalising here means every caller does not have to.
    if (width < 0.0) {
        x += width;
        width = -width;
    }

    if (height < 0.0) {
        y += height;
        height = -height;
    }

    const double scale   = pointsPerTick();
    const double rowSize = keyHeight();

    if (scale <= 0.0 || rowSize <= 0.0)
        return;

    for (std::size_t index = 0; index < notes.size(); ++index) {
        const MidiEvent& event = notes[index];
        if (event.type != project::MidiEventType::note)
            continue;

        const double noteLeft   = tickToX(event.tick);
        const double noteRight  = tickToX(event.tick + std::max<Tick>(1, event.duration));
        const double noteTop    = keyToY(event.key);
        const double noteBottom = noteTop + rowSize;

        // Intersection, not containment: a box selection that only caught notes
        // entirely inside it would feel broken on long notes.
        if (noteRight > x && noteLeft < x + width && noteBottom > y && noteTop < y + height)
            out.push_back(index);
    }
}

Tick PianoRollModel::snapTick(Tick tick) const noexcept
{
    if (snap_ <= 0)
        return tick;

    // Floor-based remainder so that negative ticks snap consistently.
    const Tick remainder = ((tick % snap_) + snap_) % snap_;
    return remainder * 2 >= snap_ ? tick - remainder + snap_ : tick - remainder;
}

void PianoRollModel::setSelection(std::vector<std::size_t> indices)
{
    std::sort(indices.begin(), indices.end());
    indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
    selection_ = std::move(indices);
}

void PianoRollModel::addToSelection(std::size_t index)
{
    const auto position = std::lower_bound(selection_.begin(), selection_.end(), index);
    if (position == selection_.end() || *position != index)
        selection_.insert(position, index);
}

void PianoRollModel::toggleSelection(std::size_t index)
{
    const auto position = std::lower_bound(selection_.begin(), selection_.end(), index);

    if (position != selection_.end() && *position == index)
        selection_.erase(position);
    else
        selection_.insert(position, index);
}

bool PianoRollModel::isSelected(std::size_t index) const noexcept
{
    return std::binary_search(selection_.begin(), selection_.end(), index);
}

void PianoRollModel::pruneSelection(std::size_t noteCount)
{
    selection_.erase(std::remove_if(selection_.begin(), selection_.end(),
                                    [noteCount](std::size_t index) { return index >= noteCount; }),
                     selection_.end());
}

} // namespace incdaw::app
