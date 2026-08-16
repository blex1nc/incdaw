#include "app/Browser.h"

#include "project/Json.h"
#include "project/ProjectFile.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <system_error>
#include <utility>

namespace incdaw::app {

namespace fs = std::filesystem;
using project::Json;

namespace {

std::string lowered(std::string text)
{
    for (char& character : text)
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));

    return text;
}

/// Dot-files are the operating system's business, not the musician's.
bool isHiddenName(const std::string& name)
{
    return !name.empty() && name.front() == '.';
}

/// Bundles: directories on macOS that are one thing to the user.
bool isPluginExtension(const std::string& extension)
{
    return extension == ".clap" || extension == ".vst3" || extension == ".component";
}

/// What a NON-directory extension means. Also answers for paths that are gone
/// (a stale favourite still knows what it was).
BrowserItemKind fileKind(const std::string& extension)
{
    if (isPluginExtension(extension))
        return BrowserItemKind::plugin;

    if (extension == ".incdaw")
        return BrowserItemKind::project;

    // Named as audio even where INCDAW cannot decode it yet: the row is
    // honest about what the file is, and Browser::canDecodeAudio is what
    // decides whether it can be used. See the header.
    if (extension == ".wav" || extension == ".aif" || extension == ".aiff" || extension == ".flac"
        || extension == ".mp3" || extension == ".m4a" || extension == ".ogg" || extension == ".caf")
        return BrowserItemKind::audio;

    if (extension == ".mid" || extension == ".midi")
        return BrowserItemKind::midi;

    return BrowserItemKind::unknown;
}

/// Folders first, then everything else; case-insensitively by name, with the
/// path breaking ties so the order is total and a listing never shuffles.
bool itemPrecedes(const BrowserItem& left, const BrowserItem& right)
{
    const bool leftIsFolder  = left.kind == BrowserItemKind::folder;
    const bool rightIsFolder = right.kind == BrowserItemKind::folder;

    if (leftIsFolder != rightIsFolder)
        return leftIsFolder;

    const std::string leftName  = lowered(left.name);
    const std::string rightName = lowered(right.name);

    if (leftName != rightName)
        return leftName < rightName;

    return left.path.string() < right.path.string();
}

/// A row for a path that may or may not still be there. Used by the favourites
/// and recents lists, which are remembered strings, not a live directory.
BrowserItem describe(const fs::path& path, bool favourite)
{
    BrowserItem item;
    item.path      = path;
    item.name      = path.filename().empty() ? path.string() : path.filename().string();
    item.kind      = Browser::classify(path);
    item.favourite = favourite;

    std::error_code code;
    item.exists = fs::exists(path, code) && !code;

    if (item.exists && fs::is_regular_file(path, code)) {
        const std::uintmax_t size = fs::file_size(path, code);
        item.sizeBytes            = code ? 0 : size;
    }

    return item;
}

} // namespace

// ── Classification ──────────────────────────────────────────────────────────

BrowserItemKind Browser::classify(const fs::path& path)
{
    const std::string extension = lowered(path.extension().string());

    if (isPluginExtension(extension))
        return BrowserItemKind::plugin;

    std::error_code code;

    if (fs::is_directory(path, code) && !code) {
        return extension == ".incdaw" || project::ProjectFile::isProjectPackage(path)
                   ? BrowserItemKind::project
                   : BrowserItemKind::folder;
    }

    return fileKind(extension);
}

bool Browser::canDecodeAudio(const fs::path& path)
{
    // WAV is what engine/audio can read today: AIFF is a writer only (Phase
    // 17), and FLAC and MP3 are gated behind a dependency decision
    // (docs/FL2026_GAP.md §2). This list grows when a reader does, not before.
    return lowered(path.extension().string()) == ".wav";
}

// ── Roots ───────────────────────────────────────────────────────────────────

void Browser::addRoot(std::string name, fs::path path)
{
    if (path.empty())
        return;

    if (name.empty())
        name = path.filename().empty() ? path.string() : path.filename().string();

    const auto existing = std::find_if(roots_.begin(), roots_.end(),
                                       [&path](const BrowserRoot& root) { return root.path == path; });

    if (existing != roots_.end()) {
        existing->name = std::move(name);
        return;
    }

    roots_.push_back(BrowserRoot{std::move(name), std::move(path)});
}

bool Browser::removeRoot(const fs::path& path)
{
    const auto existing = std::find_if(roots_.begin(), roots_.end(),
                                       [&path](const BrowserRoot& root) { return root.path == path; });

    if (existing == roots_.end())
        return false;

    roots_.erase(existing);
    return true;
}

void Browser::addDefaultRoots(const fs::path& home)
{
    if (home.empty())
        return;

    for (const char* name : {"Music", "Documents", "Downloads", "Desktop"}) {
        const fs::path candidate = home / name;

        std::error_code code;
        if (fs::is_directory(candidate, code) && !code)
            addRoot(name, candidate);
    }
}

// ── Listing and search ──────────────────────────────────────────────────────

BrowserItem Browser::itemFor(const fs::directory_entry& entry) const
{
    BrowserItem item;
    item.path      = entry.path();
    item.name      = entry.path().filename().string();
    item.favourite = isFavourite(entry.path());
    item.exists    = true;

    const std::string extension = lowered(entry.path().extension().string());

    std::error_code code;

    if (isPluginExtension(extension)) {
        item.kind = BrowserItemKind::plugin;
    } else if (entry.is_directory(code) && !code) {
        item.kind = extension == ".incdaw" || project::ProjectFile::isProjectPackage(entry.path())
                        ? BrowserItemKind::project
                        : BrowserItemKind::folder;
    } else {
        item.kind = fileKind(extension);

        const std::uintmax_t size = entry.file_size(code);
        item.sizeBytes            = code ? 0 : size;
    }

    return item;
}

std::vector<BrowserItem> Browser::list(const fs::path& directory, std::string& error) const
{
    error.clear();

    std::vector<BrowserItem> items;
    std::error_code          code;

    if (!fs::exists(directory, code)) {
        error = "No such folder: " + directory.string();
        return items;
    }

    if (!fs::is_directory(directory, code)) {
        error = "Not a folder: " + directory.string();
        return items;
    }

    fs::directory_iterator iterator(directory, fs::directory_options::skip_permission_denied, code);

    if (code) {
        error = "Could not read " + directory.string() + ": " + code.message();
        return items;
    }

    // Incremented through the error_code overload: the throwing one turns an
    // unreadable entry halfway through a folder into an exception, and a
    // browser must show what it could read instead.
    for (const fs::directory_iterator end; iterator != end; iterator.increment(code)) {
        if (code) {
            error = "Could not read all of " + directory.string() + ": " + code.message();
            break;
        }

        const fs::directory_entry& entry = *iterator;

        if (isHiddenName(entry.path().filename().string()))
            continue;

        items.push_back(itemFor(entry));
    }

    std::sort(items.begin(), items.end(), itemPrecedes);
    return items;
}

std::vector<BrowserItem> Browser::search(const fs::path& directory, std::string_view query,
                                         std::size_t limit) const
{
    std::vector<BrowserItem> results;

    if (query.empty() || limit == 0)
        return results;

    searchInto(directory, lowered(std::string(query)), limit, 0, results);
    return results;
}

void Browser::searchInto(const fs::path& directory, const std::string& loweredQuery,
                         std::size_t limit, int depth, std::vector<BrowserItem>& out) const
{
    if (out.size() >= limit || depth > searchDepthLimit)
        return;

    // One definition of "what is in a folder": search shows the same entries,
    // in the same order, that browsing into it would have shown.
    std::string      ignored;
    const auto       entries = list(directory, ignored);

    for (const BrowserItem& item : entries) {
        if (out.size() >= limit)
            return;

        if (lowered(item.name).find(loweredQuery) != std::string::npos)
            out.push_back(item);
    }

    for (const BrowserItem& item : entries) {
        if (out.size() >= limit)
            return;

        // Only folders. A project package and a plugin bundle matched above as
        // themselves; their insides are INCDAW's business, not the user's.
        if (item.kind == BrowserItemKind::folder)
            searchInto(item.path, loweredQuery, limit, depth + 1, out);
    }
}

// ── Favourites ──────────────────────────────────────────────────────────────

bool Browser::isFavourite(const fs::path& path) const
{
    return std::find(favourites_.begin(), favourites_.end(), path) != favourites_.end();
}

void Browser::setFavourite(const fs::path& path, bool favourite)
{
    if (path.empty())
        return;

    const auto existing = std::find(favourites_.begin(), favourites_.end(), path);

    if (favourite) {
        if (existing == favourites_.end())
            favourites_.push_back(path);

        return;
    }

    if (existing != favourites_.end())
        favourites_.erase(existing);
}

bool Browser::toggleFavourite(const fs::path& path)
{
    const bool wanted = !isFavourite(path);
    setFavourite(path, wanted);
    return isFavourite(path);
}

std::vector<BrowserItem> Browser::favouriteItems() const
{
    std::vector<BrowserItem> items;
    items.reserve(favourites_.size());

    for (const fs::path& path : favourites_)
        items.push_back(describe(path, true));

    return items;
}

// ── Recents ─────────────────────────────────────────────────────────────────

void Browser::noteRecent(const fs::path& path)
{
    if (path.empty())
        return;

    recent_.erase(std::remove(recent_.begin(), recent_.end(), path), recent_.end());
    recent_.insert(recent_.begin(), path);

    if (recent_.size() > recentLimit)
        recent_.resize(recentLimit);
}

std::vector<BrowserItem> Browser::recentItems() const
{
    std::vector<BrowserItem> items;
    items.reserve(recent_.size());

    for (const fs::path& path : recent_)
        items.push_back(describe(path, isFavourite(path)));

    return items;
}

// ── Persistence ─────────────────────────────────────────────────────────────

bool Browser::load(const fs::path& file, std::string& error)
{
    error.clear();
    clear();

    std::error_code code;

    if (!fs::exists(file, code))
        return true;   // a first launch has no settings, which is not a failure

    std::ifstream stream(file, std::ios::binary);

    if (!stream) {
        error = "Could not read " + file.string();
        return false;
    }

    const std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    Json        document;
    std::string parseError;

    if (!Json::parse(text, document, parseError)) {
        error = "Could not parse " + file.string() + ": " + parseError;
        return false;
    }

    // A newer file is read for what this version understands rather than
    // refused: browser settings are a convenience, and losing the user's
    // favourites because a later INCDAW added a field would be the worse bug.
    for (const Json& entry : document["roots"].elements())
        addRoot(entry["name"].asString(), fs::path(entry["path"].asString()));

    for (const Json& path : document["favourites"].elements())
        setFavourite(fs::path(path.asString()), true);

    for (const Json& path : document["recent"].elements()) {
        const std::string text_ = path.asString();

        if (!text_.empty() && recent_.size() < recentLimit)
            recent_.push_back(fs::path(text_));
    }

    return true;
}

bool Browser::save(const fs::path& file, std::string& error) const
{
    error.clear();

    if (file.empty()) {
        error = "No settings path";
        return false;
    }

    Json document = Json::object();
    document.set("version", settingsVersion);

    Json roots = Json::array();

    for (const BrowserRoot& root : roots_) {
        Json entry = Json::object();
        entry.set("name", root.name);
        entry.set("path", root.path.string());
        roots.append(std::move(entry));
    }

    document.set("roots", std::move(roots));

    Json favourites = Json::array();

    for (const fs::path& path : favourites_)
        favourites.append(Json(path.string()));

    document.set("favourites", std::move(favourites));

    Json recent = Json::array();

    for (const fs::path& path : recent_)
        recent.append(Json(path.string()));

    document.set("recent", std::move(recent));

    std::error_code code;

    if (!file.parent_path().empty())
        fs::create_directories(file.parent_path(), code);

    // Staged and renamed, like every other file INCDAW writes: an interrupted
    // save leaves the previous settings, never half of the new ones.
    const fs::path staging = fs::path(file).concat(".writing");

    {
        std::ofstream stream(staging, std::ios::binary | std::ios::trunc);

        if (!stream) {
            error = "Could not write " + staging.string();
            return false;
        }

        const std::string text = document.dump();
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));

        if (!stream) {
            error = "Could not write " + staging.string();
            return false;
        }
    }

    fs::rename(staging, file, code);

    if (code) {
        error = "Could not replace " + file.string() + ": " + code.message();

        std::error_code ignored;
        fs::remove(staging, ignored);
        return false;
    }

    return true;
}

void Browser::clear() noexcept
{
    roots_.clear();
    favourites_.clear();
    recent_.clear();
}

} // namespace incdaw::app
