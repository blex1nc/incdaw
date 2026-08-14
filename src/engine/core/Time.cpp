#include "engine/core/Time.h"

namespace incdaw::engine {

MusicalPosition ticksToMusicalPosition(Tick ticks, TimeSignature signature) noexcept
{
    MusicalPosition position;

    if (!signature.isValid())
        return position;

    const Tick perBeat = signature.ticksPerBeat();
    const Tick perBar  = signature.ticksPerBar();

    if (perBeat <= 0 || perBar <= 0)
        return position;

    // Floor division, so that negative positions (pre-roll, count-in) walk
    // backwards through bars rather than collapsing onto bar 1.
    Tick bars = ticks / perBar;
    Tick rest = ticks % perBar;

    if (rest < 0) {
        rest += perBar;
        --bars;
    }

    position.bar  = bars + 1;
    position.beat = static_cast<int>(rest / perBeat) + 1;
    position.tick = rest % perBeat;

    return position;
}

Tick musicalPositionToTicks(MusicalPosition position, TimeSignature signature) noexcept
{
    if (!signature.isValid())
        return 0;

    return (position.bar - 1) * signature.ticksPerBar()
         + static_cast<Tick>(position.beat - 1) * signature.ticksPerBeat()
         + position.tick;
}

} // namespace incdaw::engine
