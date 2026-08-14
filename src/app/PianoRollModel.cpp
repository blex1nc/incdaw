#include "app/PianoRollModel.h"

#include <algorithm>

namespace incdaw::app {

void PianoRollModel::collectVisibleNotes(const Pattern& pattern, std::vector<VisibleNote>& out) const
{
    // Cleared, not reassigned: the capacity earned on previous frames is what
    // keeps a steady-state frame allocation-free.
    out.clear();

    const double scale  = pointsPerTick();
    const double height = keyHeight();

    if (scale <= 0.0 || height <= 0.0)
        return;

    const Tick lastTick   = viewport_.firstTick + viewport_.visibleTicks;
    const int  highestKey = viewport_.lowestKey + viewport_.visibleKeys - 1;

    for (std::size_t index = 0; index < pattern.events.size(); ++index) {
        const MidiEvent& event = pattern.events[index];

        if (event.type != project::MidiEventType::note)
            continue;

        if (event.key < viewport_.lowestKey || event.key > highestKey)
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

std::size_t PianoRollModel::noteAtPoint(const Pattern& pattern, double x, double y) const
{
    const double scale  = pointsPerTick();
    const double height = keyHeight();

    if (scale <= 0.0 || height <= 0.0)
        return noNote;

    const int  key  = yToKey(y);
    const Tick tick = viewport_.firstTick + static_cast<Tick>(x / scale);

    // Back to front: the note drawn last sits on top where they overlap, and
    // that is the one the user is pointing at.
    for (std::size_t position = pattern.events.size(); position > 0; --position) {
        const std::size_t index = position - 1;
        const MidiEvent&  event = pattern.events[index];

        if (event.type != project::MidiEventType::note || event.key != key)
            continue;

        const Tick noteEnd = event.tick + std::max<Tick>(1, event.duration);
        if (tick >= event.tick && tick < noteEnd)
            return index;
    }

    return noNote;
}

bool PianoRollModel::isOverResizeHandle(const Pattern& pattern, std::size_t index,
                                        double x, double y) const
{
    if (index >= pattern.events.size())
        return false;

    const MidiEvent& event = pattern.events[index];
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

void PianoRollModel::notesInRectangle(const Pattern& pattern, double x, double y,
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

    for (std::size_t index = 0; index < pattern.events.size(); ++index) {
        const MidiEvent& event = pattern.events[index];
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
