// INCDAW — macOS application.
//
// Phase 8: the render graph is no longer assembled here. The window owns a
// Project and asks project::compileProjectGraph for a graph; every edit
// recompiles and hands the result to the audio engine through an atomic swap.
//
// Phase 8b adds the two panes that make the project visible: a pattern list and
// a Channel Rack with a step sequencer. The window holds no editing logic of
// its own — the panes emit commands and it recompiles what they changed.
//
// Not yet present: the mixer (Phase 10) and automation (Phase 11). The signal
// path is instrument -> channel gain -> master gain -> device, and nothing
// pretends otherwise — channel pan is deliberately not applied, because a pan
// law belongs to the mixer.

#import <Cocoa/Cocoa.h>

#include "app/CommandRegistry.h"
#include "app/Version.h"
#include "engine/AudioEngine.h"
#include "platform/SystemInfo.h"
#include "project/Model.h"
#include "project/ProjectGraphCompiler.h"
#include "ui/macos/ChannelRackView.h"
#include "ui/macos/PatternListView.h"
#include "ui/macos/PianoRollView.h"

#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

/// A short phrase so the editor opens with something to look at and edit,
/// rather than an empty grid. It is ordinary pattern content — selectable,
/// movable, deletable, undoable — not a fixture the UI treats specially.
void addStarterPhrase(std::vector<project::MidiEvent>& events)
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
        events.push_back(note);
    }
}

} // namespace

@interface INCDAWAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow*                window;
@property (strong) INCDAWPianoRollView*     pianoRoll;
@property (strong) INCDAWChannelRackView*   channelRack;
@property (strong) INCDAWPatternListView*   patternList;
@property (strong) NSTextField*             statusField;
@end

@implementation INCDAWAppDelegate {
    std::unique_ptr<project::Project>     _project;
    std::unique_ptr<app::CommandRegistry> _registry;
    std::unique_ptr<engine::AudioEngine>  _audio;

    NSTimer* _housekeeping;
    BOOL     _audioReady;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    _project  = std::make_unique<project::Project>();
    _registry = std::make_unique<app::CommandRegistry>(*_project);

    auto& channel = _project->addChannel("Channel 1");
    auto& pattern = _project->addPattern("Pattern 1");
    addStarterPhrase(pattern.contentFor(channel.id).events);

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

    constexpr CGFloat statusHeight  = 26.0;
    constexpr CGFloat listWidth     = 150.0;
    constexpr CGFloat rackHeight    = 220.0;

    const NSRect body = NSMakeRect(0, statusHeight, frame.size.width,
                                   frame.size.height - statusHeight);

    self.pianoRoll = [[INCDAWPianoRollView alloc]
        initWithFrame:NSMakeRect(0, 0, body.size.width - listWidth, body.size.height - rackHeight)
              project:_project.get()
             registry:_registry.get()];

    self.pianoRoll.patternIdValue = pattern.id.value();
    self.pianoRoll.channelIdValue = channel.id.value();

    self.channelRack = [[INCDAWChannelRackView alloc]
        initWithFrame:NSMakeRect(0, 0, body.size.width - listWidth, rackHeight)
              project:_project.get()
             registry:_registry.get()];

    self.channelRack.patternIdValue         = pattern.id.value();
    self.channelRack.selectedChannelIdValue = channel.id.value();

    self.patternList = [[INCDAWPatternListView alloc]
        initWithFrame:NSMakeRect(0, 0, listWidth, body.size.height)
              project:_project.get()
             registry:_registry.get()];

    self.patternList.selectedPatternIdValue = pattern.id.value();

    // Both panes scroll vertically; the rack scrolls its steps horizontally by
    // itself, so that the channel names stay pinned at the left edge.
    NSScrollView* rackScroll = [[NSScrollView alloc]
        initWithFrame:NSMakeRect(0, 0, body.size.width - listWidth, rackHeight)];
    rackScroll.hasVerticalScroller = YES;
    rackScroll.drawsBackground     = NO;
    rackScroll.documentView        = self.channelRack;

    NSScrollView* listScroll = [[NSScrollView alloc]
        initWithFrame:NSMakeRect(0, 0, listWidth, body.size.height)];
    listScroll.hasVerticalScroller = YES;
    listScroll.drawsBackground     = NO;
    listScroll.documentView        = self.patternList;

    NSSplitView* editors = [[NSSplitView alloc]
        initWithFrame:NSMakeRect(0, 0, body.size.width - listWidth, body.size.height)];
    editors.vertical      = NO;
    editors.dividerStyle  = NSSplitViewDividerStyleThin;
    [editors addSubview:self.pianoRoll];
    [editors addSubview:rackScroll];

    NSSplitView* workspace = [[NSSplitView alloc] initWithFrame:body];
    workspace.vertical         = YES;
    workspace.dividerStyle     = NSSplitViewDividerStyleThin;
    workspace.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [workspace addSubview:listScroll];
    [workspace addSubview:editors];

    [content addSubview:workspace];

    [workspace setPosition:listWidth ofDividerAtIndex:0];
    [editors setPosition:body.size.height - rackHeight ofDividerAtIndex:0];

    self.statusField = [NSTextField labelWithString:@""];
    self.statusField.frame = NSMakeRect(10, 4, frame.size.width - 20, statusHeight - 8);
    self.statusField.font = [NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    self.statusField.textColor = [NSColor colorWithCalibratedWhite:0.62 alpha:1.0];
    self.statusField.autoresizingMask = NSViewWidthSizable;
    [content addSubview:self.statusField];

    __weak INCDAWAppDelegate* weakSelf = self;

    void (^changed)(void) = ^{
        [weakSelf rebuildGraph];
        [weakSelf refreshStatus];
        [weakSelf.channelRack setNeedsDisplay:YES];
        [weakSelf.pianoRoll requestRedraw];
    };

    self.pianoRoll.onChange   = changed;
    self.channelRack.onChange = changed;
    self.patternList.onChange = changed;

    self.channelRack.onSelectChannel = ^(unsigned long long channelId) {
        [weakSelf selectChannel:channelId];
    };

    self.patternList.onSelectPattern = ^(unsigned long long patternId) {
        [weakSelf selectPattern:patternId];
    };

    [self startAudio];

    __weak INCDAWAppDelegate* weakAudioSelf = self;
    self.pianoRoll.onTransportToggle   = ^{ [weakAudioSelf toggleTransport]; };
    self.channelRack.onTransportToggle = ^{ [weakAudioSelf toggleTransport]; };
    self.patternList.onTransportToggle = ^{ [weakAudioSelf toggleTransport]; };

    // Drives the playhead and reclaims retired graphs. Both are non-realtime
    // work that must happen off the audio thread; 30 Hz is smooth enough for a
    // playhead and cheap enough to ignore.
    _housekeeping = [NSTimer scheduledTimerWithTimeInterval:1.0 / 30.0
                                                    repeats:YES
                                                      block:^(NSTimer* timer) {
        (void)timer;
        [weakAudioSelf housekeeping];
    }];

    [self buildMenu];
    [self refreshStatus];

    [self.window makeKeyAndOrderFront:nil];
    [self.window makeFirstResponder:self.pianoRoll];
    [NSApp activateIgnoringOtherApps:YES];
}

/// Points both editors at a channel. The rack owns the highlight, the Piano
/// Roll owns what it edits, and neither knows about the other.
- (void)selectChannel:(unsigned long long)channelId
{
    self.pianoRoll.channelIdValue          = channelId;
    self.channelRack.selectedChannelIdValue = channelId;

    [self.pianoRoll requestRedraw];
    [self refreshStatus];
}

/// Switches the pattern both editors and the transport are working on.
- (void)selectPattern:(unsigned long long)patternId
{
    self.pianoRoll.patternIdValue           = patternId;
    self.channelRack.patternIdValue         = patternId;
    self.patternList.selectedPatternIdValue = patternId;

    if (const project::Pattern* pattern = _project->findPattern(project::EntityId{patternId}))
        self.window.title = [NSString stringWithFormat:@"INCDAW — %s", pattern->name.c_str()];

    // The graph plays one pattern at a time until the playlist exists (Phase 9),
    // so switching pattern is a recompile, not just a view change.
    [self rebuildGraph];
    [self retargetLoop];
    [self.pianoRoll requestRedraw];
    [self refreshStatus];
}

- (void)startAudio
{
    _audio = std::make_unique<engine::AudioEngine>();

    platform::AudioDeviceConfig config;
    config.sampleRate     = 48000.0;
    config.bufferSize     = 256;
    config.outputChannels = 2;

    std::string error;
    if (!_audio->start(config, error)) {
        // Reported, not hidden: an editor that silently does not play is
        // indistinguishable from a broken one.
        NSLog(@"INCDAW: audio unavailable: %s", error.c_str());
        _audioReady = NO;
        return;
    }

    _audioReady = YES;
    _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());

    NSLog(@"INCDAW: audio started — %s, %.0f Hz, %lld frames",
          _audio->deviceName().c_str(), _audio->sampleRate(),
          static_cast<long long>(_audio->bufferSize()));

    [self rebuildGraph];
}

/// Rebuilds the render graph from the current project and swaps it in.
///
/// A whole-graph rebuild per edit rather than mutating the live one: the audio
/// thread may be halfway through reading the sequence, and the engine already
/// provides an atomic swap with deferred reclamation for exactly this
/// (docs/ARCHITECTURE.md §7). Edits happen at human speed; a rebuild costs
/// microseconds.
- (void)rebuildGraph
{
    if (!_audioReady || _project->patterns().empty())
        return;

    project::GraphCompileOptions options;
    options.sampleRate   = _audio->sampleRate();
    options.maxBlockSize = _audio->bufferSize();
    options.channelCount = _audio->outputChannels();
    options.source       = project::PlaybackSource::pattern;
    options.pattern      = project::EntityId{self.pianoRoll.patternIdValue};

    auto compiled = project::compileProjectGraph(*_project, _audio->transport().tempoMap(), options);
    if (!compiled) {
        NSLog(@"INCDAW: graph rebuild failed: %s", compiled.error.c_str());
        return;
    }

    _audio->setGraph(std::move(compiled.graph));
}

/// Loops whatever pattern is selected, so playback repeats rather than running
/// off into silence — and so that switching pattern while playing does not keep
/// looping the length of the previous one.
- (void)retargetLoop
{
    if (!_audioReady)
        return;

    auto& transport = _audio->transport();

    const project::Pattern* pattern =
        _project->findPattern(project::EntityId{self.pianoRoll.patternIdValue});

    const Tick length = pattern != nullptr && pattern->length > 0 ? pattern->length
                                                                  : ticksPerQuarterNote * 8;

    transport.setLoopRange(0, transport.tempoMap().frameForTick(length));
    transport.setLoopEnabled(true);
}

- (void)toggleTransport
{
    if (!_audioReady)
        return;

    auto& transport = _audio->transport();

    if (transport.isPlaying()) {
        transport.stop();
    } else {
        [self retargetLoop];
        transport.seek(0);
        transport.play();
    }

    [self refreshStatus];
}

- (void)housekeeping
{
    if (!_audioReady)
        return;

    _audio->collectRetiredGraphs();

    const auto position = _audio->transport().position();
    const long long tick = _audio->transport().isPlaying()
                               ? _audio->transport().tempoMap().tickForFrame(position)
                               : -1;

    self.pianoRoll.playheadTick   = tick;
    self.channelRack.playheadTick = tick;

    [self.pianoRoll requestRedraw];
    [self refreshStatus];
}

- (void)refreshStatus
{
    const project::Pattern* pattern =
        _project->findPattern(project::EntityId{self.pianoRoll.patternIdValue});

    const std::size_t noteCount = pattern != nullptr ? pattern->totalEventCount() : 0;

    NSString* undo = _registry->canUndo()
                         ? [NSString stringWithFormat:@"undo: %s", _registry->undoName().c_str()]
                         : @"undo: —";

    NSString* audio = @"audio: unavailable";
    if (_audioReady) {
        const bool playing = _audio->transport().isPlaying();
        audio = [NSString stringWithFormat:@"%@ · %.0f%% cpu",
                 playing ? @"▶ playing" : @"■ stopped",
                 _audio->profiler().peakLoad() * 100.0];
    }

    const project::Channel* channel =
        _project->findChannel(project::EntityId{self.pianoRoll.channelIdValue});

    self.statusField.stringValue = [NSString stringWithFormat:
        @"INCDAW %s  ·  %s / %s  ·  %lu notes  ·  %@  ·  %@  ·  space: play   "
        @"click: add or toggle step   ⌘Z: undo",
        app::Version::string(),
        pattern != nullptr ? pattern->name.c_str() : "—",
        channel != nullptr ? channel->name.c_str() : "—",
        static_cast<unsigned long>(noteCount), audio, undo];
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

- (void)applicationWillTerminate:(NSNotification*)notification
{
    (void)notification;

    // Stop the device before the graph it is reading goes away.
    [_housekeeping invalidate];
    _housekeeping = nil;

    if (_audio != nullptr)
        _audio->stop();
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
