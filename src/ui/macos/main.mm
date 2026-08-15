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

#include "app/AutomationWriteSession.h"
#include "app/CommandRegistry.h"
#include "app/Version.h"
#include "app/commands/RecordingCommands.h"
#include "engine/AudioEngine.h"
#include "platform/SystemInfo.h"
#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginRegistry.h"
#include "project/Model.h"
#include "project/PatternCompiler.h"
#include "project/ProjectGraphCompiler.h"
#include "project/RecordingSession.h"
#include "app/commands/AudioEditCommands.h"
#include "ui/macos/AudioEditorView.h"
#include "ui/macos/ChannelRackView.h"
#include "ui/macos/PatternListView.h"
#include "ui/macos/PianoRollView.h"
#include "ui/macos/MixerView.h"
#include "ui/macos/PlaylistView.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <limits>
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

/// Where INCDAW keeps state that belongs to the installation rather than to a
/// project: the plugin catalogue today. Empty when the directory cannot be
/// created, which callers treat as "no cache", never as a failure to launch.
std::filesystem::path incdawSupportDirectory()
{
    NSString* base = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                                          NSUserDomainMask, YES) firstObject];
    if (base == nil)
        return {};

    std::filesystem::path directory{base.UTF8String};
    directory /= "INCDAW";

    std::error_code failed;
    std::filesystem::create_directories(directory, failed);

    return failed ? std::filesystem::path{} : directory;
}

} // namespace

@interface INCDAWAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow*                window;
@property (strong) INCDAWPianoRollView*     pianoRoll;
@property (strong) INCDAWPlaylistView*      playlist;
@property (strong) INCDAWMixerView*         mixer;
@property (strong) INCDAWAudioEditorView*   audioEditor;
@property (strong) NSSegmentedControl*      editorSelector;
@property (strong) NSSegmentedControl*      transportModeSelector;
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

    /// Handles into the graph that is rendering right now. The nodes are owned
    /// by the engine; this is refreshed on every rebuild and is what lets the
    /// mixer read meters and write fader moves without recompiling.
    project::CompiledProjectGraph _live;

    /// Song mode plays the arrangement; pattern mode loops the selected
    /// pattern. FL calls these the same two things, and they are the same two
    /// things everywhere: what the transport is looking at, not a UI filter.
    BOOL     _songMode;

    NSString* _lastGraphError;

    /// One take at a time, from arm to placement (project/RecordingSession.h).
    project::RecordingSession _recording;
    NSString*                 _lastRecordError;

    /// Keeps streamed audio clips' windows filled. Graphs hold the streams;
    /// this only services them, so destruction order does not matter.
    std::unique_ptr<engine::DiskStreamer> _diskStreamer;

    /// Undo/redo of an audio edit rewrites a file behind the editor's back;
    /// watching the undo stack's depth from housekeeping catches it without
    /// the registry having to know views exist.
    std::size_t _undoDepthSeen;

    /// Write-mode automation recording (app/AutomationWriteSession.h).
    app::AutomationWriteSession _autoWrite;

    /// The plugin catalogue, and the libraries hosted from it.
    ///
    /// Both outlive every graph, deliberately: a PluginNode's instance calls
    /// back into its library when it is destroyed, and graphs are retired
    /// asynchronously (plugins/PluginInstanceManager.h). The registry is loaded
    /// from disk at launch and never scans on the startup path.
    plugins::PluginRegistry                        _pluginRegistry;
    std::unique_ptr<plugins::PluginInstanceManager> _pluginInstances;

    /// The application's parameter system: the strip built-ins plus every
    /// plugin parameter discovered so far. Graphs compile against it, so it
    /// outlives them all here.
    project::ParameterRegistry _parameters;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    _project      = std::make_unique<project::Project>();
    _registry     = std::make_unique<app::CommandRegistry>(*_project);
    _diskStreamer = std::make_unique<engine::DiskStreamer>();

    // The plugin catalogue is read from a file; launching touches no plugin
    // binary at all, because startup time must not scale with the size of a
    // plugin collection (docs/PLUGIN_HOST.md §3). A missing file is the normal
    // first-run state, not an error.
    if (const std::filesystem::path support = incdawSupportDirectory(); !support.empty())
        (void)_pluginRegistry.load(support / "plugins.tsv");

    _pluginInstances = std::make_unique<plugins::PluginInstanceManager>(_pluginRegistry);
    _parameters      = project::ParameterRegistry::withBuiltins();

    const project::EntityId channelId = _project->addChannel("Channel 1").id;
    const project::EntityId patternId = _project->addPattern("Pattern 1").id;

    project::Pattern& pattern = *_project->findPattern(patternId);
    addStarterPhrase(pattern.contentFor(channelId).events);

    // One track with the pattern placed on it, so switching to song mode plays
    // something rather than presenting an empty timeline and silence. Ordinary
    // project data — movable, deletable, undoable.
    const project::EntityId trackId =
        _project->addTrack(project::TrackType::instrument, "Track 1").id;

    project::Clip& clip = _project->addClip(project::ClipType::pattern, trackId, patternId);
    clip.startTick   = 0;
    clip.lengthTicks = pattern.length;
    clip.name        = pattern.name;
    clip.colour      = pattern.colour;

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
    constexpr CGFloat toolbarHeight = 30.0;
    constexpr CGFloat listWidth     = 150.0;
    constexpr CGFloat rackHeight    = 220.0;

    const NSRect body = NSMakeRect(0, statusHeight, frame.size.width,
                                   frame.size.height - statusHeight - toolbarHeight);

    const NSRect editorFrame = NSMakeRect(0, 0, body.size.width - listWidth,
                                          body.size.height - rackHeight);

    self.pianoRoll = [[INCDAWPianoRollView alloc]
        initWithFrame:editorFrame
              project:_project.get()
             registry:_registry.get()];

    self.pianoRoll.patternIdValue = patternId.value();
    self.pianoRoll.channelIdValue = channelId.value();

    self.channelRack = [[INCDAWChannelRackView alloc]
        initWithFrame:NSMakeRect(0, 0, body.size.width - listWidth, rackHeight)
              project:_project.get()
             registry:_registry.get()];

    self.channelRack.patternIdValue         = patternId.value();
    self.channelRack.selectedChannelIdValue = channelId.value();

    self.patternList = [[INCDAWPatternListView alloc]
        initWithFrame:NSMakeRect(0, 0, listWidth, body.size.height)
              project:_project.get()
             registry:_registry.get()];

    self.patternList.selectedPatternIdValue = patternId.value();

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

    self.playlist = [[INCDAWPlaylistView alloc]
        initWithFrame:editorFrame
              project:_project.get()
             registry:_registry.get()];

    self.playlist.patternIdValue = patternId.value();
    self.playlist.hidden         = YES;

    self.mixer = [[INCDAWMixerView alloc]
        initWithFrame:editorFrame
              project:_project.get()
             registry:_registry.get()];

    self.mixer.selectedChannelIdValue = channelId.value();
    self.mixer.hidden                 = YES;

    self.audioEditor = [[INCDAWAudioEditorView alloc]
        initWithFrame:editorFrame
              project:_project.get()
             registry:_registry.get()];
    self.audioEditor.hidden = YES;

    __weak INCDAWAppDelegate* weakSelfForEditor = self;
    self.playlist.onOpenAudioAsset = ^(unsigned long long assetId) {
        [weakSelfForEditor openAudioAssetInEditor:assetId];
    };

    self.mixer.onParameterEdited = ^(unsigned long long nodeId, const char* key,
                                     double normalized) {
        [weakSelfForEditor automationParameterEdited:nodeId key:key value:normalized];
    };

    // The editors share one region and are swapped rather than tiled: each
    // wants the whole window, and a DAW that shows half a Piano Roll above half
    // a playlist shows neither.
    NSView* editorContainer = [[NSView alloc] initWithFrame:editorFrame];
    self.pianoRoll.autoresizingMask   = NSViewWidthSizable | NSViewHeightSizable;
    self.playlist.autoresizingMask    = NSViewWidthSizable | NSViewHeightSizable;
    self.mixer.autoresizingMask       = NSViewWidthSizable | NSViewHeightSizable;
    self.audioEditor.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [editorContainer addSubview:self.pianoRoll];
    [editorContainer addSubview:self.playlist];
    [editorContainer addSubview:self.mixer];
    [editorContainer addSubview:self.audioEditor];

    NSSplitView* editors = [[NSSplitView alloc]
        initWithFrame:NSMakeRect(0, 0, body.size.width - listWidth, body.size.height)];
    editors.vertical      = NO;
    editors.dividerStyle  = NSSplitViewDividerStyleThin;

    // Without these the panes keep the size they were created at and the window
    // grows a dead margin down its right edge.
    editorContainer.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    rackScroll.autoresizingMask      = NSViewWidthSizable;
    listScroll.autoresizingMask      = NSViewHeightSizable;

    [editors addSubview:editorContainer];
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

    [workspace adjustSubviews];
    [editors adjustSubviews];

    self.editorSelector = [NSSegmentedControl
        segmentedControlWithLabels:@[@"Piano Roll", @"Playlist", @"Mixer", @"Editor"]
                      trackingMode:NSSegmentSwitchTrackingSelectOne
                            target:self
                            action:@selector(editorChanged:)];
    self.editorSelector.selectedSegment = 0;
    self.editorSelector.frame = NSMakeRect(10, frame.size.height - toolbarHeight + 3, 320, 24);

    self.transportModeSelector = [NSSegmentedControl
        segmentedControlWithLabels:@[@"Pattern", @"Song"]
                      trackingMode:NSSegmentSwitchTrackingSelectOne
                            target:self
                            action:@selector(transportModeChanged:)];
    self.transportModeSelector.selectedSegment = 0;
    self.transportModeSelector.frame = NSMakeRect(340, frame.size.height - toolbarHeight + 3, 150, 24);

    self.editorSelector.autoresizingMask        = NSViewMinYMargin;
    self.transportModeSelector.autoresizingMask = NSViewMinYMargin;

    [content addSubview:self.editorSelector];
    [content addSubview:self.transportModeSelector];

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
    self.playlist.onChange    = changed;
    self.mixer.onChange       = changed;

    // A fader move is not a graph change: the value has already reached the
    // strip that is rendering, so recompiling would only reset every meter.
    self.mixer.onParameterChange = ^{
        [weakSelf refreshStatus];
    };

    self.mixer.stripLookup = ^engine::dsp::MixerStripNode*(unsigned long long nodeId) {
        return [weakSelf stripForMixerNode:nodeId];
    };

    self.channelRack.onSelectChannel = ^(unsigned long long selected) {
        [weakSelf selectChannel:selected];
    };

    self.patternList.onSelectPattern = ^(unsigned long long selected) {
        [weakSelf selectPattern:selected];
    };

    [self startAudio];

    __weak INCDAWAppDelegate* weakAudioSelf = self;
    self.pianoRoll.onTransportToggle   = ^{ [weakAudioSelf toggleTransport]; };
    self.channelRack.onTransportToggle = ^{ [weakAudioSelf toggleTransport]; };
    self.patternList.onTransportToggle = ^{ [weakAudioSelf toggleTransport]; };
    self.playlist.onTransportToggle    = ^{ [weakAudioSelf toggleTransport]; };
    self.mixer.onTransportToggle       = ^{ [weakAudioSelf toggleTransport]; };

    self.playlist.onSeekTick = ^(long long tick) { [weakAudioSelf seekToTick:tick]; };

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
    self.pianoRoll.channelIdValue           = channelId;
    self.channelRack.selectedChannelIdValue = channelId;
    self.mixer.selectedChannelIdValue       = channelId;

    [self.pianoRoll requestRedraw];
    [self refreshStatus];
}

/// Switches the pattern both editors and the transport are working on.
- (void)selectPattern:(unsigned long long)patternId
{
    self.pianoRoll.patternIdValue           = patternId;
    self.channelRack.patternIdValue         = patternId;
    self.patternList.selectedPatternIdValue = patternId;
    self.playlist.patternIdValue            = patternId;

    if (const project::Pattern* pattern = _project->findPattern(project::EntityId{patternId}))
        self.window.title = [NSString stringWithFormat:@"INCDAW — %s", pattern->name.c_str()];

    // The graph plays one pattern at a time until the playlist exists (Phase 9),
    // so switching pattern is a recompile, not just a view change.
    [self rebuildGraph];
    [self retargetLoop];
    [self.pianoRoll requestRedraw];
    [self refreshStatus];
}

- (void)showPianoRoll:(id)sender
{
    (void)sender;
    self.editorSelector.selectedSegment = 0;
    [self showEditorAtSegment:0];
}

- (void)showPlaylist:(id)sender
{
    (void)sender;
    self.editorSelector.selectedSegment = 1;
    [self showEditorAtSegment:1];
}

- (void)togglePlayback:(id)sender
{
    (void)sender;
    [self toggleTransport];
}

- (void)showMixer:(id)sender
{
    (void)sender;
    self.editorSelector.selectedSegment = 2;
    [self showEditorAtSegment:2];
}

- (void)usePatternMode:(id)sender
{
    (void)sender;
    self.transportModeSelector.selectedSegment = 0;
    [self transportModeChanged:self.transportModeSelector];
}

- (void)useSongMode:(id)sender
{
    (void)sender;
    self.transportModeSelector.selectedSegment = 1;
    [self transportModeChanged:self.transportModeSelector];
}

- (void)editorChanged:(NSSegmentedControl*)sender
{
    [self showEditorAtSegment:sender.selectedSegment];
}

- (void)showEditorAtSegment:(NSInteger)segment
{
    self.pianoRoll.hidden   = segment != 0;
    self.playlist.hidden    = segment != 1;
    self.mixer.hidden       = segment != 2;
    self.audioEditor.hidden = segment != 3;

    NSView* focused = self.pianoRoll;
    if (segment == 1)
        focused = self.playlist;
    else if (segment == 2)
        focused = self.mixer;
    else if (segment == 3)
        focused = self.audioEditor;

    [self.window makeFirstResponder:focused];

    [self.playlist setNeedsDisplay:YES];
    [self.mixer setNeedsDisplay:YES];
    [self.audioEditor setNeedsDisplay:YES];
    [self.pianoRoll requestRedraw];
}

- (void)showAudioEditor:(id)sender
{
    (void)sender;
    self.editorSelector.selectedSegment = 3;
    [self showEditorAtSegment:3];
}

- (void)openAudioAssetInEditor:(unsigned long long)assetId
{
    self.audioEditor.assetIdValue = assetId;
    [self.audioEditor reloadWaveform];
    [self showAudioEditor:nil];
}

/// Runs one destructive edit on the editor's selection (or the whole file
/// when nothing is selected — the Edison convention), then refreshes
/// everything the file feeds: the waveform, the playback graph, the playlist.
- (void)applyAudioEdit:(app::AudioEditOp)op factor:(engine::Sample)factor
{
    if (self.audioEditor.assetIdValue == 0)
        return;

    const project::EntityId asset{self.audioEditor.assetIdValue};

    engine::edits::Region region;
    if (self.audioEditor.hasSelection) {
        region.from = self.audioEditor.selectionFrom;
        region.to   = self.audioEditor.selectionTo;
    } else {
        region.from = 0;
        region.to   = std::numeric_limits<engine::FrameCount>::max();   // clamped by the command
    }

    (void)_registry->execute(
        std::make_unique<app::EditAssetRegionCommand>(asset, region, op, factor));

    [self audioAssetChanged];
}

- (void)audioAssetChanged
{
    [self.audioEditor reloadWaveform];
    [self.playlist invalidateWaveformCache];
    [self rebuildGraph];
    [self.playlist setNeedsDisplay:YES];
    [self refreshStatus];
}

- (void)editNormalize:(id)sender { (void)sender; [self applyAudioEdit:app::AudioEditOp::normalize factor:1.0f]; }
- (void)editReverse:(id)sender   { (void)sender; [self applyAudioEdit:app::AudioEditOp::reverse factor:1.0f]; }
- (void)editSilence:(id)sender   { (void)sender; [self applyAudioEdit:app::AudioEditOp::silence factor:1.0f]; }
- (void)editFadeIn:(id)sender    { (void)sender; [self applyAudioEdit:app::AudioEditOp::fadeIn factor:1.0f]; }
- (void)editFadeOut:(id)sender   { (void)sender; [self applyAudioEdit:app::AudioEditOp::fadeOut factor:1.0f]; }
- (void)editGainUp:(id)sender    { (void)sender; [self applyAudioEdit:app::AudioEditOp::gain factor:1.412538f]; }   // +3 dB
- (void)editGainDown:(id)sender  { (void)sender; [self applyAudioEdit:app::AudioEditOp::gain factor:0.707946f]; }   // -3 dB

// ── Automation write mode ────────────────────────────────────────────────────

- (void)automationParameterEdited:(unsigned long long)nodeId
                              key:(const char*)key
                            value:(double)normalized
{
    if (!_autoWrite.isEnabled() || !_audioReady || !_audio->transport().isPlaying())
        return;

    _autoWrite.capture(project::EntityId{nodeId}, key,
                       _audio->transport().positionInTicks(), normalized);
}

- (void)toggleAudioLogger:(NSMenuItem*)sender
{
    if (!_audioReady)
        return;

    auto& logger = _audio->logger();
    logger.setEnabled(!logger.isEnabled());
    sender.state = logger.isEnabled() ? NSControlStateValueOn : NSControlStateValueOff;
    [self refreshStatus];
}

- (void)grabAudioLog:(id)sender
{
    (void)sender;

    if (!_audioReady)
        return;

    engine::AudioFileData data;
    const auto frames = _audio->logger().grab(data);

    if (frames <= 0) {
        _lastRecordError = @"audio logger: nothing captured yet";
        [self refreshStatus];
        return;
    }

    NSString* music = [NSSearchPathForDirectoriesInDomains(NSMusicDirectory,
                                                           NSUserDomainMask, YES) firstObject];
    const std::filesystem::path directory =
        std::filesystem::path{music.UTF8String} / "INCDAW" / "Recordings";

    std::error_code code;
    std::filesystem::create_directories(directory, code);

    const auto now  = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm parts{};
    localtime_r(&now, &parts);

    char stamp[40];
    std::snprintf(stamp, sizeof(stamp), "log-%04d%02d%02d-%02d%02d%02d.wav",
                  parts.tm_year + 1900, parts.tm_mon + 1, parts.tm_mday,
                  parts.tm_hour, parts.tm_min, parts.tm_sec);

    const std::filesystem::path path = directory / stamp;

    if (const auto wrote = engine::WavFile::write(path, data); !wrote) {
        _lastRecordError = @(wrote.error.c_str());
        [self refreshStatus];
        return;
    }

    // The grab ENDS now: it lands so its last frame sits at the playhead.
    project::RecordingSession::Placement placement;
    placement.succeeded    = true;
    placement.path         = path;
    placement.frameCount   = frames;
    placement.channelCount = data.channelCount;
    placement.sampleRate   = data.sampleRate;
    placement.startFrame   =
        std::max<engine::FramePosition>(0, _audio->transport().position() - frames);

    (void)_registry->execute(std::make_unique<app::InsertRecordedTakeCommand>(placement));

    [self audioAssetChanged];
}

- (void)togglePunch:(NSMenuItem*)sender
{
    _recording.setPunchToLoop(!_recording.punchToLoop());
    sender.state = _recording.punchToLoop() ? NSControlStateValueOn : NSControlStateValueOff;
}

- (void)toggleInputMonitoring:(NSMenuItem*)sender
{
    if (!_audioReady)
        return;

    if (_audio->isMonitoringEnabled()) {
        _audio->setMonitoringEnabled(false);
        sender.state = NSControlStateValueOff;
    } else {
        if (![self ensureInputOpen]) {
            [self refreshStatus];
            return;
        }

        _audio->setMonitoringEnabled(true);
        sender.state = NSControlStateValueOn;
    }

    // The toggle is a topology change: the monitor node exists exactly when
    // monitoring is on.
    [self rebuildGraph];
    [self refreshStatus];
}

- (void)toggleAutomationWrite:(NSMenuItem*)sender
{
    if (_autoWrite.isEnabled()) {
        _autoWrite.setEnabled(false);
        sender.state = NSControlStateValueOff;

        // Land every touched parameter's pass. Each is its own undo entry,
        // which is what a user riding two faders expects Cmd+Z to peel back.
        for (auto& command : _autoWrite.finish())
            (void)_registry->execute(std::move(command));

        [self rebuildGraph];
        [self.playlist setNeedsDisplay:YES];
    } else {
        _autoWrite.setEnabled(true);
        sender.state = NSControlStateValueOn;
    }

    [self refreshStatus];
}

- (void)editTrim:(id)sender
{
    (void)sender;

    // Trim is the one verb that requires a selection: "trim to everything"
    // is not an edit, and the command would refuse it anyway.
    if (self.audioEditor.assetIdValue == 0 || !self.audioEditor.hasSelection)
        return;

    const project::EntityId asset{self.audioEditor.assetIdValue};

    (void)_registry->execute(std::make_unique<app::TrimAssetCommand>(
        asset, engine::edits::Region{self.audioEditor.selectionFrom,
                                     self.audioEditor.selectionTo}));

    [self audioAssetChanged];
}

/// The strip currently rendering a mixer node, or nullptr. Valid only until the
/// next rebuild, which is why the mixer asks each time rather than caching.
- (engine::dsp::MixerStripNode*)stripForMixerNode:(unsigned long long)nodeId
{
    return _live.stripFor(project::EntityId{nodeId});
}

/// Pattern mode loops the selected pattern; song mode plays the arrangement.
///
/// This is a compile-time distinction, not a UI one: the graph is rebuilt with
/// a different source, so the audio thread never learns there are two modes.
- (void)transportModeChanged:(NSSegmentedControl*)sender
{
    _songMode = sender.selectedSegment == 1;

    const BOOL wasPlaying = _audioReady && _audio->transport().isPlaying();
    if (wasPlaying)
        _audio->transport().stop();

    [self rebuildGraph];
    [self retargetLoop];

    if (wasPlaying) {
        _audio->transport().seek(0);
        _audio->transport().play();
    }

    [self refreshStatus];
}

- (void)seekToTick:(long long)tick
{
    if (!_audioReady)
        return;

    _audio->transport().seekToTick(std::max<engine::Tick>(0, static_cast<engine::Tick>(tick)));
    [self refreshStatus];
}

- (void)startAudio
{
    _audio = std::make_unique<engine::AudioEngine>();

    platform::AudioDeviceConfig config;
    config.sampleRate     = 48000.0;

    // 512 rather than the lowest the hardware allows: this is set on the SHARED
    // device, and Bluetooth outputs cannot sustain small blocks without
    // crackling. Latency tuning belongs in audio settings (Phase 18), per
    // device, not in a hardcoded aggressive default.
    config.bufferSize     = 512;
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
    if (!_audioReady || (_project->patterns().empty() && !_songMode))
        return;

    project::GraphCompileOptions options;
    options.sampleRate   = _audio->sampleRate();

    // The device's maximum, not its current setting: a shared device delivers
    // whatever block CoreAudio is servicing, and a graph compiled smaller than
    // that renders the first part of each block and leaves the rest silent —
    // which the ear hears as a buzz.
    options.maxBlockSize = _audio->maxServiceableBlockSize();
    options.channelCount = _audio->outputChannels();
    options.source       = _songMode ? project::PlaybackSource::arrangement
                                     : project::PlaybackSource::pattern;
    options.pattern      = project::EntityId{self.pianoRoll.patternIdValue};
    options.diskStreamer = _diskStreamer.get();

    if (_audio->isMonitoringEnabled() && _audio->inputChannels() > 0) {
        options.monitorRing         = _audio->monitorRing();
        options.monitorChannelCount = _audio->inputChannels();
    }

    // Hosted plugins reach the graph through this factory and nothing else:
    // `project/` compiles the topology without knowing what a CLAP is
    // (docs/DECISIONS.md D-028). The manager outlives every graph it feeds.
    plugins::PluginInstanceManager* instances  = _pluginInstances.get();
    project::ParameterRegistry*     parameters = &_parameters;
    const double                    sampleRate = _audio->sampleRate();
    const auto maxFrames = static_cast<std::uint32_t>(_audio->maxServiceableBlockSize());

    options.parameters    = parameters;
    options.insertFactory = [instances, parameters, sampleRate, maxFrames](
                                const project::PluginSlot& slot,
                                std::string& error) -> std::unique_ptr<engine::Node> {
        if (instances == nullptr) {
            error = "plugin host unavailable";
            return nullptr;
        }

        auto node = instances->createInsert(slot.plugin, sampleRate, maxFrames, error);

        // Discovery lands in the registry HERE — between the instance being
        // created and automation lanes binding later in the same compile.
        // Registration is all it takes to make the plugin's parameters
        // automatable (docs/PLUGIN_HOST.md §5), and re-registering on every
        // rebuild is idempotent by the registry's replace-on-same-key rule.
        if (node != nullptr)
            if (const auto* discovered = instances->parametersFor(slot.plugin.uid))
                parameters->registerPluginParameters(slot.plugin.uid, *discovered);

        return node;
    };

    auto compiled = project::compileProjectGraph(*_project, _audio->transport().tempoMap(), options);
    if (!compiled) {
        // A routing cycle lands here. Reported rather than swallowed: the
        // previous graph keeps playing, and the user is told why their edit did
        // not take effect.
        NSLog(@"INCDAW: graph rebuild failed: %s", compiled.error.c_str());
        _lastGraphError = @(compiled.error.c_str());
        return;
    }

    _lastGraphError = nil;

    _audio->setGraph(std::move(compiled.graph));

    // The handles outlive the unique_ptr that was moved out: the nodes belong
    // to the graph the engine now owns.
    _live = std::move(compiled);
}

/// Loops whatever pattern is selected, so playback repeats rather than running
/// off into silence — and so that switching pattern while playing does not keep
/// looping the length of the previous one.
- (void)retargetLoop
{
    if (!_audioReady)
        return;

    auto& transport = _audio->transport();

    Tick length = 0;

    if (_songMode) {
        // The song loops over what was actually drawn. An empty arrangement
        // falls back to the pattern's length rather than to a zero-length loop,
        // which would sit at frame zero and play nothing forever.
        length = project::arrangementLengthTicks(*_project);
    }

    if (length <= 0) {
        const project::Pattern* pattern =
            _project->findPattern(project::EntityId{self.pianoRoll.patternIdValue});

        length = pattern != nullptr && pattern->length > 0 ? pattern->length
                                                           : ticksPerQuarterNote * 8;
    }

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

/// Opens the input device on demand. The engine starts output-only — the
/// microphone is never opened unasked — so the first record arm reopens the
/// device pair with input, which is also when macOS shows its permission
/// prompt: at the moment the user pressed record, when it makes sense.
- (BOOL)ensureInputOpen
{
    if (_audio->inputChannels() > 0)
        return YES;

    const BOOL wasPlaying = _audio->transport().isPlaying();
    if (wasPlaying)
        _audio->transport().stop();

    _audio->stop();

    platform::AudioDeviceConfig config;
    config.sampleRate            = 48000.0;
    config.bufferSize            = 512;
    config.outputChannels        = 2;
    config.inputDeviceIdentifier = platform::AudioDeviceConfig::defaultInput;

    std::string error;
    if (!_audio->start(config, error)) {
        // The input could not open (a Bluetooth HFP microphone at 24 kHz lands
        // here). Reopen without input so playback keeps working, and say why.
        NSLog(@"INCDAW: input unavailable: %s", error.c_str());
        _lastRecordError = [NSString stringWithFormat:@"input: %s", error.c_str()];

        config.inputDeviceIdentifier.clear();
        std::string fallback;
        if (!_audio->start(config, fallback)) {
            NSLog(@"INCDAW: audio restart failed: %s", fallback.c_str());
            _audioReady = NO;
            return NO;
        }

        [self rebuildGraph];
        return NO;
    }

    [self rebuildGraph];

    if (wasPlaying) {
        [self retargetLoop];
        _audio->transport().play();
    }

    return YES;
}

- (void)toggleRecord:(id)sender
{
    (void)sender;

    if (!_audioReady)
        return;

    if (_recording.isRecording()) {
        const auto placement = _recording.finish(*_audio);

        if (!placement.succeeded) {
            NSLog(@"INCDAW: recording failed: %s", placement.error.c_str());
            _lastRecordError = @(placement.error.c_str());
        } else if (placement.frameCount > 0) {
            if (placement.droppedFrames > 0) {
                NSLog(@"INCDAW: take has %llu dropped frames",
                      static_cast<unsigned long long>(placement.droppedFrames));
                _lastRecordError = [NSString
                    stringWithFormat:@"take damaged: %llu dropped frames",
                                     static_cast<unsigned long long>(placement.droppedFrames)];
            }

            (void)_registry->execute(
                std::make_unique<app::InsertRecordedTakeCommand>(placement));

            // The clip is data in song mode's graph; in pattern mode it is
            // visible in the playlist and will play when the mode switches.
            [self rebuildGraph];
            [self.playlist setNeedsDisplay:YES];
        }
    } else {
        _lastRecordError = nil;

        if (![self ensureInputOpen]) {
            [self refreshStatus];
            return;
        }

        NSString* music = [NSSearchPathForDirectoriesInDomains(NSMusicDirectory,
                                                               NSUserDomainMask, YES) firstObject];
        const std::filesystem::path directory =
            std::filesystem::path{music.UTF8String} / "INCDAW" / "Recordings";

        std::string error;
        if (!_recording.arm(*_audio, directory, error)) {
            NSLog(@"INCDAW: cannot record: %s", error.c_str());
            _lastRecordError = @(error.c_str());
        }
    }

    [self refreshStatus];
}

- (void)housekeeping
{
    if (!_audioReady)
        return;

    _audio->collectRetiredGraphs();

    // An undo or redo may have rewritten the file the editor and the
    // playlist's clip waveforms are showing.
    if (_registry->undoDepth() != _undoDepthSeen) {
        _undoDepthSeen = _registry->undoDepth();

        [self.playlist invalidateWaveformCache];

        if (!self.audioEditor.hidden && self.audioEditor.assetIdValue != 0) {
            [self.audioEditor reloadWaveform];
            [self rebuildGraph];
        }
    }

    const auto position = _audio->transport().position();
    const long long tick = _audio->transport().isPlaying()
                               ? _audio->transport().tempoMap().tickForFrame(position)
                               : -1;

    self.pianoRoll.playheadTick   = tick;
    self.channelRack.playheadTick = tick;
    self.playlist.playheadTick    = tick;

    [self.pianoRoll requestRedraw];

    // Meters move whether or not anything was edited, so the mixer redraws with
    // the playhead rather than only on change.
    if (!self.mixer.hidden)
        [self.mixer setNeedsDisplay:YES];

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

    NSString* audio = _lastGraphError != nil
                          ? [NSString stringWithFormat:@"⚠ %@", _lastGraphError]
                          : @"audio: unavailable";
    if (_audioReady && _lastGraphError == nil) {
        const bool playing = _audio->transport().isPlaying();
        audio = [NSString stringWithFormat:@"%@ · %.0f%% cpu",
                 playing ? @"▶ playing" : @"■ stopped",
                 _audio->profiler().peakLoad() * 100.0];

        if (_recording.isRecording()) {
            const double seconds = _audio->sampleRate() > 0.0
                ? static_cast<double>(_recording.capturedFrames()) / _audio->sampleRate()
                : 0.0;
            audio = [NSString stringWithFormat:@"● REC %.1fs · %@", seconds, audio];
        } else if (_lastRecordError != nil) {
            audio = [NSString stringWithFormat:@"⚠ %@ · %@", _lastRecordError, audio];
        }
    }

    const project::Channel* channel =
        _project->findChannel(project::EntityId{self.pianoRoll.channelIdValue});

    self.statusField.stringValue = [NSString stringWithFormat:
        @"INCDAW %s  ·  %@  ·  %s / %s  ·  %lu notes  ·  %lu clips  ·  %@  ·  %@  ·  "
        @"space: play   r: rec   ⌘Z: undo",
        app::Version::string(),
        _songMode ? @"song" : @"pattern",
        pattern != nullptr ? pattern->name.c_str() : "—",
        channel != nullptr ? channel->name.c_str() : "—",
        static_cast<unsigned long>(noteCount),
        static_cast<unsigned long>(_project->clips().size()), audio, undo];
}

- (void)buildMenu
{
    NSMenu* menuBar = [[NSMenu alloc] init];

    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [menuBar addItem:appItem];

    NSMenu* appMenu = [[NSMenu alloc] init];
    [appMenu addItemWithTitle:@"Quit INCDAW" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;

    // A View menu, so the panes are reachable by keyboard as well as by the
    // toolbar. Every DAW binds its editors to keys; a workspace you can only
    // reach with the mouse is one you stop using.
    NSMenuItem* viewItem = [[NSMenuItem alloc] init];
    [menuBar addItem:viewItem];

    NSMenu* viewMenu = [[NSMenu alloc] initWithTitle:@"View"];

    NSMenuItem* pianoRollItem = [viewMenu addItemWithTitle:@"Piano Roll"
                                                    action:@selector(showPianoRoll:)
                                             keyEquivalent:@"1"];
    pianoRollItem.target = self;

    NSMenuItem* playlistItem = [viewMenu addItemWithTitle:@"Playlist"
                                                   action:@selector(showPlaylist:)
                                            keyEquivalent:@"2"];
    playlistItem.target = self;

    NSMenuItem* mixerItem = [viewMenu addItemWithTitle:@"Mixer"
                                                action:@selector(showMixer:)
                                         keyEquivalent:@"3"];
    mixerItem.target = self;

    NSMenuItem* audioEditorItem = [viewMenu addItemWithTitle:@"Audio Editor"
                                                      action:@selector(showAudioEditor:)
                                               keyEquivalent:@"6"];
    audioEditorItem.target = self;

    [viewMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* patternModeItem = [viewMenu addItemWithTitle:@"Pattern Mode"
                                                      action:@selector(usePatternMode:)
                                               keyEquivalent:@"4"];
    patternModeItem.target = self;

    NSMenuItem* songModeItem = [viewMenu addItemWithTitle:@"Song Mode"
                                                   action:@selector(useSongMode:)
                                            keyEquivalent:@"5"];
    songModeItem.target = self;

    [viewMenu addItem:[NSMenuItem separatorItem]];

    // Space toggles the transport from every pane, but a menu entry is what
    // makes it discoverable — and what lets anything driving the app through
    // the accessibility API start playback.
    NSMenuItem* playItem = [viewMenu addItemWithTitle:@"Play / Stop"
                                               action:@selector(togglePlayback:)
                                        keyEquivalent:@""];
    playItem.target = self;

    // Plain R, no modifier: record is a transport control, and transport
    // controls live on bare keys in every DAW. The menu route means it works
    // whichever pane has focus.
    NSMenuItem* recordItem = [viewMenu addItemWithTitle:@"Record"
                                                 action:@selector(toggleRecord:)
                                          keyEquivalent:@"r"];
    recordItem.keyEquivalentModifierMask = 0;
    recordItem.target = self;

    viewItem.submenu = viewMenu;

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

    // The audio editor's verbs. Menu-only for now: bare keys belong to the
    // transport and Cmd-keys to project edits, and the command-registry
    // shortcut work (CLAUDE.md §26) is where these get keys properly.
    NSMenuItem* audioItem = [[NSMenuItem alloc] init];
    [menuBar addItem:audioItem];

    NSMenu* audioMenu = [[NSMenu alloc] initWithTitle:@"Audio"];

    const struct { NSString* title; SEL action; } verbs[] = {
        {@"Trim to Selection", @selector(editTrim:)},
        {@"Normalize",         @selector(editNormalize:)},
        {@"Reverse",           @selector(editReverse:)},
        {@"Silence",           @selector(editSilence:)},
        {@"Fade In",           @selector(editFadeIn:)},
        {@"Fade Out",          @selector(editFadeOut:)},
        {@"Gain +3 dB",        @selector(editGainUp:)},
        {@"Gain -3 dB",        @selector(editGainDown:)},
    };

    for (const auto& verb : verbs) {
        NSMenuItem* item = [audioMenu addItemWithTitle:verb.title
                                                action:verb.action
                                         keyEquivalent:@""];
        item.target = self;
    }

    [audioMenu addItem:[NSMenuItem separatorItem]];

    // Write-mode automation recording: while checked and the transport rolls,
    // every fader and pan move in the mixer is captured; unchecking lands the
    // passes as automation lanes (and clips, when the lane is new).
    NSMenuItem* writeItem = [audioMenu addItemWithTitle:@"Write Automation"
                                                 action:@selector(toggleAutomationWrite:)
                                          keyEquivalent:@""];
    writeItem.target = self;

    NSMenuItem* monitorItem = [audioMenu addItemWithTitle:@"Monitor Input"
                                                   action:@selector(toggleInputMonitoring:)
                                            keyEquivalent:@""];
    monitorItem.target = self;

    // Punch: with loop recording, only audio inside the loop range lands.
    NSMenuItem* punchItem = [audioMenu addItemWithTitle:@"Punch to Loop Range"
                                                 action:@selector(togglePunch:)
                                          keyEquivalent:@""];
    punchItem.target = self;

    [audioMenu addItem:[NSMenuItem separatorItem]];

    // The Audio Logger: the master's last minute, retrievable after the
    // fact. Off by default — 23 MB and a "was that being kept?" question
    // the user should answer, not inherit.
    NSMenuItem* loggerItem = [audioMenu addItemWithTitle:@"Audio Logger"
                                                  action:@selector(toggleAudioLogger:)
                                           keyEquivalent:@""];
    loggerItem.target = self;

    NSMenuItem* grabItem = [audioMenu addItemWithTitle:@"Grab Last 60 Seconds"
                                                action:@selector(grabAudioLog:)
                                         keyEquivalent:@""];
    grabItem.target = self;

    audioItem.submenu = audioMenu;

    NSApp.mainMenu = menuBar;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    (void)notification;

    // Stop the device before the graph it is reading goes away. A recording
    // in flight is finished first — quitting must not truncate a take's file.
    [_housekeeping invalidate];
    _housekeeping = nil;

    if (_audio != nullptr) {
        if (_recording.isRecording())
            (void)_recording.finish(*_audio);

        _audio->stop();
    }
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
