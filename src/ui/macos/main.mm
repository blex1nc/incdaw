// INCDAW — macOS application shell.
//
// Phase 1 scope: a native window that launches, reports the build, and proves
// the packaging pipeline end to end. There is no audio engine behind it yet —
// the engine is Phase 2 and the Metal widget layer is Phase 6. Nothing in this
// window pretends to do more than it does.

#import <Cocoa/Cocoa.h>

#include "app/Version.h"
#include "platform/SystemInfo.h"

#include <string>

namespace {

NSString* buildSummary()
{
    const auto info = incdaw::platform::SystemInfo::query();

    std::string text;
    text += "INCDAW ";
    text += incdaw::app::Version::string();
    text += "\n";
    text += incdaw::app::Version::phase();
    text += "\n\n";
    text += info.cpuBrand;
    text += "\n";
    text += std::to_string(info.performanceCoreCount) + " performance / "
          + std::to_string(info.efficiencyCoreCount)  + " efficiency cores\n";
    text += std::to_string(info.suggestedRealtimeWorkerCount()) + " realtime workers planned\n";
    text += std::to_string(info.physicalMemoryBytes / (1024ull * 1024ull * 1024ull)) + " GB memory";

    return [NSString stringWithUTF8String:text.c_str()];
}

} // namespace

@interface INCDAWAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow* window;
@end

@implementation INCDAWAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    const NSRect frame = NSMakeRect(0, 0, 720, 460);

    self.window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                          | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    self.window.title = @"INCDAW";
    self.window.backgroundColor = [NSColor colorWithCalibratedWhite:0.11 alpha:1.0];
    [self.window center];

    NSTextField* label = [NSTextField labelWithString:buildSummary()];
    label.font            = [NSFont monospacedSystemFontOfSize:13 weight:NSFontWeightRegular];
    label.textColor       = [NSColor colorWithCalibratedWhite:0.82 alpha:1.0];
    label.alignment       = NSTextAlignmentCenter;
    label.maximumNumberOfLines = 0;
    label.translatesAutoresizingMaskIntoConstraints = NO;

    NSView* content = self.window.contentView;
    [content addSubview:label];
    [NSLayoutConstraint activateConstraints:@[
        [label.centerXAnchor constraintEqualToAnchor:content.centerXAnchor],
        [label.centerYAnchor constraintEqualToAnchor:content.centerYAnchor],
    ]];

    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
    return YES;
}

@end

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        app.activationPolicy = NSApplicationActivationPolicyRegular;

        INCDAWAppDelegate* delegate = [[INCDAWAppDelegate alloc] init];
        app.delegate = delegate;

        [app run];
    }

    return 0;
}
