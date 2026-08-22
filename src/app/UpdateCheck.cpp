#include "app/UpdateCheck.h"

#include "app/Version.h"
#include "project/Json.h"

#include <cctype>
#include <cstdlib>
#include <string>

namespace incdaw::app {

using project::Json;

namespace {

/// INCDAW's own repository. One place, so the feed URL and anything that ever
/// links to a release page cannot disagree about which project this is.
constexpr const char* kFeedUrl = "https://api.github.com/repos/blex1nc/incdaw/releases?per_page=20";

std::string trimmed(const std::string& text)
{
    std::size_t begin = 0;
    std::size_t end   = text.size();

    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0)
        ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
        --end;

    return text.substr(begin, end - begin);
}

/// Reads digits at `position` into `out`. False when there are none, or when
/// there are so many that the number is not a version component any more — a
/// tag is user-controlled text and must not be able to overflow an int.
bool readNumber(const std::string& text, std::size_t& position, int& out)
{
    const std::size_t begin = position;

    while (position < text.size() && std::isdigit(static_cast<unsigned char>(text[position])) != 0)
        ++position;

    const std::size_t digits = position - begin;
    if (digits == 0 || digits > 9)
        return false;

    out = std::atoi(text.substr(begin, digits).c_str());
    return true;
}

} // namespace

std::optional<ReleaseVersion> ReleaseVersion::parse(const std::string& text)
{
    std::string body = trimmed(text);

    // "v1.2.3" is how tags are conventionally written and is not part of the
    // version. Accepted in either case, because tags are typed by hand.
    if (!body.empty() && (body.front() == 'v' || body.front() == 'V'))
        body.erase(body.begin());

    // Build metadata is explicitly not significant to precedence (semver §10),
    // so it is dropped before anything else looks at the string.
    if (const std::size_t plus = body.find('+'); plus != std::string::npos)
        body.erase(plus);

    if (body.empty())
        return std::nullopt;

    ReleaseVersion version;
    std::size_t    position = 0;

    if (!readNumber(body, position, version.major))
        return std::nullopt;

    if (position < body.size() && body[position] == '.') {
        ++position;
        if (!readNumber(body, position, version.minor))
            return std::nullopt;
    }

    if (position < body.size() && body[position] == '.') {
        ++position;
        if (!readNumber(body, position, version.patch))
            return std::nullopt;
    }

    if (position < body.size()) {
        // The only thing allowed after the numbers is a pre-release suffix.
        // Anything else ("1.2.3 candidate", "1.2.3rc") is not a version this
        // code understands, and guessing at it would be worse than declining.
        if (body[position] != '-')
            return std::nullopt;

        version.prerelease = body.substr(position + 1);
        if (version.prerelease.empty())
            return std::nullopt;
    }

    return version;
}

std::string ReleaseVersion::toString() const
{
    std::string text = std::to_string(major) + '.' + std::to_string(minor) + '.'
                       + std::to_string(patch);

    if (!prerelease.empty())
        text += '-' + prerelease;

    return text;
}

int ReleaseVersion::compare(const ReleaseVersion& a, const ReleaseVersion& b) noexcept
{
    if (a.major != b.major)
        return a.major < b.major ? -1 : 1;
    if (a.minor != b.minor)
        return a.minor < b.minor ? -1 : 1;
    if (a.patch != b.patch)
        return a.patch < b.patch ? -1 : 1;

    // Equal numbers: a pre-release is older than the release it leads to
    // (semver §11). Two pre-releases are ordered lexically, which is not the
    // full rule and is deliberately not pretending to be — INCDAW publishes
    // final releases, and the only job here is that a beta never outranks one.
    if (a.prerelease.empty() != b.prerelease.empty())
        return a.prerelease.empty() ? 1 : -1;

    if (a.prerelease == b.prerelease)
        return 0;

    return a.prerelease < b.prerelease ? -1 : 1;
}

const char* releaseFeedUrl() noexcept
{
    return kFeedUrl;
}

std::string updateUserAgent()
{
    return std::string{"INCDAW/"} + Version::string();
}

bool parseReleaseFeed(const std::string& json, std::vector<Release>& out, std::string& error)
{
    out.clear();
    error.clear();

    Json root;
    if (!Json::parse(json, root, error))
        return false;

    if (!root.isArray()) {
        // A single release object is what the "latest" endpoint returns, and
        // reading it costs nothing. Anything else that is an object is the
        // service explaining itself instead — a rate limit, a repository that
        // is not there — and its own words say more than a generic failure
        // would, so they are what the user is shown.
        if (!root.isObject() || !root.contains("tag_name")) {
            const std::string explanation = root["message"].asString();

            error = explanation.empty() ? "the release feed was not a list of releases"
                                        : explanation;
            return false;
        }

        Json wrapped = Json::array();
        wrapped.append(root);
        root = std::move(wrapped);
    }

    for (const Json& element : root.elements()) {
        if (!element.isObject())
            continue;

        Release release;
        release.tag        = element["tag_name"].asString();
        release.name       = element["name"].asString();
        release.url        = element["html_url"].asString();
        release.notes      = element["body"].asString();
        release.prerelease = element["prerelease"].asBool(false);
        release.draft      = element["draft"].asBool(false);

        // A draft is visible only to whoever wrote it and has no page a user
        // could open. It is not published, so it does not exist here.
        if (release.draft)
            continue;

        std::optional<ReleaseVersion> version = ReleaseVersion::parse(release.tag);

        // Some projects tag by date and put the version in the title. Reading
        // the name second costs nothing and rescues that case; a release with
        // neither is dropped, because a version is what this is all comparing.
        if (!version.has_value())
            version = ReleaseVersion::parse(release.name);

        if (!version.has_value())
            continue;

        release.version = *version;

        // A tag flagged as a pre-release by the publisher and a tag whose
        // version says so are the same thing to everything downstream.
        release.prerelease = release.prerelease || release.version.isPrerelease();

        if (release.name.empty())
            release.name = release.tag;

        out.push_back(std::move(release));
    }

    return true;
}

const Release* newestRelease(const std::vector<Release>& releases)
{
    const Release* newest = nullptr;

    for (const Release& release : releases) {
        if (release.prerelease)
            continue;

        if (newest == nullptr
            || ReleaseVersion::compare(release.version, newest->version) > 0)
            newest = &release;
    }

    return newest;
}

UpdateResult evaluateFeed(const std::string& json,
                          const std::string& currentVersion,
                          const std::string& skippedVersion)
{
    UpdateResult result;

    const std::optional<ReleaseVersion> running = ReleaseVersion::parse(currentVersion);
    if (!running.has_value()) {
        // The build cannot say what it is, so it cannot be compared to
        // anything. Silence is the only honest answer.
        result.message = "this build does not report a comparable version";
        return result;
    }

    std::vector<Release> releases;
    std::string          error;

    if (!parseReleaseFeed(json, releases, error)) {
        result.message = error.empty() ? "the release feed could not be read" : error;
        return result;
    }

    const Release* newest = newestRelease(releases);
    if (newest == nullptr) {
        result.message = "no releases have been published yet";
        return result;
    }

    result.release = *newest;

    if (ReleaseVersion::compare(newest->version, *running) <= 0) {
        result.verdict = UpdateVerdict::upToDate;
        return result;
    }

    // A skip applies to the version that was skipped and to nothing above it:
    // a user who passed on 0.9.1 has said nothing about 1.0.0.
    const std::optional<ReleaseVersion> skipped = ReleaseVersion::parse(skippedVersion);

    if (skipped.has_value() && ReleaseVersion::compare(newest->version, *skipped) <= 0) {
        result.verdict = UpdateVerdict::skipped;
        return result;
    }

    result.verdict = UpdateVerdict::available;
    return result;
}

bool automaticCheckIsDue(bool enabled, std::int64_t lastCheckedUnix, std::int64_t nowUnix) noexcept
{
    if (!enabled)
        return false;

    if (lastCheckedUnix <= 0)
        return true;

    // A stamp from the future means the clock moved, not that the check ran.
    // Waiting for it to "expire" would mean never checking again.
    if (lastCheckedUnix > nowUnix)
        return true;

    return nowUnix - lastCheckedUnix >= automaticCheckIntervalSeconds;
}

} // namespace incdaw::app
