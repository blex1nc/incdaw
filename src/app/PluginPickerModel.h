#pragma once

#include "plugins/PluginIdentifier.h"

#include <cstddef>
#include <string>
#include <vector>

namespace incdaw::app {

/// One thing the user can drop into an insert slot.
struct PluginPickerEntry {
    plugins::PluginIdentifier plugin;
    std::string               name;
    std::string               category;

    [[nodiscard]] friend bool operator==(const PluginPickerEntry&,
                                         const PluginPickerEntry&) = default;
};

/// The catalogue behind the mixer's plugin picker: what can be inserted, how it
/// is grouped, what a search leaves visible, and which row a click lands on.
///
/// It is here rather than in the view for the reason every other model in
/// app/ is: a list that filters and a hit test that has to agree with what was
/// drawn are logic, and logic in a .mm file cannot be tested. The view draws
/// rows() and asks rowAtY(); it decides nothing.
///
/// The model holds no instruments. A generator belongs to a channel, not to a
/// mixer insert slot, and offering one here would build a slot whose plugin
/// the compiler cannot make an effect out of.
class PluginPickerModel {
public:
    static constexpr std::size_t noRow = static_cast<std::size_t>(-1);

    /// Row height, in points. Geometry lives with the hit test that has to
    /// agree with it.
    static constexpr double rowHeight = 20.0;

    /// A row as drawn: a category heading, or one entry.
    struct Row {
        bool        header = false;
        std::string text;
        std::size_t entry  = 0;   ///< index into entries(); meaningless on a header
    };

    /// Every builtin effect, grouped by what it does. The grouping is the
    /// picker's own: the engine catalogue is a flat table on purpose, and a
    /// category is a statement about where a user looks for a thing.
    void addBuiltinEffects();

    /// A scanned plugin. The shell owns the scan and hands the results over,
    /// exactly as it already does for the insert menu.
    void addHosted(plugins::PluginIdentifier plugin, std::string name);

    void clear();

    /// Case-insensitive; matches the name and the category. An empty search
    /// shows everything.
    void setSearch(std::string text);
    [[nodiscard]] const std::string& search() const noexcept { return search_; }

    [[nodiscard]] const std::vector<PluginPickerEntry>& entries() const noexcept
    {
        return entries_;
    }

    [[nodiscard]] const std::vector<Row>& rows() const noexcept { return rows_; }
    [[nodiscard]] std::size_t rowCount() const noexcept { return rows_.size(); }

    /// The entry a row shows, or nullptr for a heading or an invalid row.
    [[nodiscard]] const PluginPickerEntry* entryAtRow(std::size_t row) const noexcept;

    /// The row containing `y`, measured from the top of the list. noRow past
    /// the end, which is what a click below the last row is.
    [[nodiscard]] std::size_t rowAtY(double y) const noexcept;

    /// Total height of the list as drawn.
    [[nodiscard]] double contentHeight() const noexcept
    {
        return static_cast<double>(rows_.size()) * rowHeight;
    }

    // ── The highlighted row ─────────────────────────────────────────────────
    //
    // Typing in the search field and pressing Return must insert something,
    // so the highlight is model state: it survives a re-filter by moving to
    // the first entry rather than pointing at a row that no longer exists.

    void setHighlight(std::size_t row) noexcept;
    [[nodiscard]] std::size_t highlight() const noexcept { return highlight_; }

    /// Moves the highlight by `delta` ENTRIES, skipping headings.
    void moveHighlight(int delta) noexcept;

    /// The entry the highlight is on, or nullptr.
    [[nodiscard]] const PluginPickerEntry* highlightedEntry() const noexcept;

private:
    void rebuild();

    std::vector<PluginPickerEntry> entries_;
    std::vector<Row>               rows_;
    std::string                    search_;
    std::size_t                    highlight_ = noRow;
};

} // namespace incdaw::app
