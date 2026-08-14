// INCDAW — macOS application.
//
// Phase 6: the Piano Roll is live. The audio engine, transport and MIDI engine
// exist and are tested, but are NOT yet connected to this window — playback of
// edited notes needs the instrument system (Phase 7). Nothing here pretends
// otherwise.

#import <Cocoa/Cocoa.h>

#include "app/CommandRegistry.h"
#include "app/Version.h"
#include "platform/SystemInfo.h"
#include "project/Model.h"
#include "ui/macos/PianoRollView.h"

#include <memory>
#include <string>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

/// A short phrase so the editor opens with something to look at and edit,
/// rather than an empty grid. It is ordinary pattern content — selectable,
/// movable, deletable, undoable — not a fixture the UI treats specially.
void addStarterPhrase(project::Pattern& pattern)
{
    const int  scale[] = {0, 4, 7, 12, 7, 4};
    const Tick step    = ticksPerQuarterNote / 2;

    for (int index = 0; index < 12; ++index) {
        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = static_cast<Tick>(index) * step;
        note.key      = 60 + scale[index % 6];
        note.duration = step - 20;
        note.value    = 70 + (index % 4) * 15;
        pattern.events.push_back(note);
    }
}

} // namespace

@interface INCDAWAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow*              window;
@property (strong) INCDAWPianoRollView*   pianoRoll;
@property (strong) NSTextField*           statusField;
@end

@implementation INCDAWAppDelegate {
    std::unique_ptr<project::Project>     _project;
    std::unique_ptr<app::CommandRegistry> _registry;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    _project  = std::make_unique<project::Project>();
    _registry = std::make_unique<app::CommandRegistry>(*_project);

    auto& pattern = _project->addPattern("Pattern 1");
    addStarterPhrase(pattern);

    const NSRect frame = NSMakeRect(0, 0, 1180, 720);

    self.window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                          | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    self.window.title = @"INCDAW — Pattern 1";
    self.window.backgroundColor = [NSColor colorWithCalibratedWhite:0.10 alpha:1.0];
    [self.window center];

    NSView* content = self.window.contentView;

    constexpr CGFloat statusHeight = 26.0;

    self.pianoRoll = [[INCDAWPianoRollView alloc]
        initWithFrame:NSMakeRect(0, statusHeight, frame.size.width, frame.size.height - statusHeight)
              project:_project.get()
             registry:_registry.get()];

    self.pianoRoll.patternIdValue = pattern.id.value();
    self.pianoRoll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [content addSubview:self.pianoRoll];

    self.statusField = [NSTextField labelWithString:@""];
    self.statusField.frame = NSMakeRect(10, 4, frame.size.width - 20, statusHeight - 8);
    self.statusField.font = [NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    self.statusField.textColor = [NSColor colorWithCalibratedWhite:0.62 alpha:1.0];
    self.statusField.autoresizingMask = NSViewWidthSizable;
    [content addSubview:self.statusField];

    __weak INCDAWAppDelegate* weakSelf = self;
    self.pianoRoll.onChange = ^{ [weakSelf refreshStatus]; };

    [self buildMenu];
    [self refreshStatus];

    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.pianoRoll];
    [NSApp activateIgnoringOtherApps:YES];
}

- (void)refreshStatus
{
    const auto& patterns = _project->patterns();
    const std::size_t noteCount = patterns.empty() ? 0 : patterns[0].events.size();

    NSString* undo = _registry->canUndo()
                         ? [NSString stringWithFormat:@"undo: %s", _registry->undoName().c_str()]
                         : @"undo: —";

    self.statusField.stringValue = [NSString stringWithFormat:
        @"INCDAW %s  ·  %s  ·  %lu notes  ·  %@  ·  click: add   drag: move   right-click: delete   "
        @"shift-drag: select   Q: quantize   ⌘Z: undo",
        app::Version::string(), app::Version::phase(),
        static_cast<unsigned long>(noteCount), undo];
}

- (void)buildMenu
{
    NSMenu* menuBar = [[NSMenu alloc] init];

    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [menuBar addItem:appItem];

    NSMenu* appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Quit INCDAW" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;

    NSMenuItem* editItem = [[NSMenuItem alloc] init];
    [menuBar addItem:editItem];

    // The menu deliberately carries no actions of its own: every command lives
    // in app::CommandRegistry and the view routes to it, so a menu entry and a
    // keystroke can never diverge (docs/ARCHITECTURE.md §6).
    NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];
    [editMenu addItemWithTitle:@"Undo" action:nil keyEquivalent:@"z"];
    [editMenu addItemWithTitle:@"Redo" action:nil keyEquivalent:@"Z"];
    [editMenu addItem:[NSMenuItem separatorItem]];
    [editMenu addItemWithTitle:@"Select All" action:nil keyEquivalent:@"a"];
    editItem.submenu = editMenu;

    NSApp.mainMenu = menuBar;
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
