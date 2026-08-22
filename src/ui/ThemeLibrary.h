#pragma once

// INCDAW — the folder of themes, and the rules for naming things inside it.
//
// A theme is a file, not a setting. That is the whole point: a scheme someone
// spends an evening on can be copied to another machine, mailed to somebody
// else, kept in a repository, or edited in a text editor without INCDAW being
// open. `settings.json` therefore stores a theme *name*, and this class is what
// turns that name back into colours.
//
// It is deliberately plain C++ with no AppKit in sight, because the awkward
// parts of a user-writable folder — a name with a slash in it, two themes that
// want the same file, a file that disappeared between the scan and the load —
// are logic, and logic belongs where it can be tested.

#include "ui/ThemePalette.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace incdaw::ui::theme {

/// The built-in schemes plus whatever is in the user's Themes folder.
class ThemeLibrary {
public:
    /// `directory` is normally "<Application Support>/INCDAW/Themes". An empty
    /// path is legal and yields a library of the built-ins alone — which is
    /// what a machine with no writable support directory gets, rather than an
    /// application that refuses to draw.
    explicit ThemeLibrary(std::filesystem::path directory);

    struct Entry {
        std::string           name;
        bool                  builtin = false;
        std::filesystem::path file;     ///< empty for built-ins
    };

    [[nodiscard]] const std::filesystem::path& directory() const noexcept { return directory_; }

    /// Built-ins in their declared order, then user themes sorted by name.
    /// A user theme whose name collides with a built-in is listed under its
    /// own name and wins nothing: `resolve` prefers the built-in, because the
    /// scheme INCDAW ships with must always be reachable.
    [[nodiscard]] std::vector<Entry> entries() const;

    /// Name → colours. Falls back to the default palette when the name is
    /// unknown, the file is gone, or the file is unreadable.
    [[nodiscard]] ThemePalette resolve(std::string_view name) const;

    [[nodiscard]] bool contains(std::string_view name) const;

    /// Where a user theme of this name is written. Empty when the library has
    /// no directory, or when the name sanitises down to nothing.
    [[nodiscard]] std::filesystem::path fileFor(std::string_view name) const;

    /// Writes `palette` under its own name. Refuses to overwrite a built-in,
    /// because a built-in that can be edited is not a built-in.
    [[nodiscard]] bool store(const ThemePalette& palette, std::string& error) const;

    /// Copies `source` to a free user-theme name derived from `preferred`, and
    /// returns the name actually used. Empty on failure, with `error` set.
    [[nodiscard]] std::string duplicate(const ThemePalette& source,
                                        std::string_view    preferred,
                                        std::string&        error) const;

    /// Deletes a user theme. Built-ins cannot be deleted.
    [[nodiscard]] bool remove(std::string_view name, std::string& error) const;

    /// Strips what a file name may not carry — separators, dots at the front,
    /// control characters — and trims the result. May return an empty string,
    /// which callers must treat as "no usable name".
    [[nodiscard]] static std::string sanitiseName(std::string_view name);

    /// `preferred`, or "preferred 2", "preferred 3"… until nothing owns it.
    [[nodiscard]] std::string uniqueName(std::string_view preferred) const;

private:
    std::filesystem::path directory_;
};

} // namespace incdaw::ui::theme
