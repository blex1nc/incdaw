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
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include "app/AppSettings.h"
#include "app/AutomationWriteSession.h"
#include "app/BrowserModel.h"
#include "app/CommandRegistry.h"
#include "app/Version.h"
#include "app/commands/MidiMappingCommands.h"
#include "app/commands/RecordingCommands.h"
#include "engine/AudioEngine.h"
#include "platform/MidiDevice.h"
#include "platform/SystemInfo.h"
#include "plugins/PluginInstanceManager.h"
#include "plugins/PluginRegistry.h"
#include "project/MidiFile.h"
#include "project/Model.h"
#include "project/OfflineRender.h"
#include "project/PatternCompiler.h"
#include "project/PluginStateFiles.h"
#include "project/ProjectFile.h"
#include "project/ProjectGraphCompiler.h"
#include "project/RecordingSession.h"
#include "app/commands/AudioEditCommands.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/MacroCommand.h"
#include "app/commands/SamplerCommands.h"
#include "app/commands/PatternCommands.h"
#include "app/commands/TempoCommands.h"
#include "app/commands/TrackCommands.h"
#include "ui/macos/BrowserView.h"
#include "ui/macos/CommandPalette.h"
#include "ui/macos/AudioEditorView.h"
#include "ui/macos/ChannelRackView.h"
#include "ui/macos/PatternListView.h"
#include "ui/macos/PianoRollView.h"
#include "ui/macos/MixerView.h"
#include "ui/macos/PlaylistView.h"
#include "ui/macos/SettingsWindow.h"

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
@property (strong) INCDAWSettingsWindow*    settingsWindow;
@property (strong) INCDAWBrowserView*       browser;
@property (strong) NSMenu*                  recentMenu;
@property (strong) INCDAWCommandPalette*    palette;
@property (strong) NSTextField*             tempoField;
@property (strong) NSTextField*             signatureField;
@property (strong) NSSplitView*             workspaceSplit;
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

    /// Decoded audio shared across graph rebuilds (docs/DECISIONS.md D-032):
    /// without it, every edit on a sampler channel would re-decode its
    /// samples. Graphs hold shared_ptrs into it, so entries can never vanish
    /// under a live graph.
    std::unique_ptr<engine::SampleCache> _sampleCache;

    /// MIDI learn: armed by the mixer's context menu, resolved by
    /// housekeeping when the next CC arrives at the input tap.
    bool          _learnArmed;
    NSString*     _learnParameterKey;
    unsigned long long _learnTarget;
    std::uint64_t _learnControlSeen;

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

    /// Where the project lives on disk. Empty until the first Save As or
    /// Open; Save falls back to Save As while it is.
    std::filesystem::path _projectPath;

    /// Preferences that belong to the machine rather than to the project:
    /// which interface plays, at which rate and block size, which MIDI sources
    /// are connected (app/AppSettings.h). Loaded before the device opens.
    app::AppSettings      _settings;
    std::filesystem::path _settingsPath;

    /// What the device is currently open with. Kept because reopening it —
    /// around a tempo-map swap, say — must not silently change the format the
    /// session has been running at.
    platform::AudioDeviceConfig _activeConfig;

    /// The browser's libraries, favourites and search (app/BrowserModel.h).
    /// Owned here rather than by the view, because the shell is what persists
    /// it and what decides what opening a file means.
    app::BrowserModel _browserModel;

    /// The MIDI client. Owned by the shell because the settings window decides
    /// which sources it connects to; the engine only receives the messages.
    std::unique_ptr<platform::MidiDevice> _midiDevice;
    NSString*                             _lastMidiError;

    /// What the settings window says instead of the device line: a refused
    /// device, or a change that a running take is holding up.
    NSString* _lastSettingsMessage;

    /// The scanned plugin catalogue as menu fodder, built once at launch.
    NSArray<NSDictionary*>* _availableInserts;

    /// Open plugin editor windows and their close observers, by slot key.
    /// Closed by the shell BEFORE an instance is disposed (D-031): the
    /// window's death must reach the plugin while the plugin is still alive.
    NSMutableDictionary<NSNumber*, NSWindow*>* _editorWindows;
    NSMutableDictionary<NSNumber*, id>*        _editorObservers;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    _project      = std::make_unique<project::Project>();
    _registry     = std::make_unique<app::CommandRegistry>(*_project);
    _diskStreamer = std::make_unique<engine::DiskStreamer>();
    _sampleCache  = std::make_unique<engine::SampleCache>();

    // The plugin catalogue is read from a file; launching touches no plugin
    // binary at all, because startup time must not scale with the size of a
    // plugin collection (docs/PLUGIN_HOST.md §3). A missing file is the normal
    // first-run state, not an error.
    if (const std::filesystem::path support = incdawSupportDirectory(); !support.empty()) {
        (void)_pluginRegistry.load(support / "plugins.tsv");

        // Read before the device opens: the settings file is what decides
        // which device that is. A missing or unreadable file is the normal
        // first run and yields the same defaults the shell used to hardcode.
        _settingsPath = support / "settings.json";
        _settings     = app::AppSettings::load(_settingsPath);
    }

    [self adoptBrowserSettings];
    [self registerActions];

    _pluginInstances = std::make_unique<plugins::PluginInstanceManager>(_pluginRegistry);
    _parameters      = project::ParameterRegistry::withBuiltins();

    // Builtin effect and instrument parameters register once at launch
    // through the same path a scanned plugin's discovery does; there is
    // nothing to rescan.
    _parameters.registerBuiltinEffects();
    _parameters.registerBuiltinInstruments();

    // What the mixer's Add Insert menu offers: every scanned, non-blacklisted
    // plugin, by display name. The view knows menus, not catalogues.
    NSMutableArray<NSDictionary*>* available = [NSMutableArray array];
    for (const plugins::PluginRegistry::Located& located : _pluginRegistry.plugins()) {
        [available addObject:@{
            @"uid":  @(located.plugin->id.c_str()),
            @"name": @(located.plugin->name.empty() ? located.plugin->id.c_str()
                                                    : located.plugin->name.c_str()),
        }];
    }
    _availableInserts = available;
    _editorWindows    = [NSMutableDictionary dictionary];
    _editorObservers  = [NSMutableDictionary dictionary];

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

    // Where the window was left. A DAW window is arranged around the work —
    // second display, particular height — and re-centring it every launch
    // undoes that arrangement daily.
    const app::AppSettings::Workspace& saved = _settings.workspace;
    if (saved.windowWidth > 200.0 && saved.windowHeight > 200.0) {
        const NSRect restored = NSMakeRect(saved.windowX, saved.windowY,
                                           saved.windowWidth, saved.windowHeight);

        // Only if it still lands on a screen: a window restored onto a display
        // that is no longer attached is a window the user cannot reach.
        BOOL visible = NO;
        for (NSScreen* screen in [NSScreen screens])
            visible = visible || NSIntersectsRect(restored, screen.visibleFrame);

        if (visible)
            [self.window setFrame:restored display:NO];
        else
            [self.window center];
    } else {
        [self.window center];
    }

    NSView* content = self.window.contentView;

    constexpr CGFloat statusHeight  = 26.0;
    constexpr CGFloat toolbarHeight = 30.0;
    constexpr CGFloat listWidth     = 150.0;
    constexpr CGFloat rackHeight    = 220.0;
    constexpr CGFloat browserWidth  = 210.0;

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
    self.mixer.availableInserts       = _availableInserts;

    __weak INCDAWAppDelegate* weakSelfForEditors = self;
    self.mixer.onOpenInsertEditor = ^(unsigned long long slotKey) {
        [weakSelfForEditors openEditorForSlotKey:slotKey];
    };

    self.mixer.onMidiLearn = ^(NSString* parameterKey, unsigned long long targetId) {
        [weakSelfForEditors armMidiLearnForKey:parameterKey target:targetId];
    };

    self.mixer.onMidiForget = ^(unsigned long long targetId) {
        [weakSelfForEditors forgetMidiMappingsForTarget:targetId];
    };

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

    // The browser sits outside the project: it is the one pane that shows the
    // file system rather than the song, which is why it is leftmost and why it
    // survives switching editors.
    self.browser = [[INCDAWBrowserView alloc]
        initWithFrame:NSMakeRect(0, 0, browserWidth, body.size.height)
              browser:&_browserModel];

    self.browser.autoresizingMask = NSViewHeightSizable;

    __weak INCDAWAppDelegate* weakSelfForBrowser = self;
    self.browser.onActivateFile    = ^(NSString* path) { [weakSelfForBrowser openBrowsedFile:path]; };
    self.browser.onLibraryChanged  = ^{ [weakSelfForBrowser storeBrowserSettings]; };
    self.browser.onTransportToggle = ^{ [weakSelfForBrowser toggleTransport]; };

    NSSplitView* workspace = [[NSSplitView alloc] initWithFrame:body];
    workspace.vertical         = YES;
    workspace.dividerStyle     = NSSplitViewDividerStyleThin;
    workspace.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [workspace addSubview:self.browser];
    [workspace addSubview:listScroll];
    [workspace addSubview:editors];

    self.workspaceSplit = workspace;

    [content addSubview:workspace];

    [workspace setPosition:browserWidth ofDividerAtIndex:0];
    [workspace setPosition:browserWidth + listWidth ofDividerAtIndex:1];
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

    // Tempo and time signature. Until these existed the tempo map could only
    // be set by loading a project — the model carried it and the engine
    // honoured it, but nothing in the application could change it.
    NSTextField* tempoLabel = [NSTextField labelWithString:@"BPM"];
    tempoLabel.frame     = NSMakeRect(505, frame.size.height - toolbarHeight + 6, 32, 18);
    tempoLabel.font      = [NSFont systemFontOfSize:11.0];
    tempoLabel.textColor = [NSColor colorWithCalibratedWhite:0.55 alpha:1.0];
    tempoLabel.autoresizingMask = NSViewMinYMargin;
    [content addSubview:tempoLabel];

    self.tempoField = [[NSTextField alloc]
        initWithFrame:NSMakeRect(537, frame.size.height - toolbarHeight + 4, 62, 22)];
    self.tempoField.font      = [NSFont monospacedDigitSystemFontOfSize:11.0
                                                                 weight:NSFontWeightRegular];
    self.tempoField.alignment = NSTextAlignmentRight;
    self.tempoField.target    = self;
    self.tempoField.action    = @selector(tempoEdited:);
    self.tempoField.autoresizingMask = NSViewMinYMargin;
    [content addSubview:self.tempoField];

    NSTextField* signatureLabel = [NSTextField labelWithString:@"Sig"];
    signatureLabel.frame     = NSMakeRect(609, frame.size.height - toolbarHeight + 6, 24, 18);
    signatureLabel.font      = [NSFont systemFontOfSize:11.0];
    signatureLabel.textColor = [NSColor colorWithCalibratedWhite:0.55 alpha:1.0];
    signatureLabel.autoresizingMask = NSViewMinYMargin;
    [content addSubview:signatureLabel];

    self.signatureField = [[NSTextField alloc]
        initWithFrame:NSMakeRect(633, frame.size.height - toolbarHeight + 4, 52, 22)];
    self.signatureField.font      = [NSFont monospacedDigitSystemFontOfSize:11.0
                                                                     weight:NSFontWeightRegular];
    self.signatureField.alignment = NSTextAlignmentCenter;
    self.signatureField.target    = self;
    self.signatureField.action    = @selector(signatureEdited:);
    self.signatureField.autoresizingMask = NSViewMinYMargin;
    [content addSubview:self.signatureField];

    [self refreshTempoFields];

    self.editorSelector.autoresizingMask        = NSViewMinYMargin;
    self.transportModeSelector.autoresizingMask = NSViewMinYMargin;

    [content addSubview:self.editorSelector];
    [content addSubview:self.transportModeSelector];

    // The pane and the mode the session was left in. Applied through the same
    // selectors the user would have clicked, so nothing can restore into a
    // state the UI cannot reach.
    if (_settings.workspace.activeEditor > 0 && _settings.workspace.activeEditor < 4)
        [self showEditorAtSegment:_settings.workspace.activeEditor];

    self.editorSelector.selectedSegment = _settings.workspace.activeEditor;

    if (_settings.workspace.songMode) {
        _songMode = YES;
        self.transportModeSelector.selectedSegment = 1;
    }

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

// ── Browser ──────────────────────────────────────────────────────────────────

/// Libraries and favourites from the settings file, with a first-run default.
///
/// The default is the user's Music folder rather than nothing: a browser that
/// opens empty asks the user to configure it before it can be used once, and
/// the folder every Mac already has is the one place samples are likely to be.
- (void)adoptBrowserSettings
{
    std::vector<std::filesystem::path> roots;
    for (const std::string& entry : _settings.browser.roots)
        roots.emplace_back(entry);

    if (roots.empty() && !_settings.browser.seeded) {
        for (const NSSearchPathDirectory directory : {NSMusicDirectory, NSDownloadsDirectory}) {
            NSString* path = [NSSearchPathForDirectoriesInDomains(directory, NSUserDomainMask, YES)
                firstObject];

            if (path != nil)
                roots.emplace_back(path.UTF8String);
        }
    }

    _browserModel.setRoots(roots);

    std::vector<std::filesystem::path> starred;
    for (const std::string& entry : _settings.browser.favourites)
        starred.emplace_back(entry);

    _browserModel.setFavourites(starred);

    // Recorded immediately, so that removing every library is a choice that
    // survives the next launch rather than one the defaults undo.
    if (!_settings.browser.seeded) {
        _settings.browser.seeded = true;
        [self storeBrowserSettings];
    }
}

- (void)storeBrowserSettings
{
    _settings.browser.roots.clear();
    for (const app::BrowserModel::Root& root : _browserModel.roots())
        _settings.browser.roots.push_back(root.path.string());

    _settings.browser.favourites.clear();
    for (const std::filesystem::path& path : _browserModel.favouritePaths())
        _settings.browser.favourites.push_back(path.string());

    [self persistSettings];
}

/// What opening a file in the browser means, by kind.
///
/// The browser deliberately does not decide this: loading a sample onto a
/// channel, importing MIDI and opening a project are application gestures, and
/// only the shell knows which channel is selected or what happens to the
/// project that is already open.
- (void)openBrowsedFile:(NSString*)path
{
    const std::filesystem::path file{path.UTF8String};

    switch (app::BrowserModel::kindOf(file)) {
        case app::BrowserItemKind::audio:
            [self loadSampleIntoSelectedChannel:path];
            return;

        case app::BrowserItemKind::midi:
            [self importMidiFromPath:file];
            return;

        case app::BrowserItemKind::project:
            [self openProjectAtPath:file];
            return;

        case app::BrowserItemKind::folder:
        case app::BrowserItemKind::preset:
        case app::BrowserItemKind::other:
            break;
    }

    NSBeep();
}

/// Loads a sample onto the channel the editors are pointed at, or onto a new
/// channel when the rack is empty — one gesture, one undo entry either way.
- (void)loadSampleIntoSelectedChannel:(NSString*)path
{
    const std::string filePath = path.UTF8String;

    if (const project::Channel* selected =
            _project->findChannel(project::EntityId{self.pianoRoll.channelIdValue});
        selected != nullptr) {
        if (_registry->execute(std::make_unique<app::LoadSampleCommand>(selected->id, filePath)))
            [self projectEditedFromBrowser];

        return;
    }

    auto  add     = std::make_unique<app::AddChannelCommand>(
        std::filesystem::path{filePath}.stem().string());
    auto* added   = add.get();
    auto  gesture = std::make_unique<app::MacroCommand>("channel.dropSample", "Load Sample");

    gesture->add(std::move(add));
    gesture->addStep([added, filePath](project::Project&) -> app::CommandPtr {
        return std::make_unique<app::LoadSampleCommand>(added->channelId(), filePath);
    });

    if (!_registry->execute(std::move(gesture)))
        return;

    [self selectChannel:added->channelId().value()];
    [self projectEditedFromBrowser];
}

- (void)projectEditedFromBrowser
{
    [self.channelRack setNeedsDisplay:YES];
    [self.mixer setNeedsDisplay:YES];
    [self rebuildGraph];
}

// ── Tempo and time signature ─────────────────────────────────────────────────

- (void)refreshTempoFields
{
    const engine::TempoMap&      map       = _project->tempoMap();
    const engine::TimeSignature  signature = map.timeSignatureAtTick(0);

    self.tempoField.stringValue     = [NSString stringWithFormat:@"%.2f", map.tempoAtTick(0)];
    self.signatureField.stringValue = [NSString stringWithFormat:@"%d/%d",
                                                                 signature.numerator,
                                                                 signature.denominator];
}

- (void)tempoEdited:(NSTextField*)sender
{
    const double typed = sender.doubleValue;

    // Merging, so holding the field and typing several values is one undo
    // entry rather than one per keystroke that committed.
    if (_registry->executeMerging(std::make_unique<app::SetProjectTempoCommand>(typed)))
        [self applyProjectTempo];

    // Rewritten from the model, not from what was typed: the command clamps,
    // and a field showing 900 while the project plays at 400 is a lie.
    [self refreshTempoFields];
    [self refreshStatus];
}

- (void)signatureEdited:(NSTextField*)sender
{
    int numerator   = 0;
    int denominator = 0;

    if (std::sscanf(sender.stringValue.UTF8String, "%d/%d", &numerator, &denominator) == 2
        && app::SetTimeSignatureCommand::isValid(numerator, denominator)) {
        if (_registry->execute(std::make_unique<app::SetTimeSignatureCommand>(numerator, denominator)))
            [self applyProjectTempo];
    } else {
        NSBeep();
    }

    [self refreshTempoFields];
}

/// Publishes the project's tempo map to the transport, with the device stopped.
///
/// The transport's map is read on the AUDIO THREAD every block, and instrument
/// and automation nodes hold pointers into it
/// (engine/instrument/InstrumentNode.h, engine/transport/Transport.h §
/// tempoMapForEdit). Replacing it while the device runs is a data race, and a
/// race in the render path is never an acceptable price for avoiding a gap.
/// A tempo change is a rare, deliberate edit; the device restart it costs is
/// audible once and correct always.
///
/// A lock-free tempo-map swap — the same retire-and-reclaim treatment the
/// graph already gets — is the engine change that would remove the gap. It is
/// deliberately not being made here.
- (void)applyProjectTempo
{
    if (!_audioReady || _audio == nullptr)
        return;

    const engine::Tick position   = _audio->transport().positionInTicks();
    const BOOL         wasPlaying = _audio->transport().isPlaying();

    if (wasPlaying)
        _audio->transport().stop();

    _audio->stop();
    _audio->transport().tempoMapForEdit() = _project->tempoMap();

    std::string error;
    if (!_audio->start(_activeConfig, error)) {
        NSLog(@"INCDAW: could not reopen the device after a tempo change: %s", error.c_str());
        _audioReady     = NO;
        _lastGraphError = [NSString stringWithFormat:@"audio: %s", error.c_str()];
        return;
    }

    _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());

    [self rebuildGraph];
    [self retargetLoop];
    _audio->transport().seekToTick(position);

    if (wasPlaying)
        _audio->transport().play();

    [self.playlist setNeedsDisplay:YES];
    [self.pianoRoll requestRedraw];
}

// ── Commands ─────────────────────────────────────────────────────────────────

/// Project actions, registered by id.
///
/// CLAUDE.md §26: an action addressable by id is one a menu, a shortcut, a
/// controller mapping, the palette and a future script can all reach without
/// any of them knowing which view implements it. Until this existed the
/// registry's action table was empty in the running application — every edit
/// arrived as a command, but none of them had a name anything could look up.
- (void)registerActions
{
    app::CommandRegistry* registry = _registry.get();

    registry->registerAction({"channel.add", "Add Channel", "Channel Rack", "", [registry] {
        return std::make_unique<app::AddChannelCommand>(
            "Channel " + std::to_string(registry->project().channels().size() + 1));
    }});

    registry->registerAction({"pattern.add", "Add Pattern", "Patterns", "", [registry] {
        return std::make_unique<app::AddPatternCommand>(
            "Pattern " + std::to_string(registry->project().patterns().size() + 1));
    }});

    registry->registerAction({"track.add", "Add Track", "Playlist", "", [registry] {
        return std::make_unique<app::AddTrackCommand>(
            "Track " + std::to_string(registry->project().tracks().size() + 1));
    }});
}

/// Everything the palette can run, gathered fresh each time it opens.
///
/// The menu bar is walked rather than duplicated: an action that has a menu
/// entry is listed with the same title and the same shortcut, and can never
/// drift from it. Registered project actions come next, and undo/redo last —
/// those two are the only entries the palette synthesises, because the Edit
/// menu's items are handled by whichever pane has focus.
- (NSArray<INCDAWCommandEntry*>*)paletteEntries
{
    NSMutableArray<INCDAWCommandEntry*>* entries = [NSMutableArray array];

    for (NSMenuItem* top in NSApp.mainMenu.itemArray)
        [self collectMenuItemsOf:top.submenu into:entries category:top.submenu.title];

    for (const app::CommandRegistry::Entry& action : _registry->actions()) {
        const std::string identifier = action.id;

        __weak INCDAWAppDelegate* weakSelf = self;
        [entries addObject:[INCDAWCommandEntry
            entryWithTitle:@(action.displayName.c_str())
                  category:@(action.category.c_str())
                  shortcut:@(action.defaultShortcut.c_str())
                       run:^{ [weakSelf invokeAction:@(identifier.c_str())]; }]];
    }

    __weak INCDAWAppDelegate* weakUndo = self;

    if (_registry->canUndo()) {
        [entries addObject:[INCDAWCommandEntry
            entryWithTitle:[NSString stringWithFormat:@"Undo %s", _registry->undoName().c_str()]
                  category:@"Edit"
                  shortcut:@"⌘Z"
                       run:^{ [weakUndo undoFromPalette]; }]];
    }

    if (_registry->canRedo()) {
        [entries addObject:[INCDAWCommandEntry
            entryWithTitle:[NSString stringWithFormat:@"Redo %s", _registry->redoName().c_str()]
                  category:@"Edit"
                  shortcut:@"⇧⌘Z"
                       run:^{ [weakUndo redoFromPalette]; }]];
    }

    return entries;
}

- (void)collectMenuItemsOf:(NSMenu*)menu
                      into:(NSMutableArray<INCDAWCommandEntry*>*)entries
                  category:(NSString*)category
{
    for (NSMenuItem* item in menu.itemArray) {
        if (item.isSeparatorItem)
            continue;

        if (item.submenu != nil) {
            [self collectMenuItemsOf:item.submenu into:entries category:item.title];
            continue;
        }

        // An item with no action does nothing when clicked either; listing it
        // would offer the user a command that cannot run.
        if (item.action == nullptr)
            continue;

        [entries addObject:[INCDAWCommandEntry
            entryWithTitle:item.title
                  category:category
                  shortcut:[self shortcutTextFor:item]
                       run:^{ [NSApp sendAction:item.action to:item.target from:item]; }]];
    }
}

- (NSString*)shortcutTextFor:(NSMenuItem*)item
{
    if (item.keyEquivalent.length == 0)
        return @"";

    NSMutableString* text = [NSMutableString string];

    const NSEventModifierFlags flags = item.keyEquivalentModifierMask;
    if ((flags & NSEventModifierFlagControl) != 0) [text appendString:@"⌃"];
    if ((flags & NSEventModifierFlagOption)  != 0) [text appendString:@"⌥"];
    if ((flags & NSEventModifierFlagShift)   != 0) [text appendString:@"⇧"];
    if ((flags & NSEventModifierFlagCommand) != 0) [text appendString:@"⌘"];

    // An uppercase key equivalent already implies Shift, which AppKit does not
    // put in the modifier mask.
    NSString* key = item.keyEquivalent;
    if ([key isEqualToString:key.uppercaseString] && ![key isEqualToString:key.lowercaseString]
        && (flags & NSEventModifierFlagShift) == 0)
        [text appendString:@"⇧"];

    [text appendString:key.uppercaseString];
    return text;
}

- (void)invokeAction:(NSString*)identifier
{
    if (_registry->invoke(identifier.UTF8String))
        [self projectEditedFromBrowser];

    [self.patternList setNeedsDisplay:YES];
    [self.playlist setNeedsDisplay:YES];
    [self refreshStatus];
}

- (void)undoFromPalette
{
    if (_registry->undo())
        [self projectEditedFromBrowser];

    [self.patternList setNeedsDisplay:YES];
    [self.playlist setNeedsDisplay:YES];
    [self.pianoRoll requestRedraw];
    [self refreshStatus];
}

- (void)redoFromPalette
{
    if (_registry->redo())
        [self projectEditedFromBrowser];

    [self.patternList setNeedsDisplay:YES];
    [self.playlist setNeedsDisplay:YES];
    [self.pianoRoll requestRedraw];
    [self refreshStatus];
}

- (void)showCommandPalette:(id)sender
{
    (void)sender;

    if (self.palette == nil)
        self.palette = [[INCDAWCommandPalette alloc] init];

    [self.palette showWithEntries:[self paletteEntries] relativeToWindow:self.window];
}

// ── Workspace and recent projects ────────────────────────────────────────────

- (void)persistSettings
{
    if (!_settingsPath.empty())
        (void)_settings.save(_settingsPath);
}

/// Records where the window is and what it is showing, so the next launch
/// resumes rather than resets.
- (void)captureWorkspace
{
    if (self.window == nil)
        return;

    const NSRect frame = self.window.frame;

    _settings.workspace.windowX      = frame.origin.x;
    _settings.workspace.windowY      = frame.origin.y;
    _settings.workspace.windowWidth  = frame.size.width;
    _settings.workspace.windowHeight = frame.size.height;
    _settings.workspace.activeEditor = static_cast<int>(self.editorSelector.selectedSegment);
    _settings.workspace.songMode     = _songMode == YES;
}

- (void)noteRecentProject:(const std::filesystem::path&)path
{
    _settings.noteRecentProject(path);
    [self persistSettings];
    [self rebuildRecentMenu];
}

/// Rebuilds Open Recent from the settings file.
///
/// Entries whose project is no longer there are skipped rather than deleted:
/// an external drive that is not mounted today is mounted tomorrow, and a
/// menu that forgets a project because a volume was asleep is a menu that
/// loses work the user still has.
- (void)rebuildRecentMenu
{
    if (self.recentMenu == nil)
        return;

    [self.recentMenu removeAllItems];

    NSInteger listed = 0;
    for (const std::string& entry : _settings.recentProjects) {
        const std::filesystem::path path{entry};

        std::error_code failed;
        if (!std::filesystem::exists(path, failed) || failed)
            continue;

        NSMenuItem* item = [self.recentMenu addItemWithTitle:@(path.filename().string().c_str())
                                                       action:@selector(openRecent:)
                                                keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @(entry.c_str());
        item.toolTip           = @(entry.c_str());
        ++listed;
    }

    if (listed == 0) {
        NSMenuItem* empty = [self.recentMenu addItemWithTitle:@"No Recent Projects"
                                                       action:nil
                                                keyEquivalent:@""];
        empty.enabled = NO;
        return;
    }

    [self.recentMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* clear = [self.recentMenu addItemWithTitle:@"Clear Menu"
                                                   action:@selector(clearRecent:)
                                            keyEquivalent:@""];
    clear.target = self;
}

- (void)openRecent:(NSMenuItem*)item
{
    [self openProjectAtPath:std::filesystem::path{[item.representedObject UTF8String]}];
}

- (void)clearRecent:(id)sender
{
    (void)sender;

    _settings.recentProjects.clear();
    [self persistSettings];
    [self rebuildRecentMenu];
}

- (void)toggleBrowser:(id)sender
{
    (void)sender;

    if (self.browser == nil)
        return;

    self.browser.hidden = !self.browser.hidden;
    [self.workspaceSplit adjustSubviews];

    if (!self.browser.hidden)
        [self.workspaceSplit setPosition:210.0 ofDividerAtIndex:0];
}

- (void)showSettings:(id)sender
{
    (void)sender;

    if (self.settingsWindow == nil) {
        self.settingsWindow = [[INCDAWSettingsWindow alloc] initWithSettings:&_settings];

        __weak INCDAWAppDelegate* weakSelf = self;
        self.settingsWindow.onApply        = ^{ [weakSelf applySettings]; };
        self.settingsWindow.statusProvider = ^NSString*(void) { return [weakSelf deviceStatusLine]; };
    }

    [self.settingsWindow show];
}

/// What the device actually granted, which is not always what was asked for.
///
/// A device may refuse a rate, round a block size, or hand the callback larger
/// blocks than the property query reported. Reporting the request back would
/// look tidy and be a lie; every latency figure here comes from the open
/// device (docs/AUDIO_ENGINE.md §2).
- (NSString*)deviceStatusLine
{
    if (_lastSettingsMessage != nil)
        return _lastSettingsMessage;

    if (!_audioReady || _audio == nullptr)
        return @"Audio device unavailable — the settings below are saved and applied at next launch.";

    const double       rate   = _audio->sampleRate();
    const std::int64_t buffer = _audio->bufferSize();
    const double       millis = rate > 0.0 ? (static_cast<double>(buffer) / rate) * 1000.0 : 0.0;

    NSString* midi = _lastMidiError != nil
                         ? [NSString stringWithFormat:@"  ·  ⚠ %@", _lastMidiError]
                         : @"";

    return [NSString stringWithFormat:@"%s  ·  %.0f Hz  ·  %lld frames (%.1f ms)  ·  in %lu / out %lu%@",
                                      _audio->deviceName().c_str(), rate,
                                      static_cast<long long>(buffer), millis,
                                      static_cast<unsigned long>(_audio->inputChannels()),
                                      static_cast<unsigned long>(_audio->outputChannels()),
                                      midi];
}

/// Reopens the device and the MIDI client with the settings just applied.
///
/// The file is written first and independently of whether the device opens:
/// someone who selects an interface that is currently asleep must still find
/// it selected at the next launch.
- (void)applySettings
{
    if (!_settingsPath.empty())
        (void)_settings.save(_settingsPath);

    if (_recording.isRecording()) {
        // Restarting the device under a running take would truncate its file.
        _lastSettingsMessage = @"Stop recording before changing the audio device.";
        return;
    }

    _lastSettingsMessage = nil;

    if (_audio == nullptr) {
        [self startAudio];
        return;
    }

    // Captured in ticks, not frames: a sample-rate change redefines what a
    // frame position means, and the playhead must stay on the same beat.
    const engine::Tick position  = _audio->transport().positionInTicks();
    const BOOL         wasPlaying = _audioReady && _audio->transport().isPlaying();

    if (wasPlaying)
        _audio->transport().stop();

    _audio->stop();

    platform::AudioDeviceConfig config = _settings.audio;

    // Input stays open if it is open now — changing the block size must not
    // silently drop input monitoring — unless the user chose "None".
    if (!_settings.openInputAtLaunch && _audio->inputChannels() == 0)
        config.inputDeviceIdentifier.clear();

    std::string error;
    if (!_audio->start(config, error)) {
        NSLog(@"INCDAW: audio device refused the new settings: %s", error.c_str());
        _lastSettingsMessage = [NSString stringWithFormat:@"⚠ %s", error.c_str()];

        // Fall back to the system default rather than leaving the application
        // silent: a wrong preference must not cost the user their session.
        platform::AudioDeviceConfig fallback = app::defaultAudioConfig();
        std::string                 fallbackError;
        if (!_audio->start(fallback, fallbackError)) {
            NSLog(@"INCDAW: audio restart failed: %s", fallbackError.c_str());
            _audioReady = NO;
            return;
        }
    }

    _audioReady   = YES;
    _activeConfig = config;
    _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());
    _audio->transport().seekToTick(position);

    [self openMidiInputs];
    [self rebuildGraph];

    if (wasPlaying) {
        [self retargetLoop];
        _audio->transport().play();
    }
}

- (void)startAudio
{
    _audio = std::make_unique<engine::AudioEngine>();

    // Whatever the user chose last time (app/AppSettings.h). Defaults are the
    // system default device at 48 kHz and 512 frames — the block size a shared
    // Bluetooth output can sustain, not the lowest the hardware admits to.
    platform::AudioDeviceConfig config = _settings.audio;

    // Recording is opt-in: unless the input was asked for explicitly, the
    // device opens output-only and arming a take opens it on demand.
    if (!_settings.openInputAtLaunch)
        config.inputDeviceIdentifier.clear();

    std::string error;
    if (!_audio->start(config, error)) {
        // Reported, not hidden: an editor that silently does not play is
        // indistinguishable from a broken one.
        NSLog(@"INCDAW: audio unavailable: %s", error.c_str());
        _audioReady = NO;
        return;
    }

    _audioReady   = YES;
    _activeConfig = config;
    _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());

    NSLog(@"INCDAW: audio started — %s, %.0f Hz, %lld frames",
          _audio->deviceName().c_str(), _audio->sampleRate(),
          static_cast<long long>(_audio->bufferSize()));

    [self openMidiInputs];
    [self rebuildGraph];
}

/// Connects the configured MIDI sources to the engine's input.
///
/// Until this existed, `engine::MidiInput` was fed by nothing but the tests:
/// a keyboard plugged into the Mac reached CoreMIDI and stopped there, which
/// made MIDI learn and live playing dead ends in the running application.
///
/// An empty identifier list connects every source — what someone who plugs in
/// a keyboard and presses a key expects, and what the platform layer already
/// means by an empty list.
- (void)openMidiInputs
{
    if (_audio == nullptr)
        return;

    if (_midiDevice != nullptr)
        _midiDevice->close();
    else
        _midiDevice = platform::MidiDevice::create();

    if (_midiDevice == nullptr) {
        _lastMidiError = @"MIDI unavailable";
        return;
    }

    std::string error;
    if (!_midiDevice->open(_settings.midiInputIdentifiers, _audio->midiInput(), error)) {
        // Not fatal, and deliberately so: a DAW that refuses to open because a
        // controller is missing is worse than one that plays without it.
        NSLog(@"INCDAW: MIDI input unavailable: %s", error.c_str());
        _lastMidiError = [NSString stringWithFormat:@"midi: %s", error.c_str()];
        return;
    }

    _lastMidiError = nil;
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
    options.sampleCache  = _sampleCache.get();

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

        auto node = instances->createInsert(slot.id.value(), slot.plugin, sampleRate,
                                            maxFrames, error);

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

    // Instances whose slot left the project are disposed only now, AFTER the
    // swap: the old graph no longer processes, so destroying them cannot race
    // the audio thread (D-031). Bypassed slots keep theirs — and their state.
    if (_pluginInstances != nullptr) {
        std::vector<std::uint64_t> slotKeys;
        for (const project::MixerNode& node : _project->mixerNodes())
            for (const project::PluginSlot& slot : node.inserts)
                slotKeys.push_back(slot.id.value());

        // Editor windows over vanished slots close FIRST, while their
        // instance is still alive to be told (closeEditor inside the close
        // notification). Then the instances go.
        for (NSNumber* key in _editorWindows.allKeys)
            if (std::find(slotKeys.begin(), slotKeys.end(), key.unsignedLongLongValue)
                == slotKeys.end())
                [_editorWindows[key] close];

        _pluginInstances->retainOnlyInstances(slotKeys);
    }
}

// ── Plugins as a catalogue ───────────────────────────────────────────────────

/// Scans a directory of .clap plugins through the out-of-process scanner
/// bundled next to the executable, persists the catalogue, and refreshes the
/// mixer's Add Insert menu. Scanning never runs a plugin in THIS process
/// (docs/PLUGIN_HOST.md §3) — a crashing plugin costs the child, not the DAW.
- (void)scanPlugins:(id)sender
{
    (void)sender;

    NSString* scanner = [NSBundle.mainBundle pathForAuxiliaryExecutable:@"incdaw-pluginscan"];

    if (scanner == nil) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Plugin scanner not found";
        alert.informativeText = @"The incdaw-pluginscan helper is missing from the bundle.";
        [alert runModal];
        return;
    }

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = NO;
    panel.canChooseDirectories    = YES;
    panel.allowsMultipleSelection = NO;
    panel.prompt                  = @"Scan";

    NSString* clapDirectory = [@"~/Library/Audio/Plug-Ins/CLAP" stringByExpandingTildeInPath];
    if ([NSFileManager.defaultManager fileExistsAtPath:clapDirectory])
        panel.directoryURL = [NSURL fileURLWithPath:clapDirectory];

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    const std::size_t scanned = _pluginRegistry.scanDirectory(
        std::filesystem::path{panel.URL.path.UTF8String},
        std::filesystem::path{scanner.UTF8String});

    if (const std::filesystem::path support = incdawSupportDirectory(); !support.empty())
        (void)_pluginRegistry.save(support / "plugins.tsv");

    NSMutableArray<NSDictionary*>* available = [NSMutableArray array];
    for (const plugins::PluginRegistry::Located& located : _pluginRegistry.plugins()) {
        [available addObject:@{
            @"uid":  @(located.plugin->id.c_str()),
            @"name": @(located.plugin->name.empty() ? located.plugin->id.c_str()
                                                    : located.plugin->name.c_str()),
        }];
    }
    _availableInserts           = available;
    self.mixer.availableInserts = available;

    NSLog(@"INCDAW: plugin scan ran %zu child scans; %lu plugins known", scanned,
          static_cast<unsigned long>(available.count));
}

// ── Plugin editor windows ────────────────────────────────────────────────────

/// One window per slot, embedding the plugin's own editor view
/// (docs/PLUGIN_HOST.md §7). The instance belongs to the manager and lives
/// for the slot's lifetime (D-031), so the window survives graph rebuilds;
/// the shell closes it before the instance is ever disposed.
- (void)openEditorForSlotKey:(unsigned long long)slotKey
{
    NSNumber* key = @(slotKey);

    if (NSWindow* existing = _editorWindows[key]) {
        [existing makeKeyAndOrderFront:nil];
        return;
    }

    plugins::ClapInstance* instance =
        _pluginInstances != nullptr ? _pluginInstances->instanceFor(slotKey) : nullptr;

    if (instance == nullptr || !instance->hasEditor()) {
        // Bypassed or unbuilt slots have no live instance; some plugins have
        // no editor at all. Either way there is nothing to open.
        NSBeep();
        return;
    }

    NSView* container = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, 400, 300)];

    std::uint32_t width  = 0;
    std::uint32_t height = 0;

    if (!instance->openEditor((__bridge void*)container, width, height)) {
        NSBeep();
        return;
    }

    const NSRect frame = NSMakeRect(0, 0, width, height);
    container.frame    = frame;

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    window.releasedWhenClosed = NO;
    window.contentView        = container;

    // Titled by what the user loaded, not by an internal key.
    NSString* title = @"Plugin";
    for (const project::MixerNode& node : _project->mixerNodes())
        for (const project::PluginSlot& slot : node.inserts)
            if (slot.id.value() == slotKey)
                title = @(slot.plugin.uid.c_str());
    for (NSDictionary* plugin in _availableInserts)
        if ([plugin[@"uid"] isEqualToString:title])
            title = plugin[@"name"];
    window.title = title;

    [window center];
    [window makeKeyAndOrderFront:nil];

    _editorWindows[key] = window;

    __weak INCDAWAppDelegate* weakSelf = self;
    _editorObservers[key] = [NSNotificationCenter.defaultCenter
        addObserverForName:NSWindowWillCloseNotification
                    object:window
                     queue:nil
                usingBlock:^(NSNotification*) {
                    [weakSelf editorWindowClosedForSlotKey:slotKey];
                }];
}

- (void)editorWindowClosedForSlotKey:(unsigned long long)slotKey
{
    NSNumber* key = @(slotKey);

    if (_editorWindows[key] == nil)
        return;

    if (id observer = _editorObservers[key])
        [NSNotificationCenter.defaultCenter removeObserver:observer];

    [_editorObservers removeObjectForKey:key];
    [_editorWindows removeObjectForKey:key];

    // The instance may already be gone when the slot was removed first; the
    // manager answers nullptr then, and there is nothing left to close.
    if (_pluginInstances != nullptr)
        if (plugins::ClapInstance* instance = _pluginInstances->instanceFor(slotKey))
            instance->closeEditor();
}

// ── The project as a document ────────────────────────────────────────────────

- (void)saveProject:(id)sender
{
    if (_projectPath.empty()) {
        [self saveProjectAs:sender];
        return;
    }

    [self writeProjectToPath];
}

- (void)saveProjectAs:(id)sender
{
    (void)sender;

    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.nameFieldStringValue = @"Untitled.incdaw";
    panel.canCreateDirectories = YES;

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    std::filesystem::path chosen{panel.URL.path.UTF8String};
    if (chosen.extension() != ".incdaw")
        chosen += ".incdaw";

    _projectPath = chosen;
    [self writeProjectToPath];
}

- (void)exportAudio:(id)sender
{
    (void)sender;

    if (!_audioReady)
        return;

    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.nameFieldStringValue = @"Master.wav";
    panel.canCreateDirectories = YES;
    panel.allowedContentTypes  = @[
        [UTType typeWithFilenameExtension:@"wav"], [UTType typeWithFilenameExtension:@"aiff"]
    ];

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    project::RenderOptions options;
    options.sampleRate  = _audio->sampleRate();
    options.sampleCache = _sampleCache.get();
    options.parameters  = &_parameters;

    const auto rendered = project::renderProjectToFile(
        *_project, _audio->transport().tempoMap(), options,
        std::filesystem::path{panel.URL.path.UTF8String});

    if (!rendered) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Could not export audio";
        alert.informativeText = @(rendered.error.c_str());
        [alert runModal];
        return;
    }

    for (const std::string& warning : rendered.warnings)
        NSLog(@"INCDAW: export: %s", warning.c_str());
}

- (void)exportMidi:(id)sender
{
    (void)sender;

    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.nameFieldStringValue = @"Arrangement.mid";
    panel.canCreateDirectories = YES;
    panel.allowedContentTypes  = @[ [UTType typeWithFilenameExtension:@"mid"] ];

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    const auto written = project::exportArrangement(
        *_project, std::filesystem::path{panel.URL.path.UTF8String});

    if (!written) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Could not export MIDI";
        alert.informativeText = @(written.error.c_str());
        [alert runModal];
    }
}

- (void)importMidi:(id)sender
{
    (void)sender;

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = YES;
    panel.canChooseDirectories    = NO;
    panel.allowsMultipleSelection = NO;
    panel.allowedContentTypes     = @[
        [UTType typeWithFilenameExtension:@"mid"], [UTType typeWithFilenameExtension:@"midi"]
    ];

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    [self importMidiFromPath:std::filesystem::path{panel.URL.path.UTF8String}];
}

/// The import itself, reachable from the menu and from the browser.
- (void)importMidiFromPath:(const std::filesystem::path&)file
{
    const auto imported = project::importAsPattern(*_project, file);

    if (!imported) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Could not import MIDI";
        alert.informativeText = @(imported.error.c_str());
        [alert runModal];
        return;
    }

    // Not a command, deliberately: the import creates fresh entities and the
    // undo story for "un-importing" channels a later edit may reference is
    // its own project. Recorded in HANDOFF; the redraws below make the
    // result visible immediately.
    [self.patternList setNeedsDisplay:YES];
    [self.channelRack setNeedsDisplay:YES];
    [self rebuildGraph];
}

/// The save itself. Live plugin state is captured FIRST, so the stateFile
/// paths land in the project.json this save writes (docs/PLUGIN_HOST.md §6).
- (void)writeProjectToPath
{
    for (const std::string& warning :
         project::capturePluginState(*_project, _live, _projectPath))
        NSLog(@"INCDAW: save: %s", warning.c_str());

    const auto result = project::ProjectFile::save(*_project, _projectPath);

    if (!result) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Could not save the project";
        alert.informativeText = @(result.error.c_str());
        [alert runModal];
        return;
    }

    self.window.title =
        [NSString stringWithFormat:@"INCDAW — %s", _projectPath.stem().string().c_str()];

    [self noteRecentProject:_projectPath];
}

- (void)openProject:(id)sender
{
    (void)sender;

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = YES;
    panel.canChooseDirectories    = YES;   // a package is a directory
    panel.allowsMultipleSelection = NO;

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    [self openProjectAtPath:std::filesystem::path{panel.URL.path.UTF8String}];
}

/// Opening itself, reachable from the menu, the browser and later the recent
/// list. The panel is one way in, not the only one.
- (void)openProjectAtPath:(const std::filesystem::path&)chosen
{
    if (!project::ProjectFile::isProjectPackage(chosen)) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Not an INCDAW project";
        alert.informativeText = @(chosen.string().c_str());
        [alert runModal];
        return;
    }

    // Loaded IN PLACE: every view holds a pointer to this project object, and
    // ProjectFile::load replaces its contents wholesale. Undo history against
    // the previous contents no longer applies to what is now here.
    const auto result = project::ProjectFile::load(*_project, chosen);

    if (!result) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Could not open the project";
        alert.informativeText = @(result.error.c_str());
        [alert runModal];
        return;
    }

    if (result.migrated)
        NSLog(@"INCDAW: project migrated from format %s", result.migratedFrom.c_str());

    _registry->clearHistory();
    _projectPath = chosen;

    [self noteRecentProject:chosen];
    [self adoptLoadedProject];
}

/// Points everything at what was just loaded: tempo, views, the graph — and
/// hands hosted plugins their state back once the graph that owns them exists.
- (void)adoptLoadedProject
{
    const project::EntityId firstPattern =
        _project->patterns().empty() ? project::EntityId{} : _project->patterns().front().id;
    const project::EntityId firstChannel =
        _project->channels().empty() ? project::EntityId{} : _project->channels().front().id;

    self.pianoRoll.patternIdValue           = firstPattern.value();
    self.pianoRoll.channelIdValue           = firstChannel.value();
    self.channelRack.patternIdValue         = firstPattern.value();
    self.channelRack.selectedChannelIdValue = firstChannel.value();
    self.patternList.selectedPatternIdValue = firstPattern.value();
    self.playlist.patternIdValue            = firstPattern.value();
    self.mixer.selectedChannelIdValue       = firstChannel.value();

    if (_audioReady) {
        // The loaded tempo replaces the transport's, at the device's rate.
        _audio->transport().tempoMapForEdit() = _project->tempoMap();
        _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());
    }

    [self refreshTempoFields];

    [self rebuildGraph];

    if (_audioReady && !_projectPath.empty())
        for (const std::string& warning :
             project::restorePluginState(*_project, _live, _projectPath))
            NSLog(@"INCDAW: open: %s", warning.c_str());

    [self retargetLoop];

    [self.pianoRoll setNeedsDisplay:YES];
    [self.channelRack setNeedsDisplay:YES];
    [self.patternList setNeedsDisplay:YES];
    [self.playlist setNeedsDisplay:YES];
    [self.mixer setNeedsDisplay:YES];
    [self.audioEditor setNeedsDisplay:YES];

    self.window.title =
        [NSString stringWithFormat:@"INCDAW — %s", _projectPath.stem().string().c_str()];
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

    // The configured device, with the input turned on: arming a take is what
    // opens the microphone when the settings did not open it at launch.
    platform::AudioDeviceConfig config = _settings.audio;
    if (config.inputDeviceIdentifier.empty())
        config.inputDeviceIdentifier = platform::AudioDeviceConfig::defaultInput;

    std::string error;
    if (_audio->start(config, error)) {
        _activeConfig = config;
    } else {
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

        _activeConfig = config;
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

- (void)armMidiLearnForKey:(NSString*)parameterKey target:(unsigned long long)targetId
{
    _learnArmed        = true;
    _learnParameterKey = parameterKey;
    _learnTarget       = targetId;
    _learnControlSeen  = _audioReady ? _audio->midiInput().lastControlChange() : 0;
}

- (void)forgetMidiMappingsForTarget:(unsigned long long)targetId
{
    const project::EntityId target{targetId};

    // One command per mapping keeps each undoable; targeting a node rarely
    // holds more than a knob or two.
    bool removed = false;
    for (bool again = true; again;) {
        again = false;
        for (const project::MidiMapping& mapping : _project->midiMappings()) {
            if (mapping.targetEntity != target)
                continue;

            removed = _registry->execute(
                          std::make_unique<app::RemoveMidiMappingCommand>(mapping.id))
                   || removed;
            again = true;
            break;   // the vector changed under the loop; restart
        }
    }

    if (removed)
        [self rebuildGraph];
}

/// The tail end of MIDI learn: the knob has been turned, bind it.
- (void)completeMidiLearnWithPacked:(std::uint64_t)packed
{
    _learnArmed = false;

    const int controller = engine::MidiInput::controllerOf(packed);

    // Re-learning a control replaces its old binding, undoably: remove any
    // mapping on the same CC first, then add the new one.
    for (bool again = true; again;) {
        again = false;
        for (const project::MidiMapping& mapping : _project->midiMappings()) {
            if (mapping.controller != controller || mapping.midiChannel != -1)
                continue;

            (void)_registry->execute(
                std::make_unique<app::RemoveMidiMappingCommand>(mapping.id));
            again = true;
            break;
        }
    }

    if (_registry->execute(std::make_unique<app::AddMidiMappingCommand>(
            -1, controller, std::string{_learnParameterKey.UTF8String},
            project::EntityId{_learnTarget})))
        [self rebuildGraph];
}

- (void)housekeeping
{
    if (!_audioReady)
        return;

    _audio->collectRetiredGraphs();

    if (_learnArmed) {
        const std::uint64_t packed = _audio->midiInput().lastControlChange();
        if (packed != _learnControlSeen && packed != 0)
            [self completeMidiLearnWithPacked:packed];
    }

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

    // Cmd+, — where every macOS application keeps this, and the first place
    // anyone looks when the audio comes out of the wrong interface.
    NSMenuItem* settingsItem = [appMenu addItemWithTitle:@"Settings…"
                                                  action:@selector(showSettings:)
                                           keyEquivalent:@","];
    settingsItem.target = self;

    NSMenuItem* paletteItem = [appMenu addItemWithTitle:@"Command Search…"
                                                 action:@selector(showCommandPalette:)
                                          keyEquivalent:@"k"];
    paletteItem.target = self;

    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit INCDAW" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;

    // File: the project as a document. Until this menu existed, everything a
    // session produced was lost on quit — ProjectFile worked and was tested,
    // but nothing called it.
    NSMenuItem* fileItem = [[NSMenuItem alloc] init];
    [menuBar addItem:fileItem];

    NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    [fileMenu addItemWithTitle:@"Open…" action:@selector(openProject:) keyEquivalent:@"o"];

    NSMenuItem* recentItem = [fileMenu addItemWithTitle:@"Open Recent" action:nil keyEquivalent:@""];
    self.recentMenu   = [[NSMenu alloc] initWithTitle:@"Open Recent"];
    recentItem.submenu = self.recentMenu;
    [self rebuildRecentMenu];

    [fileMenu addItem:[NSMenuItem separatorItem]];
    [fileMenu addItemWithTitle:@"Save" action:@selector(saveProject:) keyEquivalent:@"s"];
    [fileMenu addItemWithTitle:@"Save As…" action:@selector(saveProjectAs:) keyEquivalent:@"S"];
    [fileMenu addItem:[NSMenuItem separatorItem]];
    [fileMenu addItemWithTitle:@"Export Audio…"
                        action:@selector(exportAudio:)
                 keyEquivalent:@"e"];
    [fileMenu addItemWithTitle:@"Export MIDI…" action:@selector(exportMidi:) keyEquivalent:@""];
    [fileMenu addItemWithTitle:@"Import MIDI…" action:@selector(importMidi:) keyEquivalent:@""];
    [fileMenu addItem:[NSMenuItem separatorItem]];
    [fileMenu addItemWithTitle:@"Scan Plugins…" action:@selector(scanPlugins:) keyEquivalent:@""];
    fileItem.submenu = fileMenu;

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

    // The browser is a pane rather than an editor: it does not replace what is
    // on screen, it appears beside it, so it toggles instead of switching.
    NSMenuItem* browserItem = [viewMenu addItemWithTitle:@"Browser"
                                                  action:@selector(toggleBrowser:)
                                           keyEquivalent:@"b"];
    browserItem.target = self;

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

    // The MIDI client delivers on its own thread and holds a pointer to the
    // engine's input: it must be closed before the engine goes away.
    if (_midiDevice != nullptr) {
        _midiDevice->close();
        _midiDevice.reset();
    }

    [self captureWorkspace];
    [self persistSettings];
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
