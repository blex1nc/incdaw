#include "app/BrowserModel.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>

namespace incdaw::app {

namespace {

std::string lowered(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return text;
}

/// Extensions INCDAW can actually do something with. Audio is what the readers
/// support (engine/audio/WavFile.h, AiffFile.h) plus the formats a user will
/// try regardless — a file that turns out to be unreadable fails at load with a
/// reason, which is better than a browser that pretends it does not exist.
constexpr std::array<const char*, 8> audioExtensions{
    ".wav", ".wave", ".aif", ".aiff", ".aifc", ".flac", ".mp3", ".m4a"};

constexpr std::array<const char*, 2> midiExtensions{".mid", ".midi"};

/// `.incdawstate` is a plugin state blob, not a project; it is deliberately
/// classified as a preset so that opening the wrong one is impossible.
constexpr std::array<const char*, 1> projectExtensions{".incdaw"};
constexpr std::array<const char*, 3> presetExtensions{".incdawstate", ".fxp", ".vstpreset"};

bool contains(const auto& list, const std::string& extension)
{
    return std::find_if(list.begin(), list.end(), [&extension](const char* candidate) {
               return extension == candidate;
           }) != list.end();
}

/// Hidden files, and the directories no one browses for samples.
///
/// A leading dot is the rule on every Unix; the bundle suffixes are here
/// because macOS presents them as single files, and descending into one turns
/// a search for "kick" into a walk through an application's resources.
bool isHidden(const std::filesystem::path& path)
{
    const std::string name = path.filename().string();
    if (name.empty() || name.front() == '.')
        return true;

    const std::string extension = lowered(path.extension().string());
    return extension == ".app" || extension == ".bundle" || extension == ".framework";
}

bool isDirectory(const std::filesystem::directory_entry& entry)
{
    std::error_code failed;
    const bool      directory = entry.is_directory(failed);
    return !failed && directory;
}

std::uintmax_t sizeOf(const std::filesystem::directory_entry& entry)
{
    std::error_code failed;
    const auto      size = entry.file_size(failed);
    return failed ? 0 : size;
}

/// Folders first, then files; within each group, case-insensitive by name.
/// A browser sorted by the raw byte value of the name puts "Zap" before "kick",
/// which reads as unsorted.
bool orderedBefore(const BrowserItem& left, const BrowserItem& right)
{
    if (left.isFolder() != right.isFolder())
        return left.isFolder();

    const std::string leftName  = lowered(left.name);
    const std::string rightName = lowered(right.name);

    return leftName != rightName ? leftName < rightName : left.name < right.name;
}

} // namespace

// ── Roots ────────────────────────────────────────────────────────────────────

void BrowserModel::addRoot(const std::filesystem::path& path, std::string name)
{
    if (path.empty())
        return;

    for (const Root& root : roots_)
        if (root.path == path)
            return;

    std::string display = std::move(name);
    if (display.empty()) {
        display = path.filename().string();
        if (display.empty())
            display = path.string();
    }

    roots_.push_back(Root{path, std::move(display)});
}

void BrowserModel::removeRoot(const std::filesystem::path& path)
{
    roots_.erase(std::remove_if(roots_.begin(), roots_.end(),
                                [&path](const Root& root) { return root.path == path; }),
                 roots_.end());
}

void BrowserModel::setRoots(const std::vector<std::filesystem::path>& paths)
{
    roots_.clear();
    for (const std::filesystem::path& path : paths)
        addRoot(path);
}

// ── Listing ──────────────────────────────────────────────────────────────────

std::vector<BrowserItem> BrowserModel::childrenOf(const std::filesystem::path& directory) const
{
    std::vector<BrowserItem> items;

    std::error_code failed;
    std::filesystem::directory_iterator iterator(directory, failed);
    if (failed)
        return items;                      // unreadable is empty, never an error

    const std::filesystem::directory_iterator end;
    for (; iterator != end; iterator.increment(failed)) {
        if (failed)
            break;

        const std::filesystem::directory_entry& entry = *iterator;
        if (isHidden(entry.path()))
            continue;

        const BrowserItemKind kind = kindOf(entry.path());

        // Folders are never filtered out: a filter that hides the path to a
        // file hides the file.
        if (kind != BrowserItemKind::folder && !accepts(kind))
            continue;

        BrowserItem item;
        item.path      = entry.path();
        item.name      = entry.path().filename().string();
        item.kind      = kind;
        item.sizeBytes = isDirectory(entry) ? 0 : sizeOf(entry);
        item.favourite = isFavourite(entry.path());
        items.push_back(std::move(item));
    }

    std::sort(items.begin(), items.end(), orderedBefore);
    return items;
}

// ── Search ───────────────────────────────────────────────────────────────────

std::vector<BrowserItem> BrowserModel::search(const std::string& query) const
{
    truncated_ = false;

    std::vector<BrowserItem> results;
    if (query.empty())
        return results;

    const std::string needle = lowered(query);

    for (const Root& root : roots_) {
        std::error_code failed;

        std::filesystem::recursive_directory_iterator iterator(
            root.path, std::filesystem::directory_options::skip_permission_denied, failed);
        if (failed)
            continue;

        const std::filesystem::recursive_directory_iterator end;
        for (; iterator != end; iterator.increment(failed)) {
            if (failed) {
                // One unreadable subtree must not end the search; step over it.
                failed.clear();
                continue;
            }

            if (results.size() >= maximumResults_) {
                truncated_ = true;
                return results;
            }

            const std::filesystem::directory_entry& entry = *iterator;

            if (isHidden(entry.path())) {
                if (isDirectory(entry))
                    iterator.disable_recursion_pending();
                continue;
            }

            const BrowserItemKind kind = kindOf(entry.path());

            if (kind == BrowserItemKind::project) {
                // A project package is a result, not a folder to walk into.
                iterator.disable_recursion_pending();
            } else if (isDirectory(entry)) {
                // Depth is what keeps a symlinked loop or a twelve-level sample
                // library from becoming an unbounded walk.
                if (iterator.depth() + 1 >= maximumDepth_)
                    iterator.disable_recursion_pending();
                continue;
            }

            if (!accepts(kind))
                continue;

            if (lowered(entry.path().filename().string()).find(needle) == std::string::npos)
                continue;

            BrowserItem item;
            item.path      = entry.path();
            item.name      = entry.path().filename().string();
            item.kind      = kind;
            item.sizeBytes = sizeOf(entry);
            item.favourite = isFavourite(entry.path());
            results.push_back(std::move(item));
        }
    }

    std::sort(results.begin(), results.end(), orderedBefore);
    return results;
}

// ── Favourites ───────────────────────────────────────────────────────────────

void BrowserModel::toggleFavourite(const std::filesystem::path& path)
{
    if (path.empty())
        return;

    const auto existing = std::find(favourites_.begin(), favourites_.end(), path);
    if (existing != favourites_.end()) {
        favourites_.erase(existing);
        return;
    }

    favourites_.push_back(path);
}

void BrowserModel::setFavourites(const std::vector<std::filesystem::path>& paths)
{
    favourites_.clear();
    for (const std::filesystem::path& path : paths)
        if (!path.empty() && std::find(favourites_.begin(), favourites_.end(), path) == favourites_.end())
            favourites_.push_back(path);
}

bool BrowserModel::isFavourite(const std::filesystem::path& path) const
{
    return std::find(favourites_.begin(), favourites_.end(), path) != favourites_.end();
}

std::vector<BrowserItem> BrowserModel::favourites() const
{
    std::vector<BrowserItem> items;
    items.reserve(favourites_.size());

    for (const std::filesystem::path& path : favourites_)
        items.push_back(itemFor(path));

    return items;
}

// ── Classification ───────────────────────────────────────────────────────────

BrowserItemKind BrowserModel::kindOf(const std::filesystem::path& path)
{
    const std::string extension = lowered(path.extension().string());

    // Checked before the directory test, and that order matters: an .incdaw
    // project IS a directory (docs/PROJECT_FORMAT.md — a package), and a
    // browser that lists it as a folder invites the user to walk into its
    // internals instead of opening it.
    if (contains(projectExtensions, extension))
        return BrowserItemKind::project;

    std::error_code failed;
    if (std::filesystem::is_directory(path, failed) && !failed)
        return BrowserItemKind::folder;

    if (extension.empty())
        return BrowserItemKind::other;

    if (contains(audioExtensions, extension))  return BrowserItemKind::audio;
    if (contains(midiExtensions, extension))   return BrowserItemKind::midi;
    if (contains(presetExtensions, extension)) return BrowserItemKind::preset;

    return BrowserItemKind::other;
}

BrowserItem BrowserModel::itemFor(const std::filesystem::path& path) const
{
    BrowserItem item;
    item.path      = path;
    item.name      = path.filename().string();
    item.kind      = kindOf(path);
    item.favourite = isFavourite(path);

    std::error_code failed;
    const auto      size = std::filesystem::file_size(path, failed);
    item.sizeBytes = failed ? 0 : size;

    return item;
}

} // namespace incdaw::app
