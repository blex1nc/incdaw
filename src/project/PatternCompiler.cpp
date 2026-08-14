#include "project/PatternCompiler.h"

#include <algorithm>

namespace incdaw::project {
namespace {

/// splitmix64, as used by humanize — same generator so that "deterministic for
/// a seed" means the same thing everywhere in the project layer.
std::uint64_t nextRandom(std::uint64_t& state) noexcept
{
    state += 0x9E3779B97F4A7C15ull;
    std::uint64_t result = state;
    result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9ull;
    result = (result ^ (result >> 27)) * 0x94D049BB133111EBull;
    return result ^ (result >> 31);
}

/// Delays the off-beat steps of the grid.
///
/// Swing is applied here rather than written into the notes so that turning it
/// off restores the original timing exactly — a destructive shuffle is a shuffle
/// the user can never fully undo.
[[nodiscard]] Tick applySwing(Tick tick, Tick stepDivision, double swing) noexcept
{
    if (swing <= 0.0 || stepDivision <= 1)
        return tick;

    const Tick step = tick / stepDivision;
    if ((step % 2) == 0)
        return tick;   // on-beat steps never move

    const auto shift = static_cast<Tick>(std::clamp(swing, 0.0, 1.0)
                                         * static_cast<double>(stepDivision) * 0.5);
    return tick + shift;
}

/// True when this note is played at all. Rolled once per emitted instance, so a
/// repeated step can differ between repeats while the whole sequence stays
/// reproducible for a given seed.
[[nodiscard]] bool passesProbability(const MidiEvent& event, std::uint64_t& state) noexcept
{
    if (event.probability >= 1.0)
        return true;

    if (event.probability <= 0.0)
        return false;

    const double roll = static_cast<double>(nextRandom(state) % 1000000ull) / 1000000.0;
    return roll < event.probability;
}

[[nodiscard]] bool playsOnChannel(const MidiEvent& event, EntityId channel, EntityId defaultChannel) noexcept
{
    if (!channel.isValid())
        return true;   // no filter: every note in the pattern

    const EntityId owner = event.channelId.isValid() ? event.channelId : defaultChannel;
    return owner == channel;
}

} // namespace

std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern,
                                                  const PatternCompileOptions& options)
{
    std::vector<engine::SequencedNote> notes;
    notes.reserve(pattern.events.size());

    if (pattern.length <= 0)
        return notes;

    // A channel may loop at a shorter length than the pattern it lives in.
    // That is what makes a pattern polymetric: a 3-step hi-hat against a 4-step
    // kick repeats every three steps and drifts across the bar, exactly as it
    // does on hardware step sequencers.
    const Tick channelLength = pattern.lengthFor(options.channel);
    const double swing       = pattern.swingFor(options.channel);

    const Tick span   = options.span > 0 ? options.span : pattern.length;
    const Tick offset = std::max<Tick>(0, options.sourceOffset);
    const Tick end    = offset + span;

    // More than one cycle to fill means the content repeats, and repeated
    // content has to be confined or it collides with its own next repetition.
    const bool bounded = options.bounded || end > channelLength;

    std::uint64_t state = options.randomSeed;

    // `cycle` walks the pattern's own repeats inside the requested span; a
    // clip stretched over four bars replays a one-bar pattern four times.
    for (Tick cycle = 0; cycle < end; cycle += channelLength) {
        for (const MidiEvent& event : pattern.events) {
            if (event.type != MidiEventType::note)
                continue;   // CC and pitch bend are not notes; they arrive with automation

            if (event.duration <= 0 || event.value <= 0)
                continue;

            if (!playsOnChannel(event, options.channel, options.defaultChannel))
                continue;

            if (event.tick < 0)
                continue;

            if (bounded && event.tick >= channelLength)
                continue;   // outside this channel's loop; it would collide with the repeat

            if (!passesProbability(event, state))
                continue;

            const Tick swung    = applySwing(event.tick, pattern.stepDivision, swing);
            const Tick absolute = cycle + swung;

            if (absolute < offset || (bounded && absolute >= end))
                continue;

            engine::SequencedNote note;
            note.startTick   = options.startTick + (absolute - offset);
            note.lengthTicks = event.duration;
            note.channel     = event.channel;
            note.key         = event.key;
            note.velocity    = event.value;

            // A note is not allowed to ring past the span it was placed in:
            // the next placement would then overlap it on the same key, and
            // the note-off of the first would silence the second.
            if (bounded) {
                const Tick remaining = span - (absolute - offset);
                note.lengthTicks     = std::min(note.lengthTicks, std::max<Tick>(1, remaining));
            }

            notes.push_back(note);
        }
    }

    return notes;
}

std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern, std::uint64_t randomSeed)
{
    PatternCompileOptions options;
    options.randomSeed = randomSeed;
    return compilePattern(pattern, options);
}

void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern, std::uint64_t randomSeed)
{
    PatternCompileOptions options;
    options.randomSeed = randomSeed;
    compilePatternInto(sequence, pattern, options);
}

void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern,
                        const PatternCompileOptions& options)
{
    sequence.setLoopLength(options.span > 0 ? options.span : pattern.length);
    sequence.setNotes(compilePattern(pattern, options));
}

std::vector<engine::SequencedNote> compileArrangement(const Project& project, EntityId channel,
                                                      std::uint64_t randomSeed)
{
    std::vector<engine::SequencedNote> notes;

    const engine::TempoMap& tempoMap = project.tempoMap();

    for (const Clip& clip : project.clips()) {
        if (clip.type != ClipType::pattern || clip.muted)
            continue;

        const Pattern* pattern = project.findPattern(clip.source);
        if (pattern == nullptr || pattern->length <= 0)
            continue;

        PatternCompileOptions options;
        options.channel        = channel;
        options.defaultChannel = project.defaultChannel();
        options.startTick      = tempoMap.tickForFrame(clip.start);
        options.sourceOffset   = tempoMap.tickForFrame(clip.sourceOffset);

        options.span = clip.length > 0
                           ? tempoMap.tickForFrame(clip.start + clip.length) - options.startTick
                           : pattern->length;

        // Every placement of a pattern is seeded from the pattern, never from
        // the clip. That is what makes two placements of one pattern play
        // identically — the defining property of the pattern workflow, and the
        // exit criterion for this phase.
        options.randomSeed = randomSeed ^ pattern->id.value();
        options.bounded    = true;   // a clip owns exactly its span of the timeline

        const std::vector<engine::SequencedNote> placed = compilePattern(*pattern, options);
        notes.insert(notes.end(), placed.begin(), placed.end());
    }

    return notes;
}

void compileArrangementInto(engine::NoteSequence& sequence, const Project& project,
                            EntityId channel, std::uint64_t randomSeed)
{
    sequence.setLoopLength(0);
    sequence.setNotes(compileArrangement(project, channel, randomSeed));
}

Tick arrangementLengthTicks(const Project& project)
{
    Tick end = 0;

    for (const Clip& clip : project.clips()) {
        if (clip.type != ClipType::pattern)
            continue;

        const Pattern* pattern = project.findPattern(clip.source);
        if (pattern == nullptr)
            continue;

        const Tick start  = project.tempoMap().tickForFrame(clip.start);
        const Tick length = clip.length > 0
                                ? project.tempoMap().tickForFrame(clip.start + clip.length) - start
                                : pattern->length;

        end = std::max(end, start + length);
    }

    return end;
}

} // namespace incdaw::project
