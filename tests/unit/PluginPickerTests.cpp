// The mixer's plugin picker (UI build-out increment 15).
//
// The load-bearing claims: the catalogue the picker offers is the engine's own
// and stays in step with it, a search narrows without breaking the hit test,
// the highlight always points at something insertable, and instruments never
// appear — a generator in an insert slot is a slot the compiler cannot build.

#include "doctest.h"

#include "app/PluginPickerModel.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "engine/instrument/BuiltinInstruments.h"

#include <string>

using namespace incdaw;

namespace {

app::PluginPickerModel builtinPicker()
{
    app::PluginPickerModel picker;
    picker.addBuiltinEffects();
    return picker;
}

std::size_t entryRows(const app::PluginPickerModel& picker)
{
    std::size_t count = 0;
    for (const app::PluginPickerModel::Row& row : picker.rows())
        if (!row.header)
            ++count;

    return count;
}

} // namespace

TEST_CASE("the picker offers exactly the engine's effect catalogue")
{
    const app::PluginPickerModel picker = builtinPicker();

    CHECK(picker.entries().size() == engine::dsp::builtinEffects().size());
    CHECK(entryRows(picker) == engine::dsp::builtinEffects().size());

    // Every entry resolves in the engine: a picker row that cannot be built is
    // a menu entry that fails on click.
    for (const app::PluginPickerEntry& entry : picker.entries()) {
        CAPTURE(entry.plugin.uid);
        CHECK(entry.plugin.format == plugins::Format::builtin);
        CHECK(engine::dsp::findBuiltinEffect(entry.plugin.uid) != nullptr);
        CHECK_FALSE(entry.category.empty());
    }
}

TEST_CASE("no instrument is offered as an insert")
{
    const app::PluginPickerModel picker = builtinPicker();

    for (const app::PluginPickerEntry& entry : picker.entries()) {
        CAPTURE(entry.plugin.uid);
        CHECK(engine::findBuiltinInstrument(entry.plugin.uid) == nullptr);
    }
}

TEST_CASE("rows are grouped, and every group has a heading above its entries")
{
    const app::PluginPickerModel picker = builtinPicker();
    REQUIRE(picker.rowCount() > 0);

    CHECK(picker.rows().front().header);

    std::string heading;
    for (std::size_t row = 0; row < picker.rowCount(); ++row) {
        const app::PluginPickerModel::Row& current = picker.rows()[row];

        if (current.header) {
            CHECK_FALSE(current.text.empty());
            heading = current.text;
            CHECK(picker.entryAtRow(row) == nullptr);
            continue;
        }

        const app::PluginPickerEntry* entry = picker.entryAtRow(row);
        REQUIRE(entry != nullptr);
        CHECK(entry->category == heading);
        CHECK(entry->name == current.text);
    }
}

TEST_CASE("search narrows by name and by category, case-insensitively")
{
    app::PluginPickerModel picker = builtinPicker();
    const std::size_t everything = entryRows(picker);

    picker.setSearch("REVERB");
    CHECK(entryRows(picker) == 1);
    REQUIRE(picker.highlightedEntry() != nullptr);
    CHECK(picker.highlightedEntry()->plugin.uid == "incdaw.reverb");

    // A category is a search term too: it is how the user finds "something
    // that squashes this" without knowing the name.
    picker.setSearch("dynamics");
    CHECK(entryRows(picker) == 5);

    picker.setSearch("");
    CHECK(entryRows(picker) == everything);

    picker.setSearch("no such plugin");
    CHECK(picker.rowCount() == 0);
    CHECK(picker.highlightedEntry() == nullptr);
    CHECK(picker.highlight() == app::PluginPickerModel::noRow);
}

TEST_CASE("the hit test agrees with the geometry the list is drawn at")
{
    const app::PluginPickerModel picker = builtinPicker();
    const double height = app::PluginPickerModel::rowHeight;

    CHECK(picker.rowAtY(-1.0) == app::PluginPickerModel::noRow);
    CHECK(picker.rowAtY(0.0) == 0);
    CHECK(picker.rowAtY(height * 0.5) == 0);
    CHECK(picker.rowAtY(height) == 1);
    CHECK(picker.rowAtY(height * 2.5) == 2);

    CHECK(picker.rowAtY(picker.contentHeight()) == app::PluginPickerModel::noRow);
    CHECK(picker.rowAtY(picker.contentHeight() - 0.5) == picker.rowCount() - 1);
    CHECK(picker.contentHeight()
          == static_cast<double>(picker.rowCount()) * height);
}

TEST_CASE("the highlight only ever rests on something insertable")
{
    app::PluginPickerModel picker = builtinPicker();

    // It starts on the first entry, not on the heading above it.
    REQUIRE(picker.highlight() != app::PluginPickerModel::noRow);
    CHECK(picker.entryAtRow(picker.highlight()) != nullptr);

    // Walking the whole list steps over every heading on the way.
    for (std::size_t step = 0; step < picker.rowCount() + 4; ++step) {
        picker.moveHighlight(1);
        CAPTURE(step);
        CHECK(picker.entryAtRow(picker.highlight()) != nullptr);
    }

    // …and stops at the end rather than falling off it.
    const std::size_t last = picker.highlight();
    picker.moveHighlight(1);
    CHECK(picker.highlight() == last);

    for (std::size_t step = 0; step < picker.rowCount() + 4; ++step)
        picker.moveHighlight(-1);

    CHECK(picker.entryAtRow(picker.highlight()) != nullptr);
    picker.moveHighlight(-1);
    CHECK(picker.entryAtRow(picker.highlight()) != nullptr);

    // A heading cannot be selected by clicking one either.
    for (std::size_t row = 0; row < picker.rowCount(); ++row) {
        if (!picker.rows()[row].header)
            continue;

        const std::size_t before = picker.highlight();
        picker.setHighlight(row);
        CHECK(picker.highlight() == before);
    }
}

TEST_CASE("scanned plugins join the list without being classified")
{
    app::PluginPickerModel picker = builtinPicker();
    const std::size_t builtins = entryRows(picker);

    picker.addHosted({plugins::Format::clap, "com.acme.reverb"}, "Acme Reverb");
    picker.addHosted({plugins::Format::clap, "com.acme.nameless"}, "");

    CHECK(entryRows(picker) == builtins + 2);

    // The scan records no plugin type, so the picker says "Plugins" rather
    // than guessing from the name — an honest label beats a wrong one.
    const app::PluginPickerEntry& acme = picker.entries()[builtins];
    CHECK(acme.category == "Plugins");
    CHECK(acme.name == "Acme Reverb");

    // A plugin that reported no name is still reachable by its id.
    CHECK(picker.entries()[builtins + 1].name == "com.acme.nameless");

    picker.setSearch("acme");
    CHECK(entryRows(picker) == 2);

    picker.clear();
    CHECK(picker.rowCount() == 0);
    CHECK(picker.entries().empty());
}
