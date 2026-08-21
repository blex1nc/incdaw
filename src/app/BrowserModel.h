#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace incdaw::app {

/// What a file is, as far as the browser is concerned.
///
/// Classified by extension, deliberately: opening every file in a sample
/// library to sniff its header is the difference between a folder that lists
/// instantly and one that stalls the UI for seconds. Anything the browser
/// actually loads probes its real header at that point (`LoadSampleCommand`),
/// so a mislabelled extension costs a failed load, never a wrong decode.
enum class BrowserItemKind : std::uint8_t {
    folder,
    audio,
    midi,
    project,
    preset,
    other,
};

/// One row in the browser.
struct BrowserItem {
    std::filesystem::path path;
    std::string           name;
    BrowserItemKind       kind      = BrowserItemKind::other;
    std::uintmax_t        sizeBytes = 0;
    bool                  favourite = false;

    [[nodiscard]] bool isFolder() const noexcept { return kind == BrowserItemKind::folder; }
};

/// The file-system side of the browser (CLAUDE.md §19), with no UI in it.
///
/// Everything here is bounded on purpose. A sample library is routinely a
/// hundred thousand files across a network volume, and the two operations a
/// browser performs — list a folder, search a tree — are exactly the two that
/// turn into a beachball when written naively. Listing never recurses, search
/// is capped in both depth and results, and every file-system call goes
/// through the `error_code` overloads: a folder that cannot be read is an
/// empty folder, never an exception thrown into an AppKit callback.
class BrowserModel {
public:
    struct Root {
        std::filesystem::path path;
        std::string           name;    ///< display name; the folder's own by default
    };

    /// Ceilings for `search`. Generous for a human, cheap for a machine.
    static constexpr std::size_t defaultMaximumResults = 500;
    static constexpr int         defaultMaximumDepth   = 8;

    // ── Roots ───────────────────────────────────────────────────────────────
    void addRoot(const std::filesystem::path& path, std::string name = {});
    void removeRoot(const std::filesystem::path& path);
    void setRoots(const std::vector<std::filesystem::path>& paths);

    [[nodiscard]] const std::vector<Root>& roots() const noexcept { return roots_; }

    // ── Listing ─────────────────────────────────────────────────────────────
    /// Immediate children of `directory`: folders first, then files, each
    /// group ordered case-insensitively by name. Hidden entries are skipped,
    /// and so are files excluded by the kind filter — folders never are,
    /// because a filter that hides the path to a file is a filter that hides
    /// the file.
    [[nodiscard]] std::vector<BrowserItem> childrenOf(const std::filesystem::path& directory) const;

    // ── Search ──────────────────────────────────────────────────────────────
    /// Case-insensitive substring match on file names, under every root.
    ///
    /// An empty query returns nothing rather than everything: "no filter" in a
    /// search field means the user has not searched yet, and answering it with
    /// a hundred thousand rows is not helpful.
    [[nodiscard]] std::vector<BrowserItem> search(const std::string& query) const;

    void setMaximumResults(std::size_t results) noexcept { maximumResults_ = results; }
    void setMaximumDepth(int depth) noexcept { maximumDepth_ = depth; }

    /// True when the last `search` stopped at `maximumResults` with more to
    /// find. The UI says so rather than implying the library is that small.
    [[nodiscard]] bool lastSearchWasTruncated() const noexcept { return truncated_; }

    // ── Kind filter ─────────────────────────────────────────────────────────
    [[nodiscard]] static constexpr std::uint32_t bitFor(BrowserItemKind kind) noexcept
    {
        return 1u << static_cast<std::uint32_t>(kind);
    }

    /// Everything the browser can act on, which is the sensible default.
    static constexpr std::uint32_t allKinds = 0xFFFFFFFFu;

    void setVisibleKinds(std::uint32_t mask) noexcept { visibleKinds_ = mask; }
    [[nodiscard]] std::uint32_t visibleKinds() const noexcept { return visibleKinds_; }

    // ── Favourites ──────────────────────────────────────────────────────────
    void toggleFavourite(const std::filesystem::path& path);
    void setFavourites(const std::vector<std::filesystem::path>& paths);

    [[nodiscard]] bool isFavourite(const std::filesystem::path& path) const;

    /// Favourites as rows, ordered as added. Entries whose file has since
    /// disappeared are kept — a favourite on an unmounted volume is not a
    /// mistake to be silently corrected — and reported with `sizeBytes == 0`.
    [[nodiscard]] std::vector<BrowserItem> favourites() const;

    [[nodiscard]] const std::vector<std::filesystem::path>& favouritePaths() const noexcept
    {
        return favourites_;
    }

    // ── Classification ──────────────────────────────────────────────────────
    [[nodiscard]] static BrowserItemKind kindOf(const std::filesystem::path& path);

    /// Builds a row for one path, including whether it is a favourite.
    [[nodiscard]] BrowserItem itemFor(const std::filesystem::path& path) const;

private:
    [[nodiscard]] bool accepts(BrowserItemKind kind) const noexcept
    {
        return (visibleKinds_ & bitFor(kind)) != 0;
    }

    std::vector<Root>                  roots_;
    std::vector<std::filesystem::path> favourites_;

    std::uint32_t visibleKinds_   = allKinds;
    std::size_t   maximumResults_ = defaultMaximumResults;
    int           maximumDepth_   = defaultMaximumDepth;

    mutable bool truncated_ = false;
};

} // namespace incdaw::app
