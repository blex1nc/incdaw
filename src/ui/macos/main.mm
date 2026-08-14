// INCDAW — macOS application.
//
// Phase 8: the window is a channel rack and a Piano Roll over one project. The
// graph is no longer assembled here — project::compileProjectGraph builds it
// from the model, so every channel, pattern and clip the project contains is
// what plays (docs/DECISIONS.md D-013).
//
// Not yet present: the mixer (Phase 10) and automation (Phase 11). The signal
// path is instrument -> channel strip -> master gain -> device, and nothing
// pretends otherwise.

#import <Cocoa/Cocoa.h>

#include "app/CommandRegistry.h"
#include "app/commands/PatternCommands.h"
#include "app/Version.h"
#include "engine/AudioEngine.h"
#include "engine/graph/RenderGraph.h"
#include "platform/SystemInfo.h"
#include "project/GraphCompiler.h"
#include "project/Model.h"
#include "ui/macos/ChannelRackView.h"
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
void addStarterPhrase(project::Pattern& pattern, project::EntityId channel)
{
    const int  scale[] = {0, 4, 7, 12, 7, 4};
    const Tick step    = ticksPerQuarterNote / 2;

    for (int index = 0; index < 12; ++index) {
        project::MidiEvent note;
        note.type      = project::MidiEventType::note;
        note.tick      = static_cast<Tick>(index) * step;
        note.key       = 60 + scale[index % 6];
        note.duration  = step - 20;
        note.value     = 70 + (index % 4) * 15;
        note.channelId = channel;
        pattern.events.push_back(note);
    }
}

/// A four-on-the-floor kick and an off-beat hat, so the step grid opens with
/// something on it. Same status as the phrase above: ordinary pattern content.
void addStarterBeat(project::Pattern& pattern, project::EntityId kick, project::EntityId hat)
{
    const Tick step = pattern.stepDivision;

    for (int index = 0; index < 16; ++index) {
        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = static_cast<Tick>(index) * step;
        note.duration = step;
        note.value    = 100;

        if (index % 4 == 0) {
            note.key       = 36;
            note.channelId = kick;
            pattern.events.push_back(note);
        }

        if (index % 4 == 2) {
            note.key       = 37;
            note.value     = 78;
            note.channelId = hat;
            pattern.events.push_back(note);
        }
    }
}

} // namespace

@interface INCDAWAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow*              window;
@property (strong) INCDAWPianoRollView*   pianoRoll;
@property (strong) INCDAWChannelRackView* channelRack;
@property (strong) NSTextField*           statusField;
@end

@implementation INCDAWAppDelegate {
    std::unique_ptr<project::Project>     _project;
    std::unique_ptr<app::CommandRegistry> _registry;
    std::unique_ptr<engine::AudioEngine>  _audio;

    project::PlaybackMode _mode;

    /// Ticks the current graph plays before it repeats: the pattern's length in
    /// pattern mode, the arrangement's in song mode. Comes back from the
    /// compiler rather than being recomputed here, so the loop and the notes
    /// can never disagree.
    engine::Tick _playbackLengthTicks;

    NSTimer* _housekeeping;
    BOOL     _audioReady;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    _project  = std::make_unique<project::Project>();
    _registry = std::make_unique<app::CommandRegistry>(*_project);
    _mode     = project::PlaybackMode::pattern;

    const project::EntityId kick = _project->addChannel("Kick").id;
    const project::EntityId hat  = _project->addChannel("Hat").id;
    const project::EntityId lead = _project->addChannel("Lead").id;

    _project->addTrack(project::TrackType::instrument, "Track 1");

    auto& pattern = _project->addPattern("Pattern 1");
    addStarterBeat(pattern, kick, hat);
    addStarterPhrase(pattern, lead);

    const project::EntityId patternId = pattern.id;

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

    const CGFloat rackHeight =
        [INCDAWChannelRackView heightForChannelCount:_project->channels().size()];

    self.pianoRoll = [[INCDAWPianoRollView alloc]
        initWithFrame:NSMakeRect(0, statusHeight + rackHeight, frame.size.width,
                                 frame.size.height - statusHeight - rackHeight)
              project:_project.get()
             registry:_registry.get()];

    self.pianoRoll.patternIdValue = patternId.value();
    self.pianoRoll.channelIdValue = lead.value();
    self.pianoRoll.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [content addSubview:self.pianoRoll];

    self.channelRack = [[INCDAWChannelRackView alloc]
        initWithFrame:NSMakeRect(0, statusHeight, frame.size.width, rackHeight)
              project:_project.get()
             registry:_registry.get()];

    self.channelRack.patternIdValue         = patternId.value();
    self.channelRack.selectedChannelIdValue = lead.value();
    self.channelRack.autoresizingMask       = NSViewWidthSizable | NSViewMaxYMargin;
    [content addSubview:self.channelRack];

    self.statusField = [NSTextField labelWithString:@""];
    self.statusField.frame = NSMakeRect(10, 4, frame.size.width - 20, statusHeight - 8);
    self.statusField.font = [NSFont monospacedSystemFontOfSize:11 weight:NSFontWeightRegular];
    self.statusField.textColor = [NSColor colorWithCalibratedWhite:0.62 alpha:1.0];
    self.statusField.autoresizingMask = NSViewWidthSizable;
    [content addSubview:self.statusField];

    __weak INCDAWAppDelegate* weakSelf = self;
    self.pianoRoll.onChange = ^{
        [weakSelf rebuildGraph];
        [weakSelf refreshStatus];
        [weakSelf.channelRack setNeedsDisplay:YES];
    };

    self.channelRack.onChange = ^{
        [weakSelf rebuildGraph];
        [weakSelf refreshStatus];
        [weakSelf.pianoRoll requestRedraw];
    };

    // Selecting a channel decides which notes the Piano Roll edits; the rest
    // become ghost notes there rather than disappearing.
    self.channelRack.onChannelSelected = ^{
        weakSelf.pianoRoll.channelIdValue = weakSelf.channelRack.selectedChannelIdValue;
        [weakSelf.pianoRoll requestRedraw];
        [weakSelf refreshStatus];
    };

    [self startAudio];

    __weak INCDAWAppDelegate* weakAudioSelf = self;
    self.pianoRoll.onTransportToggle = ^{ [weakAudioSelf toggleTransport]; };

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
    if (!_audioReady)
        return;

    project::GraphCompileOptions options;
    options.mode          = _mode;
    options.activePattern = project::EntityId{self.pianoRoll.patternIdValue};
    options.sampleRate    = _audio->sampleRate();
    options.maxBlockSize  = _audio->bufferSize();
    options.channelCount  = _audio->outputChannels();

    auto compiled = project::compileProjectGraph(*_project, _audio->transport().tempoMap(), options);

    if (compiled.graph == nullptr) {
        NSLog(@"INCDAW: graph rebuild failed: %s", compiled.error.c_str());
        return;
    }

    _playbackLengthTicks = compiled.lengthTicks;
    _audio->setGraph(std::move(compiled.graph));

    // A running transport must keep looping the right span when the mode or the
    // arrangement changes underneath it.
    if (_audio->transport().isPlaying())
        [self applyLoopRange];
}

/// Loops whatever the current graph plays, from the start.
- (void)applyLoopRange
{
    auto& transport = _audio->transport();

    const engine::Tick length = _playbackLengthTicks > 0 ? _playbackLengthTicks
                                                         : ticksPerQuarterNote * 4;

    transport.setLoopRange(0, transport.tempoMap().frameForTick(length));
    transport.setLoopEnabled(YES);
}

- (void)toggleSongMode
{
    _mode = _mode == project::PlaybackMode::song ? project::PlaybackMode::pattern
                                                 : project::PlaybackMode::song;

    // Song mode with an empty arrangement would play silence and look broken,
    // so the first switch places the pattern once rather than leaving a puzzle.
    if (_mode == project::PlaybackMode::song && _project->clips().empty()
        && !_project->tracks().empty() && !_project->patterns().empty()) {
        const project::EntityId track   = _project->tracks().front().id;
        const project::EntityId pattern = _project->patterns().front().id;

        (void)_registry->execute(std::make_unique<app::AddPatternClipCommand>(track, pattern, 0));
    }

    [self rebuildGraph];

    if (_audioReady && _audio->transport().isPlaying()) {
        [self applyLoopRange];
        _audio->transport().seek(0);
    }

    [self refreshStatus];
}

- (void)toggleTransport
{
    if (!_audioReady)
        return;

    auto& transport = _audio->transport();

    if (transport.isPlaying()) {
        transport.stop();
    } else {
        // Loop whatever the graph plays, so playback repeats rather than running
        // off into silence.
        [self applyLoopRange];
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
    self.pianoRoll.playheadTick = _audio->transport().isPlaying()
                                      ? _audio->transport().tempoMap().tickForFrame(position)
                                      : -1;

    [self.pianoRoll requestRedraw];

    self.channelRack.playheadTick = self.pianoRoll.playheadTick;
    [self.channelRack setNeedsDisplay:YES];

    [self refreshStatus];
}

- (void)refreshStatus
{
    const project::Pattern* pattern =
        _project->findPattern(project::EntityId{self.pianoRoll.patternIdValue});

    const std::size_t noteCount = pattern != nullptr ? pattern->events.size() : 0;

    const project::Channel* channel =
        _project->findChannel(project::EntityId{self.pianoRoll.channelIdValue});

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

    self.statusField.stringValue = [NSString stringWithFormat:
        @"INCDAW %s  ·  %@  ·  %@  ·  %lu notes  ·  %@  ·  %@  ·  space: play   ⌘M: song/pattern   "
        @"click: add   right-click: delete   Q: quantize   ⌘Z: undo",
        app::Version::string(),
        _mode == project::PlaybackMode::song ? @"song" : @"pattern",
        channel != nullptr ? [NSString stringWithUTF8String:channel->name.c_str()] : @"—",
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

    NSMenuItem* transportItem = [[NSMenuItem alloc] init];
    [menuBar addItem:transportItem];

    NSMenu* transportMenu = [[NSMenu alloc] initWithTitle:@"Transport"];
    [transportMenu addItemWithTitle:@"Song / Pattern Mode"
                             action:@selector(toggleSongMode)
                      keyEquivalent:@"m"];
    transportItem.submenu = transportMenu;

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
