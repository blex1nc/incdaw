#pragma once

namespace incdaw::app {

class CommandRegistry;

/// Registers the project actions the shell exposes by name.
///
/// CLAUDE.md §26: an action addressable by id is one a menu, a shortcut, a
/// controller mapping, the command palette and a future script can all reach
/// without any of them knowing which view implements it. The table lived
/// nowhere before this: every edit arrived as a command, but none of them had
/// a name anything could look up.
///
/// It lives in app/ rather than in the macOS shell for the reason every other
/// piece of project logic does — a table of what the application can do is not
/// a property of the window system, and a table only the shell can build is a
/// table no test can check (docs/ARCHITECTURE.md §2).
///
/// Deliberately small. An action belongs here once it is meaningful without a
/// selection: "Add Channel" is, "Delete Selected Notes" is not — the latter
/// needs a pattern, a channel and a selection, and those live in the view that
/// owns them. Widening this table is how the palette grows.
void registerStandardActions(CommandRegistry& registry);

} // namespace incdaw::app
