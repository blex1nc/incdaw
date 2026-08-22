// The update check — everything about it except the socket.
//
// The whole point of app::UpdateCheck is that the decision is separable from
// the network: what a version means, which release is newest, whether the user
// already said no, and whether a check is due are all answerable without one.
// These tests answer them.
//
// The failure this file exists to prevent is not "the check said nothing". It
// is the check being WRONG in the user's favour twice over: telling someone
// they are up to date when they are not, or nagging them about a version they
// explicitly skipped. Both are silent, and neither shows up in a build.

#include "doctest.h"

#include "app/UpdateCheck.h"
#include "app/Version.h"

#include <string>
#include <vector>

using namespace incdaw;

namespace {

/// A releases feed shaped like GitHub's, with only the fields the parser reads.
std::string feed(const std::string& entries)
{
    return "[" + entries + "]";
}

std::string entry(const std::string& tag,
                  const std::string& name       = "",
                  bool               prerelease = false,
                  bool               draft      = false)
{
    return std::string{"{"} + R"("tag_name":")" + tag + R"(",)" + R"("name":")" + name + R"(",)"
           + R"("html_url":"https://github.com/blex1nc/incdaw/releases/tag/)" + tag + R"(",)"
           + R"("body":"notes for )" + tag + R"(",)"
           + R"("prerelease":)" + (prerelease ? "true" : "false") + ","
           + R"("draft":)" + (draft ? "true" : "false") + "}";
}

} // namespace

// ── Versions ─────────────────────────────────────────────────────────────────

TEST_CASE("a version is read from the shapes a tag is actually written in")
{
    const auto plain = app::ReleaseVersion::parse("1.2.3");
    REQUIRE(plain.has_value());
    CHECK(plain->major == 1);
    CHECK(plain->minor == 2);
    CHECK(plain->patch == 3);
    CHECK_FALSE(plain->isPrerelease());

    // The conventional "v" prefix belongs to the tag, not to the version.
    CHECK(app::ReleaseVersion::parse("v1.2.3")->toString() == "1.2.3");
    CHECK(app::ReleaseVersion::parse("V1.2.3")->toString() == "1.2.3");
    CHECK(app::ReleaseVersion::parse("  1.2.3\n")->toString() == "1.2.3");

    // Missing components are zero, not a parse failure: "1.2" is a version.
    CHECK(app::ReleaseVersion::parse("1.2")->toString() == "1.2.0");
    CHECK(app::ReleaseVersion::parse("2")->toString() == "2.0.0");

    // Build metadata does not affect precedence (semver §10), so it is dropped.
    CHECK(app::ReleaseVersion::parse("1.2.3+build.77")->toString() == "1.2.3");

    const auto beta = app::ReleaseVersion::parse("1.0.0-beta.2");
    REQUIRE(beta.has_value());
    CHECK(beta->prerelease == "beta.2");
    CHECK(beta->toString() == "1.0.0-beta.2");
}

TEST_CASE("a tag that is not a version is declined rather than guessed at")
{
    // The alternative — reading these as 0.0.0 — would offer every user in the
    // world an upgrade to nothing, which is exactly the kind of wrong that
    // nobody notices until it has already annoyed everyone.
    CHECK_FALSE(app::ReleaseVersion::parse("nightly").has_value());
    CHECK_FALSE(app::ReleaseVersion::parse("").has_value());
    CHECK_FALSE(app::ReleaseVersion::parse("v").has_value());
    CHECK_FALSE(app::ReleaseVersion::parse("1.2.3rc").has_value());
    CHECK_FALSE(app::ReleaseVersion::parse("1.2.3 candidate").has_value());
    CHECK_FALSE(app::ReleaseVersion::parse("1.2.3-").has_value());
    CHECK_FALSE(app::ReleaseVersion::parse("release-2026-08-22").has_value());

    // A tag is text from the internet. It must not be able to overflow an int.
    CHECK_FALSE(app::ReleaseVersion::parse("99999999999.0.0").has_value());
}

TEST_CASE("versions order by number, and a pre-release sorts below its release")
{
    const auto compare = [](const char* a, const char* b) {
        return app::ReleaseVersion::compare(*app::ReleaseVersion::parse(a),
                                            *app::ReleaseVersion::parse(b));
    };

    CHECK(compare("0.9.0", "0.9.1") < 0);
    CHECK(compare("0.9.1", "0.9.0") > 0);
    CHECK(compare("0.9.0", "0.10.0") < 0);      // not a string comparison
    CHECK(compare("0.9.0", "1.0.0") < 0);
    CHECK(compare("1.0.0", "1.0.0") == 0);

    // The rule that stops a beta being offered as an upgrade to the final.
    CHECK(compare("1.0.0-beta.2", "1.0.0") < 0);
    CHECK(compare("1.0.0", "1.0.0-beta.2") > 0);
    CHECK(compare("1.0.0-beta.1", "1.0.0-beta.2") < 0);
}

TEST_CASE("the running version parses")
{
    // If this ever fails, every check in the running application silently
    // reports "unavailable" — the build cannot say what it is.
    CHECK(app::ReleaseVersion::parse(app::Version::string()).has_value());
    CHECK(app::updateUserAgent() == std::string{"INCDAW/"} + app::Version::string());
}

// ── The feed ─────────────────────────────────────────────────────────────────

TEST_CASE("a releases feed is read, and what is not published is dropped")
{
    std::vector<app::Release> releases;
    std::string               error;

    REQUIRE(app::parseReleaseFeed(feed(entry("v1.0.0", "One point oh") + ","
                                       + entry("v0.9.2", "", true) + ","
                                       + entry("v2.0.0", "", false, true) + ","
                                       + entry("nightly") + ","
                                       + entry("v0.9.1")),
                                  releases, error));

    // The draft and the unparseable tag are gone; the pre-release is kept but
    // flagged, because whether to offer it is the caller's policy.
    REQUIRE(releases.size() == 3);
    CHECK(releases[0].version.toString() == "1.0.0");
    CHECK(releases[0].name == "One point oh");
    CHECK(releases[0].url == "https://github.com/blex1nc/incdaw/releases/tag/v1.0.0");
    CHECK(releases[1].prerelease);
    CHECK(releases[2].version.toString() == "0.9.1");

    // A release with no title is named by its tag rather than by nothing.
    CHECK(releases[2].name == "v0.9.1");

    const app::Release* newest = app::newestRelease(releases);
    REQUIRE(newest != nullptr);
    CHECK(newest->version.toString() == "1.0.0");
}

TEST_CASE("the newest release is never a pre-release")
{
    std::vector<app::Release> releases;
    std::string               error;

    REQUIRE(app::parseReleaseFeed(feed(entry("v1.1.0", "", true) + "," + entry("v1.0.0")),
                                  releases, error));

    const app::Release* newest = app::newestRelease(releases);
    REQUIRE(newest != nullptr);
    CHECK(newest->version.toString() == "1.0.0");
}

TEST_CASE("a feed that is not a feed is refused instead of half-read")
{
    std::vector<app::Release> releases;
    std::string               error;

    CHECK_FALSE(app::parseReleaseFeed("", releases, error));
    CHECK_FALSE(error.empty());

    // The service explaining itself rather than answering. Its own words are
    // more useful than a generic failure, so they become the error.
    CHECK_FALSE(app::parseReleaseFeed(R"({"message":"API rate limit exceeded"})", releases, error));
    CHECK(error == "API rate limit exceeded");

    // Truncated mid-transfer — the shape a real network failure takes.
    CHECK_FALSE(app::parseReleaseFeed(R"([{"tag_name":"v1.0)", releases, error));

    // An empty list is a valid feed. It simply has nothing in it.
    CHECK(app::parseReleaseFeed("[]", releases, error));
    CHECK(releases.empty());
    CHECK(app::newestRelease(releases) == nullptr);
}

// ── The verdict ──────────────────────────────────────────────────────────────

TEST_CASE("a newer release is offered")
{
    const app::UpdateResult result = app::evaluateFeed(feed(entry("v0.9.1")), "0.9.0", "");

    CHECK(result.verdict == app::UpdateVerdict::available);
    CHECK(result.release.version.toString() == "0.9.1");
    CHECK(result.release.notes == "notes for v0.9.1");
}

TEST_CASE("the same version, and an older one, are up to date")
{
    CHECK(app::evaluateFeed(feed(entry("v0.9.0")), "0.9.0", "").verdict
          == app::UpdateVerdict::upToDate);

    // A build ahead of the newest release — which is every development build
    // between two releases — must not be told to downgrade.
    CHECK(app::evaluateFeed(feed(entry("v0.9.0")), "0.10.0", "").verdict
          == app::UpdateVerdict::upToDate);
}

TEST_CASE("a skipped version stays skipped, and nothing above it does")
{
    CHECK(app::evaluateFeed(feed(entry("v0.9.1")), "0.9.0", "0.9.1").verdict
          == app::UpdateVerdict::skipped);

    // The whole reason a skip stores a version rather than a flag: passing on
    // 0.9.1 is not consent to hear nothing about 1.0.0 forever.
    CHECK(app::evaluateFeed(feed(entry("v1.0.0")), "0.9.0", "0.9.1").verdict
          == app::UpdateVerdict::available);

    // A skip written by hand, or left over from a version scheme that changed,
    // must not silence everything.
    CHECK(app::evaluateFeed(feed(entry("v1.0.0")), "0.9.0", "not a version").verdict
          == app::UpdateVerdict::available);
}

TEST_CASE("not knowing is not an error the user has to act on")
{
    // Offline, rate-limited, or a project with nothing published yet: all of
    // them land on `unavailable` with something a human can read, and none of
    // them are a failure the launch path should interrupt anyone about.
    const app::UpdateResult offline = app::evaluateFeed("", "0.9.0", "");
    CHECK(offline.verdict == app::UpdateVerdict::unavailable);
    CHECK_FALSE(offline.message.empty());

    const app::UpdateResult empty = app::evaluateFeed("[]", "0.9.0", "");
    CHECK(empty.verdict == app::UpdateVerdict::unavailable);
    CHECK(empty.message == "no releases have been published yet");

    // Only tags nobody can compare to anything.
    CHECK(app::evaluateFeed(feed(entry("nightly")), "0.9.0", "").verdict
          == app::UpdateVerdict::unavailable);

    // And a build that cannot say what it is compares to nothing.
    CHECK(app::evaluateFeed(feed(entry("v1.0.0")), "dev", "").verdict
          == app::UpdateVerdict::unavailable);
}

// ── Cadence ──────────────────────────────────────────────────────────────────

TEST_CASE("the automatic check runs once a day, and only when it is wanted")
{
    constexpr std::int64_t day = app::automaticCheckIntervalSeconds;
    constexpr std::int64_t now = 1'800'000'000;

    // Off means off. Nothing about a stamp changes that.
    CHECK_FALSE(app::automaticCheckIsDue(false, 0, now));
    CHECK_FALSE(app::automaticCheckIsDue(false, now - day * 10, now));

    CHECK(app::automaticCheckIsDue(true, 0, now));              // never checked
    CHECK_FALSE(app::automaticCheckIsDue(true, now, now));      // just checked
    CHECK_FALSE(app::automaticCheckIsDue(true, now - day + 1, now));
    CHECK(app::automaticCheckIsDue(true, now - day, now));
    CHECK(app::automaticCheckIsDue(true, now - day * 30, now));

    // A stamp from the future is a clock that moved, not a check that ran. If
    // it were treated as recent, the check would never run again on that
    // machine.
    CHECK(app::automaticCheckIsDue(true, now + day * 365, now));
}
