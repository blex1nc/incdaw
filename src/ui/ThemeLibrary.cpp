#include "ui/ThemeLibrary.h"

#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace incdaw::ui::theme {
namespace {

constexpr const char* extension = ".json";

} // namespace

ThemeLibrary::ThemeLibrary(std::filesystem::path directory)
    : directory_(std::move(directory))
{
}

std::string ThemeLibrary::sanitiseName(std::string_view name)
{
    std::string result;
    result.reserve(name.size());

    for (const char character : name) {
        const unsigned char raw = static_cast<unsigned char>(character);

        // Path separators and control characters would make the name mean
        // something to the filesystem that it does not mean to the user.
        if (raw < 0x20 || character == '/' || character == '\\' || character == ':')
            continue;

        result.push_back(character);
    }

    const std::size_t first = result.find_first_not_of(" \t.");
    const std::size_t last  = result.find_last_not_of(" \t.");
    if (first == std::string::npos)
        return {};

    result = result.substr(first, last - first + 1);

    // A file name long enough to be refused by the filesystem is a theme that
    // cannot be saved; a truncated one is a theme that can.
    if (result.size() > 64)
        result.resize(64);

    return result;
}

std::filesystem::path ThemeLibrary::fileFor(std::string_view name) const
{
    if (directory_.empty())
        return {};

    const std::string safe = sanitiseName(name);
    if (safe.empty())
        return {};

    return directory_ / (safe + extension);
}

std::vector<ThemeLibrary::Entry> ThemeLibrary::entries() const
{
    std::vector<Entry> result;
    result.reserve(builtinCount() + 8);

    for (std::size_t slot = 0; slot < builtinCount(); ++slot)
        result.push_back(Entry{builtinName(slot), true, {}});

    if (directory_.empty())
        return result;

    std::error_code code;
    std::filesystem::directory_iterator walk(directory_, code);
    if (code)
        return result;

    std::vector<Entry> user;
    for (const std::filesystem::directory_entry& item : walk) {
        if (!item.is_regular_file(code) || item.path().extension() != extension)
            continue;

        // The name is the file's, not the file content's: two themes may not
        // occupy one file, and the folder is what the user sees.
        const std::string name = item.path().stem().string();
        if (name.empty())
            continue;

        user.push_back(Entry{name, false, item.path()});
    }

    std::sort(user.begin(), user.end(), [](const Entry& a, const Entry& b) {
        return a.name < b.name;
    });

    result.insert(result.end(), user.begin(), user.end());
    return result;
}

bool ThemeLibrary::contains(std::string_view name) const
{
    if (isBuiltinName(name))
        return true;

    const std::filesystem::path file = fileFor(name);
    if (file.empty())
        return false;

    std::error_code code;
    return std::filesystem::exists(file, code) && !code;
}

ThemePalette ThemeLibrary::resolve(std::string_view name) const
{
    for (std::size_t slot = 0; slot < builtinCount(); ++slot)
        if (namesMatch(name, builtinName(slot)))
            return builtinPalette(slot);

    const std::filesystem::path file = fileFor(name);
    if (file.empty())
        return defaultPalette();

    std::error_code code;
    if (!std::filesystem::exists(file, code) || code)
        return defaultPalette();

    // `ThemePalette::load` already takes the name from the file rather than
    // from its contents, which is what keeps a copied "Midnight.json" renamed
    // to "Neon.json" from reporting itself as the built-in.
    return ThemePalette::load(file);
}

bool ThemeLibrary::store(const ThemePalette& palette, std::string& error) const
{
    if (isBuiltinName(palette.name)) {
        error = "built-in themes cannot be overwritten";
        return false;
    }

    const std::filesystem::path file = fileFor(palette.name);
    if (file.empty()) {
        error = "no writable themes folder";
        return false;
    }

    if (!palette.save(file)) {
        error = "the theme could not be written";
        return false;
    }

    error.clear();
    return true;
}

std::string ThemeLibrary::uniqueName(std::string_view preferred) const
{
    const std::string base = sanitiseName(preferred);
    if (base.empty())
        return {};

    if (!contains(base))
        return base;

    for (int suffix = 2; suffix < 1000; ++suffix) {
        std::string candidate = base + " " + std::to_string(suffix);
        if (!contains(candidate))
            return candidate;
    }

    return {};
}

std::string ThemeLibrary::duplicate(const ThemePalette& source,
                                    std::string_view    preferred,
                                    std::string&        error) const
{
    const std::string name = uniqueName(preferred);
    if (name.empty()) {
        error = "no usable name for the copy";
        return {};
    }

    ThemePalette copy = source;
    copy.name         = name;

    if (!store(copy, error))
        return {};

    error.clear();
    return name;
}

bool ThemeLibrary::remove(std::string_view name, std::string& error) const
{
    if (isBuiltinName(name)) {
        error = "built-in themes cannot be deleted";
        return false;
    }

    const std::filesystem::path file = fileFor(name);
    if (file.empty()) {
        error = "no such theme";
        return false;
    }

    std::error_code code;
    if (!std::filesystem::remove(file, code) || code) {
        error = "the theme could not be deleted";
        return false;
    }

    error.clear();
    return true;
}

} // namespace incdaw::ui::theme
