#include "project/MidiFile.h"

#include "engine/midi/NoteSequence.h"
#include "project/PatternCompiler.h"

#include <algorithm>

namespace incdaw::project {

engine::SmfFile::Result exportArrangement(const Project& project,
                                          const std::filesystem::path& path,
                                          std::uint64_t randomSeed)
{
    engine::SmfDocument document;

    document.tempo          = project.tempoMap().tempoEvents();
    document.timeSignatures = project.tempoMap().timeSignatureEvents();

    for (const Channel& channel : project.channels()) {
        engine::NoteSequence sequence;
        compileArrangementInto(sequence, project, channel.id, randomSeed);

        if (sequence.noteCount() == 0)
            continue;

        engine::SmfTrack track;
        track.name = channel.name;
        track.notes.reserve(sequence.noteCount());

        for (const engine::SequencedNote& note : sequence.notes()) {
            engine::SmfNote out;
            out.startTick   = note.startTick;
            out.lengthTicks = note.lengthTicks;
            out.channel     = note.channel;
            out.key         = note.key;
            out.velocity    = note.velocity;
            track.notes.push_back(out);
        }

        document.tracks.push_back(std::move(track));
    }

    return engine::SmfFile::write(path, document);
}

MidiImportResult importAsPattern(Project& project, const std::filesystem::path& path)
{
    MidiImportResult result;

    engine::SmfDocument document;
    if (const auto read = engine::SmfFile::read(path, document); !read) {
        result.error = read.error;
        return result;
    }

    if (document.tracks.empty()) {
        result.error = "the MIDI file holds no notes";
        return result;
    }

    Pattern& pattern = project.addPattern(path.stem().string());
    result.pattern   = pattern.id;

    engine::Tick longest = 0;

    for (const engine::SmfTrack& track : document.tracks) {
        Channel& channel =
            project.addChannel(track.name.empty() ? "Imported" : track.name);
        result.newChannels.push_back(channel.id);

        // Refetched by id each round: entities are addressed by id, never by
        // a reference held across mutations (docs/ARCHITECTURE.md §5).
        Pattern* target = project.findPattern(result.pattern);

        auto& content = target->contentFor(channel.id);
        content.events.reserve(track.notes.size());

        for (const engine::SmfNote& note : track.notes) {
            MidiEvent event;
            event.type     = MidiEventType::note;
            event.tick     = note.startTick;
            event.duration = note.lengthTicks;
            event.key      = note.key;
            event.value    = std::clamp(note.velocity, 1, 127);
            content.events.push_back(event);

            longest = std::max(longest, note.startTick + note.lengthTicks);
        }
    }

    // The pattern is as long as its content, rounded up to a whole bar of
    // quarters so it loops musically.
    Pattern* target = project.findPattern(result.pattern);
    const engine::Tick bar = engine::ticksPerQuarterNote * 4;
    target->length = std::max<engine::Tick>(bar, ((longest + bar - 1) / bar) * bar);

    result.succeeded = true;
    return result;
}

} // namespace incdaw::project
