#pragma once

#include <functional>
#include <string>

namespace incdaw::platform {

/// What a request came back with.
struct HttpResponse {
    /// The HTTP status, or 0 when the request never reached a server at all —
    /// no network, DNS failure, timeout. Those are not statuses and must not be
    /// reported as one.
    long statusCode = 0;

    std::string body;

    /// Empty on success. Carries the transport-level failure otherwise, in the
    /// system's own words, because "offline" and "the host refused" are
    /// different problems to whoever reads it.
    std::string error;

    [[nodiscard]] bool ok() const noexcept
    {
        return error.empty() && statusCode >= 200 && statusCode < 300;
    }
};

/// A one-shot HTTP GET, performed off the calling thread.
///
/// `onFinished` is invoked exactly once, on the main thread, whatever happens —
/// success, HTTP error, timeout or no network at all. Delivering on the main
/// thread is the contract because every caller so far is a piece of UI, and a
/// callback that arrives on a background queue turns every one of them into a
/// threading bug waiting to be written.
///
/// This is the only networking primitive in INCDAW and it is deliberately this
/// small: a GET, a timeout, a callback. There is no session to configure, no
/// cookie jar, no cache and no credential store — nothing that could quietly
/// accumulate state about a user between launches.
///
/// NEVER call this from the audio thread. It allocates, it opens sockets, and
/// it may block for `timeoutSeconds`. See docs/AUDIO_ENGINE.md §1.
void httpGet(const std::string&                url,
             const std::string&                userAgent,
             double                            timeoutSeconds,
             std::function<void(HttpResponse)> onFinished);

} // namespace incdaw::platform
