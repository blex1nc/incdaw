// The one place in INCDAW that opens a socket.
//
// NSURLSession rather than a bundled HTTP library: it is part of the system,
// it is what macOS expects an application to use (App Transport Security, the
// user's proxy settings and the system trust store all apply for free), and it
// adds nothing to third_party/. No dependency is introduced by this file.

#include "platform/Http.h"

#import <Foundation/Foundation.h>

#include <memory>
#include <utility>

namespace incdaw::platform {

namespace {

using Callback = std::shared_ptr<std::function<void(HttpResponse)>>;

/// Every path out of `httpGet` ends here, so the promise the header makes —
/// invoked exactly once, on the main thread, whatever happened — holds for the
/// failures that never start a request as well as for the ones that do.
void deliverOnMain(Callback callback, HttpResponse response)
{
    auto payload = std::make_shared<HttpResponse>(std::move(response));

    dispatch_async(dispatch_get_main_queue(), ^{
        (*callback)(std::move(*payload));
    });
}

} // namespace

void httpGet(const std::string&                url,
             const std::string&                userAgent,
             double                            timeoutSeconds,
             std::function<void(HttpResponse)> onFinished)
{
    if (!onFinished)
        return;

    const Callback callback =
        std::make_shared<std::function<void(HttpResponse)>>(std::move(onFinished));

    NSURL* target = [NSURL URLWithString:@(url.c_str())];

    if (target == nil || target.host == nil) {
        HttpResponse response;
        response.error = "the address could not be understood";
        deliverOnMain(callback, std::move(response));
        return;
    }

    // Plain HTTP is refused here rather than by App Transport Security, so the
    // reason reaches the caller as a sentence instead of as a generic failure.
    if (![target.scheme isEqualToString:@"https"]) {
        HttpResponse response;
        response.error = "only https addresses are fetched";
        deliverOnMain(callback, std::move(response));
        return;
    }

    // Ephemeral: nothing is written to disk. No cookie jar, no credential
    // store, no cache that outlives the request — a fetch leaves no trace on
    // the machine, which is part of what makes it safe to run at launch.
    NSURLSessionConfiguration* configuration =
        [NSURLSessionConfiguration ephemeralSessionConfiguration];

    configuration.HTTPCookieStorage         = nil;
    configuration.URLCache                  = nil;
    configuration.requestCachePolicy        = NSURLRequestReloadIgnoringLocalCacheData;
    configuration.timeoutIntervalForRequest = timeoutSeconds > 0.0 ? timeoutSeconds : 15.0;

    // Waiting for connectivity would leave the request pending until the
    // machine came back online, and a launch-time check must fail quickly and
    // quietly rather than fire hours later over a session the user has moved on
    // from.
    configuration.waitsForConnectivity = NO;

    NSURLSession* session = [NSURLSession sessionWithConfiguration:configuration];

    NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:target];
    request.HTTPMethod           = @"GET";

    if (!userAgent.empty())
        [request setValue:@(userAgent.c_str()) forHTTPHeaderField:@"User-Agent"];

    NSURLSessionDataTask* task = [session
        dataTaskWithRequest:request
          completionHandler:^(NSData* data, NSURLResponse* urlResponse, NSError* error) {
              HttpResponse response;

              if (error != nil) {
                  response.error = error.localizedDescription.UTF8String;
              } else {
                  if ([urlResponse isKindOfClass:[NSHTTPURLResponse class]])
                      response.statusCode = static_cast<long>(
                          static_cast<NSHTTPURLResponse*>(urlResponse).statusCode);

                  if (data.length > 0)
                      response.body.assign(static_cast<const char*>(data.bytes),
                                           static_cast<std::size_t>(data.length));
              }

              // The session holds this block; invalidating it once the single
              // task it exists for has finished is what lets both go away.
              [session finishTasksAndInvalidate];

              deliverOnMain(callback, std::move(response));
          }];

    [task resume];
}

} // namespace incdaw::platform
