#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace incdaw::app {

/// The update check, with the network taken out of it.
///
/// A DAW that is distributed as a .dmg has no package manager behind it: a user
/// who installed 0.9.0 in March is still running 0.9.0 in September unless
/// something tells them otherwise. This is that something — the smallest form
/// of it that is honest: INCDAW asks its own release feed what the newest
/// version is, compares it to what is running, and says so. It never downloads,
/// never installs, never replaces a binary behind the user's back. The
/// "Download" button opens a page in a browser and the user decides.
///
/// Everything in this header is pure. Parsing a feed, comparing two versions
/// and deciding whether a check is due are all decisions, and decisions belong
/// where a test can reach them without a socket. The one impure part — the GET
/// itself — is platform/Http.h, and the shell is what joins the two.
///
/// NOTHING here may be called from the audio thread. It allocates, it parses,
/// and the code that feeds it blocks on a network. See docs/AUDIO_ENGINE.md §1.

/// A version, parsed from a release tag.
///
/// Semantic versioning as far as it is needed and no further: three numbers and
/// an optional pre-release suffix. The suffix is compared only as a tie-break,
/// and a pre-release sorts BELOW the release it leads to — 1.0.0-beta.2 is
/// older than 1.0.0, which is the rule that stops a beta tag from being offered
/// as an upgrade to the final build.
struct ReleaseVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;

    /// "beta.2" from "1.0.0-beta.2". Empty for a final release.
    std::string prerelease;

    /// Accepts "1.2.3", "v1.2.3", "1.2", "1.2.3-beta.2", and tolerates
    /// surrounding whitespace. Build metadata after "+" is ignored, as semver
    /// says it must be.
    ///
    /// Anything else is `nullopt` rather than a zero version: a tag that is not
    /// a version cannot be compared to one, and treating "nightly" as 0.0.0
    /// would silently offer every user a downgrade.
    [[nodiscard]] static std::optional<ReleaseVersion> parse(const std::string& text);

    /// "0.9.1" / "1.0.0-beta.2". Never carries the leading "v" — that belongs
    /// to the tag, not to the version.
    [[nodiscard]] std::string toString() const;

    [[nodiscard]] bool isPrerelease() const noexcept { return !prerelease.empty(); }

    /// Negative when `a` is older, zero when equal, positive when `a` is newer.
    [[nodiscard]] static int compare(const ReleaseVersion& a, const ReleaseVersion& b) noexcept;
};

/// One published release, as the update check cares about it.
struct Release {
    std::string    tag;        ///< exactly as published ("v0.9.1")
    ReleaseVersion version;
    std::string    name;       ///< the release's title, or the tag when it has none
    std::string    url;        ///< the page a human opens; never a binary
    std::string    notes;      ///< the release body, as written
    bool           prerelease = false;
    bool           draft      = false;
};

/// What the shell should tell the user.
enum class UpdateVerdict {
    /// Nothing newer than what is running has been published.
    upToDate,

    /// A newer release exists and the user has not asked to be left alone
    /// about it.
    available,

    /// A newer release exists, and it is the one the user chose to skip.
    /// The manual check reports it anyway; the automatic one stays quiet.
    skipped,

    /// The feed could not be turned into an answer — offline, rate-limited,
    /// malformed, or a project with no releases published yet. Never an error
    /// dialog on the launch path: not knowing is not a failure the user has to
    /// do anything about.
    unavailable,
};

struct UpdateResult {
    UpdateVerdict verdict = UpdateVerdict::unavailable;

    /// The release the verdict is about. Meaningful for `available` and
    /// `skipped`, and carries the newest known release for `upToDate`.
    Release release;

    /// Why, in words a user can read. Always set for `unavailable`.
    std::string message;
};

/// Where releases are published.
///
/// The public, unauthenticated GitHub releases endpoint for INCDAW's own
/// repository. Unauthenticated is the point: the request carries no token, no
/// account and no identifier, so an update check cannot become telemetry by
/// accident (docs/DECISIONS.md D-038).
[[nodiscard]] const char* releaseFeedUrl() noexcept;

/// What INCDAW calls itself when it asks. The GitHub API requires a User-Agent
/// and refuses requests without one; this is the whole of what is disclosed.
[[nodiscard]] std::string updateUserAgent();

/// Turns a GitHub releases response into releases.
///
/// Drafts are dropped (they are not published), and so is any tag that is not a
/// version. Pre-releases are kept here and filtered by the caller, because
/// "should a beta be offered" is a policy question, not a parsing one.
///
/// Returns false only when the text is not a JSON array of releases at all.
[[nodiscard]] bool parseReleaseFeed(const std::string& json,
                                    std::vector<Release>& out,
                                    std::string&          error);

/// The newest final release, or nullptr when the list holds none.
/// Pre-releases are never chosen: a stable build must not be told to upgrade
/// to a beta.
[[nodiscard]] const Release* newestRelease(const std::vector<Release>& releases);

/// The whole decision, given what the feed said and what the user already
/// chose to ignore.
///
/// `currentVersion` is normally `app::Version::string()`. A `skippedVersion`
/// that is older than the newest release no longer applies — skipping 0.9.1
/// must not silence 1.0.0.
[[nodiscard]] UpdateResult evaluateFeed(const std::string& json,
                                        const std::string& currentVersion,
                                        const std::string& skippedVersion);

/// A day. Long enough that a user who opens INCDAW forty times a day makes one
/// request, short enough that a release is not missed for a week.
inline constexpr std::int64_t automaticCheckIntervalSeconds = 24 * 60 * 60;

/// Whether the automatic check should run now.
///
/// `false` whenever the user turned it off, and `false` again until a day has
/// passed. A `lastCheckedUnix` in the future — a clock that was wrong, or a
/// settings file copied from another machine — is treated as due rather than
/// as a lock that never expires.
[[nodiscard]] bool automaticCheckIsDue(bool         enabled,
                                       std::int64_t lastCheckedUnix,
                                       std::int64_t nowUnix) noexcept;

} // namespace incdaw::app
