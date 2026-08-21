// Tempo and time signature as project edits.
//
// Until these existed the tempo map could only be set by loading a project:
// the model carried it, the engine honoured it, and nothing could change it.
// The properties that matter are that an edit is reversible, that it leaves
// later tempo changes alone, and that a hostile value cannot produce a timeline
// that never advances.

#include "doctest.h"

#include "app/CommandRegistry.h"
#include "app/commands/TempoCommands.h"
#include "project/Model.h"

#include <memory>

using namespace incdaw;
using namespace incdaw::app;

TEST_CASE("setting the tempo is reversible")
{
    project::Project project;
    CommandRegistry  registry{project};

    const double original = project.tempoMap().tempoAtTick(0);

    REQUIRE(registry.execute(std::make_unique<SetProjectTempoCommand>(174.0)));
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(174.0));

    CHECK(registry.undo());
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(original));

    CHECK(registry.redo());
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(174.0));
}

TEST_CASE("setting the same tempo is a no-op, not an undo entry")
{
    project::Project project;
    CommandRegistry  registry{project};

    const double current = project.tempoMap().tempoAtTick(0);

    CHECK_FALSE(registry.execute(std::make_unique<SetProjectTempoCommand>(current)));
    CHECK(registry.undoDepth() == 0);
}

TEST_CASE("a tempo drag is one undo entry")
{
    project::Project project;
    CommandRegistry  registry{project};

    const double original = project.tempoMap().tempoAtTick(0);

    for (double tempo = 121.0; tempo <= 140.0; tempo += 1.0)
        CHECK(registry.executeMerging(std::make_unique<SetProjectTempoCommand>(tempo)));

    CHECK(registry.undoDepth() == 1);
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(140.0));

    CHECK(registry.undo());
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(original));
}

TEST_CASE("hostile tempos are clamped rather than adopted")
{
    // A tempo of zero is a timeline that never advances; the UI must not be
    // the only thing standing between a typo and a silent project.
    CHECK(SetProjectTempoCommand::clamped(0.0) == doctest::Approx(SetProjectTempoCommand::minimumTempo));
    CHECK(SetProjectTempoCommand::clamped(-120.0) == doctest::Approx(SetProjectTempoCommand::minimumTempo));
    CHECK(SetProjectTempoCommand::clamped(1e9) == doctest::Approx(SetProjectTempoCommand::maximumTempo));
    CHECK(SetProjectTempoCommand::clamped(std::nan("")) == doctest::Approx(120.0));

    project::Project project;
    CommandRegistry  registry{project};

    REQUIRE(registry.execute(std::make_unique<SetProjectTempoCommand>(0.0)));
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(SetProjectTempoCommand::minimumTempo));
}

TEST_CASE("a later tempo change survives an edit to the base tempo")
{
    // A project with a written ritardando must not lose it because someone
    // corrected the starting tempo.
    project::Project project;
    CommandRegistry  registry{project};

    project.tempoMap().setTempoEvents({{0, 120.0}, {engine::ticksPerQuarterNote * 16, 90.0}});

    REQUIRE(registry.execute(std::make_unique<SetProjectTempoCommand>(128.0)));

    CHECK(project.tempoMap().tempoEvents().size() == 2);
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(128.0));
    CHECK(project.tempoMap().tempoAtTick(engine::ticksPerQuarterNote * 16) == doctest::Approx(90.0));

    CHECK(registry.undo());
    CHECK(project.tempoMap().tempoAtTick(0) == doctest::Approx(120.0));
    CHECK(project.tempoMap().tempoAtTick(engine::ticksPerQuarterNote * 16) == doctest::Approx(90.0));
}

TEST_CASE("the time signature is validated, applied and reversible")
{
    project::Project project;
    CommandRegistry  registry{project};

    CHECK(SetTimeSignatureCommand::isValid(7, 8));
    CHECK_FALSE(SetTimeSignatureCommand::isValid(4, 3));      // not a note value
    CHECK_FALSE(SetTimeSignatureCommand::isValid(0, 4));
    CHECK_FALSE(SetTimeSignatureCommand::isValid(64, 4));     // a typo, not a metre

    REQUIRE(registry.execute(std::make_unique<SetTimeSignatureCommand>(7, 8)));

    const engine::TimeSignature applied = project.tempoMap().timeSignatureAtTick(0);
    CHECK(applied.numerator == 7);
    CHECK(applied.denominator == 8);

    CHECK(registry.undo());
    CHECK(project.tempoMap().timeSignatureAtTick(0).numerator == 4);
    CHECK(project.tempoMap().timeSignatureAtTick(0).denominator == 4);

    // An invalid signature never reaches the model.
    CHECK_FALSE(registry.execute(std::make_unique<SetTimeSignatureCommand>(5, 3)));
    CHECK(project.tempoMap().timeSignatureAtTick(0).denominator == 4);
}

TEST_CASE("the tempo edit is what the bar ruler reads back")
{
    // The point of the edit: musical positions must move with it, or the
    // playlist ruler and the transport disagree about where bar 2 is.
    project::Project project;
    CommandRegistry  registry{project};

    const engine::FramePosition atOneTwenty =
        project.tempoMap().frameForTick(engine::ticksPerQuarterNote * 4);

    REQUIRE(registry.execute(std::make_unique<SetProjectTempoCommand>(240.0)));

    const engine::FramePosition atTwoForty =
        project.tempoMap().frameForTick(engine::ticksPerQuarterNote * 4);

    CHECK(atTwoForty == atOneTwenty / 2);
}
