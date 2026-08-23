#include "app/PluginPickerModel.h"

#include "engine/dsp/effects/BuiltinEffects.h"

#include <algorithm>
#include <cctype>

namespace incdaw::app {
namespace {

/// Where each builtin effect is looked for.
///
/// A table rather than a naming rule: "incdaw.tone" is a tone stack and
/// "incdaw.transientsplit" is dynamics, and no prefix scheme separates those
/// without renaming uids that project files already store.
const char* categoryFor(const std::string& uid)
{
    if (uid == "incdaw.compressor" || uid == "incdaw.limiter" || uid == "incdaw.limiterla"
        || uid == "incdaw.gate" || uid == "incdaw.transientsplit"
        || uid == "incdaw.multiband" || uid == "incdaw.deesser")
        return "Dynamics";

    if (uid == "incdaw.eq" || uid == "incdaw.tone" || uid == "incdaw.filter"
        || uid == "incdaw.saturator" || uid == "incdaw.eqp" || uid == "incdaw.shaper")
        return "Tone";

    if (uid == "incdaw.chorus" || uid == "incdaw.flanger" || uid == "incdaw.phaser")
        return "Modulation";

    if (uid == "incdaw.delay" || uid == "incdaw.reverb" || uid == "incdaw.convolver")
        return "Space";

    if (uid == "incdaw.utility" || uid == "incdaw.analyzer" || uid == "incdaw.loudness"
        || uid == "incdaw.imager")
        return "Utility";

    if (uid == "incdaw.vocoder")
        return "Modulation";

    // A builtin added later is still findable; it simply has no home yet.
    return "Effects";
}

std::string lowered(std::string text)
{
    for (char& character : text)
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));

    return text;
}

bool matches(const PluginPickerEntry& entry, const std::string& needle)
{
    if (needle.empty())
        return true;

    return lowered(entry.name).find(needle) != std::string::npos
        || lowered(entry.category).find(needle) != std::string::npos;
}

} // namespace

void PluginPickerModel::addBuiltinEffects()
{
    for (const engine::dsp::BuiltinEffectInfo& info : engine::dsp::builtinEffects()) {
        PluginPickerEntry entry;
        entry.plugin   = {plugins::Format::builtin, info.uid};
        entry.name     = info.displayName;
        entry.category = categoryFor(entry.plugin.uid);

        entries_.push_back(std::move(entry));
    }

    rebuild();
}

void PluginPickerModel::addHosted(plugins::PluginIdentifier plugin, std::string name)
{
    PluginPickerEntry entry;
    entry.plugin = std::move(plugin);
    entry.name   = name.empty() ? entry.plugin.uid : std::move(name);

    // Scanned plugins are not classified: the CLAP scan records an id, a name
    // and a vendor, and nothing about what the plugin does. Inventing a
    // category from the name would be a guess presented as a fact.
    entry.category = "Plugins";

    entries_.push_back(std::move(entry));
    rebuild();
}

void PluginPickerModel::clear()
{
    entries_.clear();
    rows_.clear();
    highlight_ = noRow;
}

void PluginPickerModel::setSearch(std::string text)
{
    search_ = std::move(text);
    rebuild();
}

void PluginPickerModel::rebuild()
{
    rows_.clear();

    const std::string needle = lowered(search_);

    // Categories appear in the order their first entry was added, so the
    // builtins keep the order the engine catalogue lists them in and scanned
    // plugins land at the end.
    std::vector<std::string> order;

    for (const PluginPickerEntry& entry : entries_) {
        if (!matches(entry, needle))
            continue;

        if (std::find(order.begin(), order.end(), entry.category) == order.end())
            order.push_back(entry.category);
    }

    for (const std::string& category : order) {
        rows_.push_back({true, category, 0});

        for (std::size_t index = 0; index < entries_.size(); ++index) {
            const PluginPickerEntry& entry = entries_[index];
            if (entry.category != category || !matches(entry, needle))
                continue;

            rows_.push_back({false, entry.name, index});
        }
    }

    // The highlight follows the filter: after typing, Return must insert what
    // the user is looking at, and the row it was on may not exist any more.
    highlight_ = noRow;
    for (std::size_t row = 0; row < rows_.size(); ++row) {
        if (!rows_[row].header) {
            highlight_ = row;
            break;
        }
    }
}

const PluginPickerEntry* PluginPickerModel::entryAtRow(std::size_t row) const noexcept
{
    if (row >= rows_.size() || rows_[row].header)
        return nullptr;

    const std::size_t index = rows_[row].entry;
    if (index >= entries_.size())
        return nullptr;

    return &entries_[index];
}

std::size_t PluginPickerModel::rowAtY(double y) const noexcept
{
    if (y < 0.0)
        return noRow;

    const auto row = static_cast<std::size_t>(y / rowHeight);
    return row < rows_.size() ? row : noRow;
}

void PluginPickerModel::setHighlight(std::size_t row) noexcept
{
    if (row < rows_.size() && !rows_[row].header)
        highlight_ = row;
}

void PluginPickerModel::moveHighlight(int delta) noexcept
{
    if (rows_.empty() || delta == 0)
        return;

    const int step = delta > 0 ? 1 : -1;
    int remaining  = delta > 0 ? delta : -delta;

    auto cursor = static_cast<long long>(highlight_ == noRow ? 0 : highlight_);

    while (remaining > 0) {
        auto candidate = cursor + step;

        // Walk past headings rather than stopping on one: the arrow keys move
        // between things that can be inserted.
        while (candidate >= 0 && candidate < static_cast<long long>(rows_.size())
               && rows_[static_cast<std::size_t>(candidate)].header)
            candidate += step;

        if (candidate < 0 || candidate >= static_cast<long long>(rows_.size()))
            break;   // at an end: the highlight stays where it is

        cursor = candidate;
        --remaining;
    }

    if (cursor >= 0 && cursor < static_cast<long long>(rows_.size())
        && !rows_[static_cast<std::size_t>(cursor)].header)
        highlight_ = static_cast<std::size_t>(cursor);
}

const PluginPickerEntry* PluginPickerModel::highlightedEntry() const noexcept
{
    return entryAtRow(highlight_);
}

} // namespace incdaw::app
