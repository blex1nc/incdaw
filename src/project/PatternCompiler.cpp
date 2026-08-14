#include "project/PatternCompiler.h"

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

} // namespace

std::vector<engine::SequencedNote> compilePattern(const Pattern& pattern, std::uint64_t randomSeed)
{
    std::vector<engine::SequencedNote> notes;
    notes.reserve(pattern.events.size());

    std::uint64_t state = randomSeed;

    for (const MidiEvent& event : pattern.events) {
        if (event.type != MidiEventType::note)
            continue;   // CC and pitch bend are not notes; they arrive with automation

        if (event.duration <= 0 || event.value <= 0)
            continue;

        // Probability is evaluated here, deterministically, so that playback and
        // offline render agree. Evaluating it in the audio thread would make the
        // two differ every time.
        if (event.probability < 1.0) {
            const double roll = static_cast<double>(nextRandom(state) % 1000000ull) / 1000000.0;
            if (roll >= event.probability)
                continue;
        }

        engine::SequencedNote note;
        note.startTick   = event.tick;
        note.lengthTicks = event.duration;
        note.channel     = event.channel;
        note.key         = event.key;
        note.velocity    = event.value;

        notes.push_back(note);
    }

    return notes;
}

void compilePatternInto(engine::NoteSequence& sequence, const Pattern& pattern, std::uint64_t randomSeed)
{
    sequence.setLoopLength(pattern.length);
    sequence.setNotes(compilePattern(pattern, randomSeed));
}

} // namespace incdaw::project
