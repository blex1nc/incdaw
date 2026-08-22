#include "project/PatternCompiler.h"

#include <algorithm>
#include <cmath>

namespace incdaw::project {
namespace {

/// splitmix64, as used by humanize — the same generator throughout the project
/// layer, so that "deterministic for a seed" means one thing everywhere.
std::uint64_t nextRandom(std::uint64_t& state) noexcept
{
    state += 0x9E3779B97F4A7C15ull;
    std::uint64_t result = state;
    result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ull;
    result = (result ^ (result >> 27)) * 0x94D049BB133111EBull;
    return result ^ (result >> 31);
}

/// Folds several identifiers into one seed.
///
/// Placements need to roll probability independently — two copies of a pattern
/// with 50% notes should not fire in lockstep — while each one still rolls the
/// same way every time it is compiled.
std::uint64_t mixSeed(std::uint64_t seed, std::uint64_t a, std::uint64_t b) noexcept
{
    std::uint64_t state = seed ^ (a * 0x9E3779B97F4A7C15ull) ^ (b * 0xC2B2AE3D27D4EB4Full);
    return nextRandom(state);
}

/// Displacement swing applies to a note.
///
/// Off-beats — the odd multiples of the swing grid — are pushed later by up to
/// half a subdivision.
///
/// The rule is exact rather than approximate: a note swings only if it sits
/// precisely on an odd grid line. A tolerance window was tried first and
/// rejected — its width is unexplainable to a user, and it silently decides
/// that a note played 30 ms early was "meant" to be on the beat. Anything not
/// on the grid was placed expressively, and swing leaves it alone.
Tick swingOffset(Tick tick, Tick grid, double amount) noexcept
{
    if (grid <= 0 || amount <= 0.0 || tick < 0)
        return 0;

    if (tick % grid != 0 || (tick / grid) % 2 == 0)
        return 0;

    return static_cast<Tick>(std::llround(std::clamp(amount, 0.0, 1.0)
                                          * static_cast<double>(grid) * 0.5));
}

/// How many times a channel's content repeats inside its pattern.
Tick repeatCount(const PatternChannelContent& content, Tick patternLength) noexcept
{
    if (content.loopLength <= 0 || content.loopLength >= patternLength || patternLength <= 0)
        return 1;

    return (patternLength + content.loopLength - 1) / content.loopLength;
}

/// Appends one pattern's notes for one channel, offset by `tickOffset`.
///
/// `emit` decides what survives: the arrangement uses it to trim a placement to
/// its clip window, and pattern mode accepts everything.
template <typename Emit>
void expand(const Pattern& pattern, const PatternChannelContent& content,
            std::uint64_t seed, Emit&& emit)
{
    const Tick patternLength = pattern.length > 0 ? pattern.length : 0;
    const Tick repeats       = repeatCount(content, patternLength);
    const Tick period        = content.loopLength > 0 ? content.loopLength : patternLength;

    std::uint64_t state = seed;

    for (Tick repeat = 0; repeat < repeats; ++repeat) {
        const Tick base = repeat * period;

        for (const MidiEvent& event : content.events) {
            if (event.type != MidiEventType::note)
                continue;   // CC and pitch bend are not notes; they arrive with automation

            if (event.duration <= 0 || event.value <= 0)
                continue;

            Tick start = base + event.tick;

            // A repeat that would start past the end of the pattern is not part
            // of the pattern; polymetry loops within the bar, it does not
            // lengthen it.
            if (patternLength > 0 && start >= patternLength)
                continue;

            start += swingOffset(start, pattern.swingGrid, pattern.swing);

            // Probability is rolled here, off the audio thread, so that playback
            // and offline render agree. Rolling it in the audio callback would
            // make the two differ every time.
            const double roll = static_cast<double>(nextRandom(state) % 1000000ull) / 1000000.0;
            if (event.probability < 1.0 && roll >= event.probability)
                continue;

            engine::SequencedNote note;
            note.startTick   = start;
            note.lengthTicks = event.duration;
            note.channel     = event.channel;
            note.key         = event.key;
            note.velocity    = event.value;

            emit(note);
        }
    }
}


/// True when a clip on `track` should be heard.
///
/// A clip whose track has been deleted is silent rather than an error: the
/// arrangement can outlive a track for as long as an undo entry holds it.
bool isTrackAudible(const Project& project, const Track* track, bool anySoloed) noexcept
{
    if (track == nullptr)
        return false;

    // Folders propagate: a muted folder mutes everything under it, and a
    // soloed one lets its whole group through without flagging each child.
    if (trackEffectivelyMuted(project, *track))
        return false;

    return !anySoloed || trackEffectivelySoloed(project, *track);
}

} // namespace

std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern, EntityId channel,
                                                  std::uint64_t randomSeed)
{
    std::vector<engine::SequencedNote> notes;

    const PatternChannelContent* content = pattern.content(channel);
    if (content == nullptr)
        return notes;

    notes.reserve(content->events.size());
    expand(pattern, *content, randomSeed, [&notes](const engine::SequencedNote& note) {
        notes.push_back(note);
    });

    std::stable_sort(notes.begin(), notes.end(),
                     [](const engine::SequencedNote& a, const engine::SequencedNote& b) {
                         return a.startTick < b.startTick;
                     });
    return notes;
}

std::vector<engine::SequencedNote> compileArrangement(const Project& project, EntityId channel,
                                                      std::uint64_t randomSeed)
{
    std::vector<engine::SequencedNote> notes;

    // Track solo is exclusive across the arrangement, exactly as channel solo is
    // across the rack: the moment any track is soloed, every other one goes
    // quiet. Resolved here rather than on the audio thread, because a muted
    // track's notes are then simply never compiled.
    const bool anyTrackSoloed = std::any_of(project.tracks().begin(), project.tracks().end(),
                                            [](const Track& track) { return track.soloed; });

    for (const Clip& clip : project.clips()) {
        if (clip.type != ClipType::pattern || clip.muted)
            continue;

        if (!isTrackAudible(project, project.findTrack(clip.track), anyTrackSoloed))
            continue;

        const Pattern* pattern = project.findPattern(clip.source);
        if (pattern == nullptr || pattern->length <= 0)
            continue;

        const PatternChannelContent* content = pattern->content(channel);
        if (content == nullptr || content->events.empty())
            continue;

        const Tick window = clip.lengthTicks > 0 ? clip.lengthTicks : pattern->length;
        const Tick offset = std::max<Tick>(0, clip.sourceOffsetTicks);
        const Tick spans  = (offset + window + pattern->length - 1) / pattern->length;

        for (Tick repeat = 0; repeat < spans; ++repeat) {
            const Tick sourceBase = repeat * pattern->length;

            // Each repetition of each placement rolls its own probability, and
            // does so from a seed that depends only on the project — recompiling
            // gives the identical result, which is what makes an export match
            // what was heard.
            const auto seed = mixSeed(randomSeed, clip.id.value(),
                                      static_cast<std::uint64_t>(repeat));

            expand(*pattern, *content, seed, [&](engine::SequencedNote note) {
                const Tick sourceTick = sourceBase + note.startTick;
                if (sourceTick < offset || sourceTick >= offset + window)
                    return;

                note.startTick = clip.startTick + (sourceTick - offset);

                // A note running past the end of its clip is cut there. The clip
                // boundary is what the user drew; letting audio leak past it
                // would make trimming meaningless.
                const Tick remaining = clip.startTick + window - note.startTick;
                note.lengthTicks = std::min(note.lengthTicks, remaining);
                if (note.lengthTicks <= 0)
                    return;

                notes.push_back(note);
            });
        }
    }

    std::stable_sort(notes.begin(), notes.end(),
                     [](const engine::SequencedNote& a, const engine::SequencedNote& b) {
                         return a.startTick < b.startTick;
                     });
    return notes;
}

void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern, EntityId channel,
                        std::uint64_t randomSeed)
{
    sequence.setLoopLength(pattern.length);
    sequence.setNotes(compilePattern(pattern, channel, randomSeed));
}

void compileArrangementInto(engine::NoteSequence& sequence, const Project& project, EntityId channel,
                            std::uint64_t randomSeed)
{
    // No loop length: the arrangement runs to its own end, and the transport's
    // loop range decides what repeats.
    sequence.setLoopLength(0);
    sequence.setNotes(compileArrangement(project, channel, randomSeed));
}

Tick arrangementLengthTicks(const Project& project)
{
    Tick end = 0;

    // Muted and silenced clips still count towards the length. The song is as
    // long as the user drew it; unmuting a track at the end must not have
    // changed where the song ended.
    for (const Clip& clip : project.clips()) {
        if (clip.type != ClipType::pattern)
            continue;

        const Pattern* pattern = project.findPattern(clip.source);
        const Tick     window  = clip.lengthTicks > 0 ? clip.lengthTicks
                                                      : (pattern != nullptr ? pattern->length : 0);
        end = std::max(end, clip.startTick + window);
    }

    return end;
}

} // namespace incdaw::project
