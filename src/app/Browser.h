#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace incdaw::app {

/// What the browser found at a path.
///
/// The distinction that carries weight is `folder` versus `project` and
/// `plugin`: the latter two are directories on disk that must never be walked
/// into as if they were folders. A project package and a .clap bundle are each
/// one thing to the user, and the only sensible verb on them is "open".
enum class BrowserItemKind { folder, project, audio, midi, plugin, unknown };

/// One row in the browser.
struct BrowserItem {
    std::filesystem::path path;
    std::string           name;                            ///< file name, as shown
    BrowserItemKind       kind      = BrowserItemKind::unknown;
    bool                  favourite = false;
    bool                  exists    = true;                ///< false only for stale favourites and recents
    std::uintmax_t        sizeBytes = 0;                   ///< 0 for directories, and when the size could not be read
};

/// A named place the user browses from — "Music", a sample library, a folder
/// they dragged in. Roots are the browser's top level and are persisted.
struct BrowserRoot {
    std::string           name;
    std::filesystem::path path;
};

/// The Browser's file model: roots, listings, search, favourites and recents.
///
/// This is ordinary filesystem work with no OS-specific API in it, which is
/// why it lives in app/ and not in the shell: the pane draws exactly what this
/// returns, so what the user sees is what a test can assert. Every call here
/// touches the disk and none of it is realtime-safe — browsing is a UI-thread
/// activity by definition, and nothing in the audio path may call it.
///
/// What it deliberately does NOT do: decode, play, or load anything. Preview
/// and drag-into-project are separate steps built on top of this listing
/// (docs/FL2026_GAP.md P9), so that classification stays a pure question about
/// a path and cannot become a place where files get opened by accident.
class Browser {
public:
    /// Search stops at these limits rather than walking a whole disk. Both are
    /// generous for a sample library and small enough that a mistyped root
    /// (say "/") returns in a moment instead of hanging the pane.
    static constexpr std::size_t defaultSearchLimit = 500;
    static constexpr int         searchDepthLimit   = 8;

    static constexpr std::size_t recentLimit     = 20;
    static constexpr int         settingsVersion  = 1;

    // ── Classification ──────────────────────────────────────────────────────

    /// What `path` is, by name and by what is on disk. Costs a stat on
    /// directories; safe on paths that do not exist (they classify by
    /// extension alone, which is what a stale favourite needs).
    [[nodiscard]] static BrowserItemKind classify(const std::filesystem::path& path);

    /// Whether INCDAW can actually decode this audio file today.
    ///
    /// Kept separate from `classify` on purpose: a .flac is audio, and saying
    /// so is honest, but the sampler cannot read one yet. The UI needs both
    /// answers — one to draw the row, the other to refuse the drop with a
    /// reason instead of failing silently.
    [[nodiscard]] static bool canDecodeAudio(const std::filesystem::path& path);

    // ── Roots ───────────────────────────────────────────────────────────────

    /// Adds a root. A path already present is not duplicated; its name is
    /// updated, because renaming a root is the only other thing the user could
    /// have meant.
    void addRoot(std::string name, std::filesystem::path path);
    bool removeRoot(const std::filesystem::path& path);

    [[nodiscard]] const std::vector<BrowserRoot>& roots() const noexcept { return roots_; }

    /// The places a first launch should show: the user's own folders, each
    /// added only if it exists. Never removes or reorders what is already there.
    void addDefaultRoots(const std::filesystem::path& home);

    // ── Listing and search ──────────────────────────────────────────────────

    /// One directory, sorted: folders first, then everything else, each
    /// case-insensitively by name. Dot-files are skipped. On failure the
    /// result is empty and `error` says why — an unreadable folder is a
    /// message in the pane, never a crash.
    [[nodiscard]] std::vector<BrowserItem> list(const std::filesystem::path& directory,
                                                std::string&                 error) const;

    /// Recursive name search under `directory`, in tree order, capped at
    /// `limit` results and `searchDepthLimit` levels. Matching is a
    /// case-insensitive substring of the file name.
    ///
    /// Project packages and plugin bundles match as items but are never
    /// descended into: their insides are INCDAW's business, not the user's.
    [[nodiscard]] std::vector<BrowserItem> search(const std::filesystem::path& directory,
                                                  std::string_view             query,
                                                  std::size_t limit = defaultSearchLimit) const;

    // ── Favourites ──────────────────────────────────────────────────────────

    /// Returns the new state.
    bool toggleFavourite(const std::filesystem::path& path);
    void setFavourite(const std::filesystem::path& path, bool favourite);

    [[nodiscard]] bool isFavourite(const std::filesystem::path& path) const;

    [[nodiscard]] const std::vector<std::filesystem::path>& favourites() const noexcept
    {
        return favourites_;
    }

    /// Favourites as rows, in the order they were added. A favourite whose
    /// file has since gone comes back with `exists == false` rather than
    /// being dropped: the user put it there, and losing it silently would
    /// hide the fact that a library moved.
    [[nodiscard]] std::vector<BrowserItem> favouriteItems() const;

    // ── Recents ─────────────────────────────────────────────────────────────

    /// Most recent first, deduplicated, capped at `recentLimit`.
    void noteRecent(const std::filesystem::path& path);

    [[nodiscard]] const std::vector<std::filesystem::path>& recent() const noexcept
    {
        return recent_;
    }

    [[nodiscard]] std::vector<BrowserItem> recentItems() const;

    // ── Persistence ─────────────────────────────────────────────────────────

    /// Loads roots, favourites and recents from `file`.
    ///
    /// A file that does not exist is NOT a failure: a first launch has no
    /// settings, and the browser starts empty so the shell can offer the
    /// default roots. Returns false only for a file that exists and cannot be
    /// read or parsed, with the state left cleared.
    [[nodiscard]] bool load(const std::filesystem::path& file, std::string& error);

    /// Writes `file` through a staged sibling and a rename, so an interrupted
    /// save leaves the previous settings intact rather than a half file.
    [[nodiscard]] bool save(const std::filesystem::path& file, std::string& error) const;

    void clear() noexcept;

private:
    /// Shared by `list` and `search`: `entry` is already stat'ed by the
    /// iterator, so this avoids asking the filesystem the same question twice.
    [[nodiscard]] BrowserItem itemFor(const std::filesystem::directory_entry& entry) const;

    void searchInto(const std::filesystem::path& directory, const std::string& loweredQuery,
                    std::size_t limit, int depth, std::vector<BrowserItem>& out) const;

    std::vector<BrowserRoot>           roots_;
    std::vector<std::filesystem::path> favourites_;
    std::vector<std::filesystem::path> recent_;
};

} // namespace incdaw::app
