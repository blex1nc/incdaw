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
#include "app/Browser.h"
#include "app/CommandRegistry.h"
#include "app/ProjectSession.h"
#include "app/StandardActions.h"
#include "app/UpdateCheck.h"
#include "app/Version.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/MidiMappingCommands.h"
#include "app/commands/RecordingCommands.h"
#include "app/commands/SamplerCommands.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "engine/AudioEngine.h"
#include "platform/AudioUnitHost.h"
#include "platform/Http.h"
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
#include "app/commands/ImportCommands.h"
#include "app/commands/SlicerCommands.h"
#include "app/commands/TempoCommands.h"
#include "engine/audio/OnsetDetection.h"
#include "engine/dsp/effects/BuiltinEffects.h"
#include "ui/macos/AudioEditorView.h"
#include "ui/macos/BrowserView.h"
#include "ui/macos/ChannelRackView.h"
#include "engine/dsp/effects/UtilityEffects.h"
#include "ui/macos/ControlBarView.h"
#include "ui/macos/InsertParameterPanel.h"
#include "ui/macos/SpectrumView.h"
#include "ui/macos/TonePanel.h"
#include "ui/macos/Theme.h"
#include "ui/ThemeLibrary.h"
#include "ui/macos/PatternListView.h"
#include "ui/macos/PianoRollView.h"
#include "ui/macos/PianoRollHeaderView.h"
#include "ui/macos/MixerView.h"
#include "ui/macos/PlaylistView.h"
#include "ui/macos/CommandPalette.h"
#include "ui/macos/SettingsWindow.h"

#include <atomic>
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

/// Where the user's own colour schemes live. A folder rather than a key inside
/// settings.json, because a theme somebody spends an evening on should be a
/// file they can copy to another machine or send to somebody else
/// (ui/ThemeLibrary.h, docs/DECISIONS.md D-039).
std::filesystem::path incdawThemesDirectory()
{
    const std::filesystem::path support = incdawSupportDirectory();
    if (support.empty())
        return {};

    return support / "Themes";
}

} // namespace

/// One export destination and the options that render it.
struct ExportJob {
    std::filesystem::path          destination;
    incdaw::project::RenderOptions options;
    std::string                    displayName;
};

/// Recent projects live in user defaults as an array of absolute paths,
/// newest first.
static NSString* const      kRecentProjectsKey = @"INCDAWRecentProjects";
static const NSUInteger     kRecentProjectsCap = 10;
static const NSTimeInterval kAutosaveInterval  = 120.0;

@interface INCDAWAppDelegate : NSObject <NSApplicationDelegate>
@property (strong) NSWindow*                window;
@property (strong) INCDAWPianoRollView*     pianoRoll;
@property (strong) INCDAWPianoRollHeaderView* pianoRollHeader;
@property (strong) INCDAWPlaylistView*      playlist;
@property (strong) INCDAWMixerView*         mixer;
@property (strong) INCDAWAudioEditorView*   audioEditor;
@property (strong) INCDAWControlBarView*    controlBar;
@property (strong) INCDAWChannelRackView*   channelRack;
@property (strong) INCDAWPatternListView*   patternList;
@property (strong) INCDAWStatusBarView*     statusBar;
@property (strong) INCDAWBrowserView*       browserPane;
@property (strong) INCDAWSettingsWindow*    settingsWindow;
@property (strong) INCDAWCommandPalette*    palette;
@end

@implementation INCDAWAppDelegate {
    std::unique_ptr<project::Project>     _project;
    std::unique_ptr<app::CommandRegistry> _registry;
    std::unique_ptr<engine::AudioEngine>  _audio;

    NSTimer* _housekeeping;
    BOOL     _audioReady;

    /// The metronome. Session state, like input monitoring: it is a monitoring
    /// aid, not part of the project, and must never reach a render.
    BOOL     _metronomeEnabled;

    /// Tap-tempo history, newest last. Cleared by a long gap.
    std::vector<double> _tapTimes;

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

    /// Set by any project mutation since the last save, open, or new — what
    /// the quit and open guards ask, and what the close button's dot shows.
    /// Sourced from the undo stack's depth in housekeeping, plus the paths
    /// that mutate without a command (MIDI import). A knob turned inside a
    /// plugin's own editor window is NOT seen — recorded limitation.
    BOOL _dirty;

    /// The safety net (app/ProjectSession.h decides where it lands). Fires
    /// every two minutes; a clean project is skipped.
    NSTimer* _autosave;

    /// File > Open Recent, rebuilt from user defaults whenever they change.
    NSMenu* _recentMenu;

    /// The scanned plugin catalogue as menu fodder, built once at launch.
    NSArray<NSDictionary*>* _availableInserts;

    /// Open plugin editor windows and their close observers, by slot key.
    /// Closed by the shell BEFORE an instance is disposed (D-031): the
    /// window's death must reach the plugin while the plugin is still alive.
    NSMutableDictionary<NSNumber*, NSWindow*>* _editorWindows;
    NSMutableDictionary<NSNumber*, id>*        _editorObservers;

    /// The Browser pane's model: roots, favourites and recents. Application
    /// state rather than project state — a sample library belongs to the
    /// installation, and follows the user from project to project.
    app::Browser          _browser;
    std::filesystem::path _browserSettings;

    /// Generic parameter panels, by slot key — what "Open Editor" opens for a
    /// builtin effect, or a hosted plugin without an editor of its own. The
    /// panels hold no engine pointers (sinks die with their graph); writes
    /// re-resolve through _live on every move.
    NSMutableDictionary<NSNumber*, NSWindow*>* _panelWindows;
    NSMutableDictionary<NSNumber*, id>*        _panelObservers;
    int                                        _panelRefreshTick;

    /// The sampler zone editor: one window, over one channel at a time. The
    /// per-row control references live in _zoneRows so a field's action can
    /// read its whole row; content is rebuilt from the model on every change.
    NSWindow*                     _zoneWindow;
    unsigned long long            _zoneChannelKey;
    NSMutableArray<NSDictionary*>* _zoneRows;

    /// The MIDI mapping list: one window, refreshed on every rebuild while
    /// open (learn and forget change mappings from elsewhere).
    NSWindow*                  _mappingWindow;
    NSMutableArray<NSNumber*>* _mappingRowIds;

    /// Export cancel flag, shared with the rendering thread. Reset per run.
    std::shared_ptr<std::atomic<bool>> _exportCancel;

    /// The audio editor's clipboard: whatever Copy or Cut last extracted.
    /// App-local by design — a WAV region is not pasteboard text.
    engine::AudioFileData _audioClipboard;

    /// Which interface plays, at which rate and block size, which MIDI sources
    /// are connected, and where the window was left (app/AppSettings.h). Read
    /// before the device opens, written on every apply and at quit. Separate
    /// from the project file on purpose: a project that carried its author's
    /// interface would be unopenable on a second Mac.
    app::AppSettings      _settings;
    std::filesystem::path _settingsPath;

    /// One update request at a time. Pressing the menu item twice must not
    /// stack two alerts on top of each other.
    BOOL _updateCheckInFlight;

    /// The MIDI client. Owned by the shell because the settings window decides
    /// which sources are connected, and the engine's input is what they feed.
    /// Closed before the engine goes away: it delivers on its own thread and
    /// holds a reference to engine::MidiInput.
    std::unique_ptr<platform::MidiDevice> _midiDevice;
    NSString*                             _lastMidiError;

    /// What the settings window reports instead of the device line: a refused
    /// rate, a device that would not open, an apply a running take blocked.
    NSString* _lastSettingsMessage;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;

    _project      = std::make_unique<project::Project>();
    _registry     = std::make_unique<app::CommandRegistry>(*_project);
    _diskStreamer = std::make_unique<engine::DiskStreamer>();
    _sampleCache  = std::make_unique<engine::SampleCache>();

    // Read before anything opens a device: the settings file is what decides
    // which interface, which rate and which block size the engine asks for
    // (app/AppSettings.h). A missing file is the normal first run, and a
    // corrupt one degrades to defaults rather than to a failure to launch.
    if (const std::filesystem::path support = incdawSupportDirectory(); !support.empty()) {
        _settingsPath = support / "settings.json";
        _settings     = app::AppSettings::load(_settingsPath);
    }

    // Before any view exists. A palette applied after the window is built is a
    // window that flashes the default scheme on every launch, and the settings
    // file has already said which scheme the user meant (ui/ThemePalette.h).
    ui::theme::setPalette(
        ui::theme::ThemeLibrary(incdawThemesDirectory()).resolve(_settings.appearance.themeName));

    // AppKit's own controls follow the palette rather than the other way round:
    // a light theme with dark scrollers over it is a theme that only half took.
    [self observePaletteChanges];

    // The registry's action table was empty in the running application until
    // this existed: every edit arrived as a command, but none of them had a
    // name the palette, a shortcut or a script could look up (CLAUDE.md §26).
    app::registerStandardActions(*_registry);

    // The plugin catalogue is read from a file; launching touches no plugin
    // binary at all, because startup time must not scale with the size of a
    // plugin collection (docs/PLUGIN_HOST.md §3). A missing file is the normal
    // first-run state, not an error.
    if (const std::filesystem::path support = incdawSupportDirectory(); !support.empty())
        (void)_pluginRegistry.load(support / "plugins.tsv");

    // The browser keeps its roots and favourites next to that catalogue. A
    // first launch has no file and is given the user's own folders, because a
    // browser that opens empty teaches nothing about what it is for.
    if (const std::filesystem::path support = incdawSupportDirectory(); !support.empty()) {
        _browserSettings = support / "browser.json";

        std::string browserError;
        if (!_browser.load(_browserSettings, browserError))
            NSLog(@"INCDAW: browser settings ignored: %s", browserError.c_str());
    }

    if (_browser.roots().empty()) {
        _browser.addDefaultRoots(std::filesystem::path{NSHomeDirectory().UTF8String});
        [self saveBrowserSettings];
    }

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
        const plugins::PluginIdentifier identifier{plugins::Format::clap, located.plugin->id};

        [available addObject:@{
            @"id":   @(identifier.toString().c_str()),
            @"name": @(located.plugin->name.empty() ? located.plugin->id.c_str()
                                                    : located.plugin->name.c_str()),
        }];
    }

    // Audio Units need no scan: the system's component registry IS their
    // catalogue, and enumerating it runs no plugin code (docs/PLUGIN_HOST.md
    // §3). Instruments are left out — an insert slot is an effect.
    for (const platform::AudioUnitDescription& unit : platform::scanAudioUnits()) {
        if (unit.isInstrument)
            continue;

        const plugins::PluginIdentifier identifier{plugins::Format::audioUnit, unit.uid};

        NSString* label = unit.manufacturer.empty()
                              ? @(unit.name.c_str())
                              : [NSString stringWithFormat:@"%s — %s", unit.name.c_str(),
                                                           unit.manufacturer.c_str()];

        [available addObject:@{@"id": @(identifier.toString().c_str()), @"name": label}];
    }
    _availableInserts = available;
    _editorWindows    = [NSMutableDictionary dictionary];
    _editorObservers  = [NSMutableDictionary dictionary];
    _panelWindows     = [NSMutableDictionary dictionary];
    _panelObservers   = [NSMutableDictionary dictionary];

    [self seedStarterProject];

    const project::EntityId channelId = _project->channels().front().id;
    const project::EntityId patternId = _project->patterns().front().id;

    // The size a first launch gets. A returning one is restored below, and
    // everything after that measures the window rather than this constant.
    const NSRect defaultFrame = NSMakeRect(0, 0, 1280, 800);

    self.window = [[NSWindow alloc]
        initWithContentRect:defaultFrame
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                          | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    [self refreshWindowTitle];
    self.window.backgroundColor = ui::theme::ink(ui::theme::Ink::windowBackground);

    // The shell paints its own dark surfaces; telling AppKit the same thing is
    // what keeps scrollers, menus, sheets and text fields from arriving in the
    // light scheme on top of them.
    self.window.appearance = [NSAppearance appearanceNamed:ui::theme::paletteIsLight()
                                                              ? NSAppearanceNameAqua
                                                              : NSAppearanceNameDarkAqua];
    self.window.titlebarAppearsTransparent = YES;

    [self restoreWindowFrame];

    NSView* content = self.window.contentView;

    // Measured, not assumed: the window may have just been restored to the
    // size it was left at, and a layout built from the default constant would
    // overhang it by exactly the difference until the first manual resize.
    const NSRect frame = content.bounds;

    constexpr CGFloat statusHeight  = ui::theme::metrics::statusBarHeight;
    constexpr CGFloat toolbarHeight = ui::theme::metrics::controlBarHeight;
    constexpr CGFloat listWidth     = 172.0;
    constexpr CGFloat browserWidth  = 210.0;
    constexpr CGFloat rackHeight    = 244.0;

    const NSRect body = NSMakeRect(0, statusHeight, frame.size.width,
                                   frame.size.height - statusHeight - toolbarHeight);

    const NSRect editorFrame = NSMakeRect(0, 0, body.size.width - listWidth - browserWidth,
                                          body.size.height - rackHeight);

    self.pianoRoll = [[INCDAWPianoRollView alloc]
        initWithFrame:editorFrame
              project:_project.get()
             registry:_registry.get()];

    self.pianoRoll.patternIdValue = patternId.value();
    self.pianoRoll.channelIdValue = channelId.value();

    // The Piano Roll's control strip is a sibling, not a band inside the
    // editor: the editor draws through Metal, where text costs a layer each and
    // the budget is spent on ten thousand notes (D-006). Chrome is drawn with
    // CoreGraphics through the theme, like every other control surface here.
    const CGFloat pianoRollHeaderHeight = [INCDAWPianoRollHeaderView preferredHeight];

    self.pianoRollHeader = [[INCDAWPianoRollHeaderView alloc]
        initWithFrame:NSMakeRect(0, editorFrame.size.height - pianoRollHeaderHeight,
                                 editorFrame.size.width, pianoRollHeaderHeight)];

    self.pianoRoll.frame = NSMakeRect(0, 0, editorFrame.size.width,
                                      editorFrame.size.height - pianoRollHeaderHeight);

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

    self.mixer.onParameterGestureEnded = ^(unsigned long long nodeId, const char* key) {
        [weakSelfForEditor automationGestureEnded:nodeId key:key];
    };

    // The editors share one region and are swapped rather than tiled: each
    // wants the whole window, and a DAW that shows half a Piano Roll above half
    // a playlist shows neither.
    NSView* editorContainer = [[NSView alloc] initWithFrame:editorFrame];
    self.pianoRoll.autoresizingMask   = NSViewWidthSizable | NSViewHeightSizable;

    // Width with the pane, but pinned to its top edge: a strip that grew with
    // the window would eat the grid.
    self.pianoRollHeader.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    self.playlist.autoresizingMask    = NSViewWidthSizable | NSViewHeightSizable;
    self.mixer.autoresizingMask       = NSViewWidthSizable | NSViewHeightSizable;
    self.audioEditor.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [editorContainer addSubview:self.pianoRoll];
    [editorContainer addSubview:self.pianoRollHeader];
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

    // The Browser: leftmost, as in every DAW that has one. It opens nothing
    // itself — a double-click hands the path back here, and this decides what
    // opening it means.
    self.browserPane = [[INCDAWBrowserView alloc]
        initWithFrame:NSMakeRect(0, 0, browserWidth, body.size.height)
              browser:&_browser];

    self.browserPane.autoresizingMask = NSViewHeightSizable;

    __weak INCDAWAppDelegate* weakSelfForBrowser = self;

    self.browserPane.onActivate = ^(NSString* path) {
        [weakSelfForBrowser browserActivated:path];
    };

    self.browserPane.onSettingsChanged = ^{
        [weakSelfForBrowser saveBrowserSettings];
    };

    self.browserPane.onPreview = ^(NSString* path) {
        [weakSelfForBrowser browserPreview:path];
    };

    self.browserPane.onStopPreview = ^{
        [weakSelfForBrowser stopBrowserPreview];
    };

    NSSplitView* workspace = [[NSSplitView alloc] initWithFrame:body];
    workspace.vertical         = YES;
    workspace.dividerStyle     = NSSplitViewDividerStyleThin;
    workspace.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [workspace addSubview:self.browserPane];
    [workspace addSubview:listScroll];
    [workspace addSubview:editors];

    [content addSubview:workspace];

    [workspace setPosition:browserWidth ofDividerAtIndex:0];
    [workspace setPosition:browserWidth + listWidth ofDividerAtIndex:1];
    [editors setPosition:body.size.height - rackHeight ofDividerAtIndex:0];

    [workspace adjustSubviews];
    [editors adjustSubviews];

    // The control bar: transport, mode, centre display and the editor tabs.
    // It reports intent through blocks and holds no engine state of its own.
    self.controlBar = [[INCDAWControlBarView alloc]
        initWithFrame:NSMakeRect(0, frame.size.height - toolbarHeight,
                                 frame.size.width, toolbarHeight)];

    self.controlBar.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    [content addSubview:self.controlBar];

    self.statusBar = [[INCDAWStatusBarView alloc]
        initWithFrame:NSMakeRect(0, 0, frame.size.width, statusHeight)];

    self.statusBar.autoresizingMask = NSViewWidthSizable | NSViewMaxYMargin;
    [content addSubview:self.statusBar];

    __weak INCDAWAppDelegate* weakBarSelf = self;

    self.controlBar.onTransport = ^(INCDAWTransportAction action) {
        [weakBarSelf handleTransportAction:action];
    };

    self.controlBar.onSelectEditor = ^(NSInteger index) {
        [weakBarSelf showEditorAtSegment:index];
    };

    self.controlBar.onSelectMode = ^(BOOL songMode) {
        [weakBarSelf setSongMode:songMode];
    };

    self.controlBar.onTempoChange = ^(double beatsPerMinute, BOOL finished) {
        [weakBarSelf applyTempo:beatsPerMinute finished:finished];
    };

    self.controlBar.onTimeSignature = ^(NSInteger numerator, NSInteger denominator) {
        [weakBarSelf applyTimeSignature:numerator over:denominator];
    };

    __weak INCDAWAppDelegate* weakSelf = self;

    void (^changed)(void) = ^{
        [weakSelf rebuildGraph];
        [weakSelf refreshStatus];
        [weakSelf.channelRack setNeedsDisplay:YES];
        [weakSelf.pianoRoll requestRedraw];
    };

    // The control strip: it owns none of these settings, so every pick is
    // handed to the editor and the strip is then told what the editor says.
    // One direction of truth, or the two drift the first time a keystroke
    // changes one of them.
    self.pianoRollHeader.onSnapPicked = ^(long long ticks) {
        weakSelf.pianoRoll.snapTicks = ticks;
        [weakSelf syncPianoRollHeader];
    };

    self.pianoRollHeader.onKeyPicked = ^(int rootPitchClass) {
        weakSelf.pianoRoll.keyRootPitchClass = rootPitchClass;
        [weakSelf syncPianoRollHeader];
    };

    self.pianoRollHeader.onScalePicked = ^(int scaleIndex) {
        weakSelf.pianoRoll.scaleIndex = scaleIndex;
        [weakSelf syncPianoRollHeader];
    };

    self.pianoRollHeader.onToggleGhosts = ^{
        weakSelf.pianoRoll.ghostNotesVisible = !weakSelf.pianoRoll.ghostNotesVisible;
        [weakSelf syncPianoRollHeader];
    };

    self.pianoRollHeader.onToggleVelocityLane = ^{
        weakSelf.pianoRoll.velocityLaneVisible = !weakSelf.pianoRoll.velocityLaneVisible;
        [weakSelf syncPianoRollHeader];
    };

    // The same settings have keystrokes (E, G). When one of those moves, the
    // strip is told rather than left showing what used to be true.
    self.pianoRoll.onEditorStateChanged = ^{
        [weakSelf syncPianoRollHeader];
        [weakSelf refreshStatus];
    };

    [self syncPianoRollHeader];

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

    self.channelRack.onEditInstrument = ^(unsigned long long channelKey) {
        [weakAudioSelf openInstrumentPanelForChannel:channelKey];
    };
    self.channelRack.onEditSamplerZones = ^(unsigned long long channelKey) {
        [weakAudioSelf openZoneEditorForChannel:channelKey];
    };

    // Drives the playhead and reclaims retired graphs. Both are non-realtime
    // work that must happen off the audio thread; 30 Hz is smooth enough for a
    // playhead and cheap enough to ignore.
    _housekeeping = [NSTimer scheduledTimerWithTimeInterval:1.0 / 30.0
                                                    repeats:YES
                                                      block:^(NSTimer* timer) {
        (void)timer;
        [weakAudioSelf housekeeping];
    }];

    // The safety net. Two minutes: long enough to stay invisible, short
    // enough that a crash costs minutes of work, not a session.
    _autosave = [NSTimer scheduledTimerWithTimeInterval:kAutosaveInterval
                                                repeats:YES
                                                  block:^(NSTimer* timer) {
        (void)timer;
        [weakAudioSelf autosaveTick];
    }];

    [self buildMenu];
    [self refreshStatus];

    [self.window makeKeyAndOrderFront:nil];

    // The pane and the mode the session was left in, applied through the same
    // entry points the user's own click would take — nothing can restore into a
    // state the UI has no way to reach. Selecting a pane is also what gives it
    // first responder, so this runs after the window is on screen rather than
    // before: focus must end up on the pane that is actually showing.
    const NSInteger restoredEditor = _settings.workspace.activeEditor;
    [self showEditorAtSegment:(restoredEditor > 0 && restoredEditor < 4) ? restoredEditor : 0];

    if (_settings.workspace.songMode)
        [self setSongMode:YES];

    [NSApp activateIgnoringOtherApps:YES];

    [self offerAutosaveRecovery];

    // Last, and deliberately: the answer arrives from the network some time
    // after this returns, so it can never delay a launch, and it queues behind
    // the recovery prompt rather than over it.
    [self checkForUpdatesInBackground];
}

/// A leftover unsaved-project autosave means the last session ended without a
/// normal quit — a normal quit deletes it. Offer it before the user starts
/// working over it. Saved projects recover through Open instead: their
/// autosave lives beside them and is offered there when it is newer.
- (void)offerAutosaveRecovery
{
    const std::filesystem::path recovery = [self untitledAutosavePath];
    if (recovery.empty() || !project::ProjectFile::isProjectPackage(recovery))
        return;

    NSAlert* alert        = [[NSAlert alloc] init];
    alert.messageText     = @"Restore the autosaved project?";
    alert.informativeText = @"INCDAW did not quit normally last time, and an "
                            @"autosave of an unsaved project exists.";
    [alert addButtonWithTitle:@"Restore"];
    [alert addButtonWithTitle:@"Discard"];

    if ([alert runModal] != NSAlertFirstButtonReturn) {
        [self removeUntitledAutosave];
        return;
    }

    const auto result = project::ProjectFile::load(*_project, recovery);

    if (!result) {
        NSAlert* failed        = [[NSAlert alloc] init];
        failed.messageText     = @"Could not restore the autosave";
        failed.informativeText = @(result.error.c_str());
        [failed runModal];
        return;
    }

    _registry->clearHistory();
    _projectPath.clear();

    [self adoptLoadedProjectRestoringStateFrom:recovery];

    // Restored contents live nowhere the user chose yet.
    [self markDirty];
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
    [self showEditorAtSegment:0];
}

- (void)showPlaylist:(id)sender
{
    (void)sender;
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
    [self showEditorAtSegment:2];
}

/// The control bar's transport cluster. Rewind and stop are separate actions
/// rather than one toggle, because a stopped transport that is already at zero
/// still has to be able to stay there.
- (void)handleTransportAction:(INCDAWTransportAction)action
{
    if (!_audioReady)
        return;

    auto& transport = _audio->transport();

    switch (action) {
        case INCDAWTransportPlay:
            [self toggleTransport];
            break;

        case INCDAWTransportStop:
            transport.stop();
            transport.seek(0);
            break;

        case INCDAWTransportRewind:
            transport.seek(0);
            break;

        case INCDAWTransportRecord:
            [self toggleRecord:nil];
            break;

        case INCDAWTransportLoop:
            transport.setLoopEnabled(!transport.isLoopEnabled());
            break;

        case INCDAWTransportMetronome:
            [self toggleMetronome:nil];
            break;
    }

    [self refreshStatus];
}

- (void)usePatternMode:(id)sender
{
    (void)sender;
    [self setSongMode:NO];
}

- (void)useSongMode:(id)sender
{
    (void)sender;
    [self setSongMode:YES];
}

/// Copies the editor's settings onto the strip that shows them.
- (void)syncPianoRollHeader
{
    self.pianoRollHeader.snapTicks           = self.pianoRoll.snapTicks;
    self.pianoRollHeader.keyRootPitchClass   = self.pianoRoll.keyRootPitchClass;
    self.pianoRollHeader.scaleIndex          = self.pianoRoll.scaleIndex;
    self.pianoRollHeader.ghostsVisible       = self.pianoRoll.ghostNotesVisible;
    self.pianoRollHeader.velocityLaneVisible = self.pianoRoll.velocityLaneVisible;

    [self.pianoRoll requestRedraw];
}

- (void)showEditorAtSegment:(NSInteger)segment
{
    self.controlBar.editorIndex = segment;

    self.pianoRoll.hidden       = segment != 0;
    self.pianoRollHeader.hidden = segment != 0;
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

/// The selection as a region, or nothing to act on. Cut/Copy/Delete demand a
/// selection — "the whole file" is a surprising thing to cut by accident.
- (BOOL)editorSelection:(engine::edits::Region*)outRegion
                  asset:(project::EntityId*)outAsset
{
    if (self.audioEditor.assetIdValue == 0 || !self.audioEditor.hasSelection)
        return NO;

    *outAsset       = project::EntityId{self.audioEditor.assetIdValue};
    outRegion->from = static_cast<engine::FrameCount>(self.audioEditor.selectionFrom);
    outRegion->to   = static_cast<engine::FrameCount>(self.audioEditor.selectionTo);
    return YES;
}

/// Reads the selected region into the clipboard. Non-destructive: no
/// command, no undo entry — there is nothing to undo.
- (BOOL)copySelectionToClipboard
{
    engine::edits::Region region;
    project::EntityId     asset;
    if (![self editorSelection:&region asset:&asset])
        return NO;

    const project::AudioAsset* record = nullptr;
    for (const project::AudioAsset& candidate : _project->audioAssets())
        if (candidate.id == asset)
            record = &candidate;
    if (record == nullptr)
        return NO;

    engine::AudioFileData data;
    const std::string path =
        !record->absolutePath.empty() ? record->absolutePath : record->relativePath;
    if (!engine::WavFile::read(path, data))
        return NO;

    _audioClipboard = engine::edits::extractRegion(data, region);
    return _audioClipboard.frameCount > 0;
}

- (void)editCopy:(id)sender
{
    (void)sender;
    (void)[self copySelectionToClipboard];
}

- (void)editCut:(id)sender
{
    (void)sender;

    engine::edits::Region region;
    project::EntityId     asset;
    if (![self editorSelection:&region asset:&asset] || ![self copySelectionToClipboard])
        return;

    (void)_registry->execute(std::make_unique<app::DeleteAudioRegionCommand>(asset, region));
    [self audioAssetChanged];
}

- (void)editDelete:(id)sender
{
    (void)sender;

    engine::edits::Region region;
    project::EntityId     asset;
    if (![self editorSelection:&region asset:&asset])
        return;

    (void)_registry->execute(std::make_unique<app::DeleteAudioRegionCommand>(asset, region));
    [self audioAssetChanged];
}

- (void)editPaste:(id)sender
{
    (void)sender;

    if (self.audioEditor.assetIdValue == 0 || _audioClipboard.frameCount <= 0)
        return;

    const project::EntityId asset{self.audioEditor.assetIdValue};

    // At the selection start when there is one, else at the very end.
    const engine::FramePosition at = self.audioEditor.hasSelection
        ? static_cast<engine::FramePosition>(self.audioEditor.selectionFrom)
        : std::numeric_limits<engine::FramePosition>::max();

    (void)_registry->execute(
        std::make_unique<app::InsertAudioCommand>(asset, at, _audioClipboard));
    [self audioAssetChanged];
}

- (void)editNormalize:(id)sender { (void)sender; [self applyAudioEdit:app::AudioEditOp::normalize factor:1.0f]; }
- (void)editReverse:(id)sender   { (void)sender; [self applyAudioEdit:app::AudioEditOp::reverse factor:1.0f]; }
- (void)editSilence:(id)sender   { (void)sender; [self applyAudioEdit:app::AudioEditOp::silence factor:1.0f]; }
- (void)editFadeIn:(id)sender    { (void)sender; [self applyAudioEdit:app::AudioEditOp::fadeIn factor:1.0f]; }
- (void)editFadeOut:(id)sender   { (void)sender; [self applyAudioEdit:app::AudioEditOp::fadeOut factor:1.0f]; }
- (void)editGainUp:(id)sender    { (void)sender; [self applyAudioEdit:app::AudioEditOp::gain factor:1.412538f]; }   // +3 dB
- (void)editGainDown:(id)sender  { (void)sender; [self applyAudioEdit:app::AudioEditOp::gain factor:0.707946f]; }   // -3 dB

/// One numeric prompt shared by the stretch verbs.
- (double)promptForValue:(NSString*)title
                 message:(NSString*)message
                 initial:(NSString*)initial
{
    NSAlert* alert        = [[NSAlert alloc] init];
    alert.messageText     = title;
    alert.informativeText = message;
    [alert addButtonWithTitle:@"Apply"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field  = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 200, 24)];
    field.stringValue   = initial;
    alert.accessoryView = field;
    alert.window.initialFirstResponder = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return std::numeric_limits<double>::quiet_NaN();

    return field.doubleValue;
}

/// Time stretch / pitch shift on the editor selection (or the whole file),
/// through engine::dsp::timeStretch — offline WSOLA with transient locking.
- (void)applyStretchWithRatio:(double)ratio semitones:(double)semitones
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
        region.to   = std::numeric_limits<engine::FrameCount>::max();
    }

    (void)_registry->execute(
        std::make_unique<app::StretchAssetCommand>(asset, region, ratio, semitones));

    [self audioAssetChanged];
}

- (void)editTimeStretch:(id)sender
{
    (void)sender;
    const double percent = [self promptForValue:@"Time Stretch"
                                        message:@"New length as a percentage of the current one."
                                        initial:@"200"];
    if (!std::isnan(percent) && percent > 1.0 && percent < 1600.0)
        [self applyStretchWithRatio:percent / 100.0 semitones:0.0];
}

- (void)editPitchShift:(id)sender
{
    (void)sender;
    const double semitones = [self promptForValue:@"Pitch Shift"
                                          message:@"Shift in semitones, -24 to +24."
                                          initial:@"0"];
    if (!std::isnan(semitones) && semitones != 0.0 && std::fabs(semitones) <= 24.0)
        [self applyStretchWithRatio:1.0 semitones:semitones];
}

/// Slices the editor's asset onto a new sampler channel: every detected hit
/// on its own key, and the current pattern replaying the loop's timing.
- (void)sliceToNewChannel:(id)sender
{
    (void)sender;

    if (self.audioEditor.assetIdValue == 0)
        return;

    const project::EntityId assetId{self.audioEditor.assetIdValue};

    const project::AudioAsset* asset = nullptr;
    for (const project::AudioAsset& candidate : _project->audioAssets())
        if (candidate.id == assetId)
            asset = &candidate;
    if (asset == nullptr)
        return;

    engine::AudioFileData data;
    const std::string path =
        !asset->absolutePath.empty() ? asset->absolutePath : asset->relativePath;
    if (!engine::WavFile::read(path, data))
        return;

    const std::vector<engine::FrameCount> onsets = engine::audio::detectOnsets(data);
    if (onsets.empty())
        return;

    if (_registry->execute(std::make_unique<app::SliceAssetCommand>(
            assetId, project::EntityId{self.pianoRoll.patternIdValue}, onsets))) {
        [self.channelRack setNeedsDisplay:YES];
        [self audioAssetChanged];
    }
}

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

- (void)automationGestureEnded:(unsigned long long)nodeId key:(const char*)key
{
    _autoWrite.gestureEnded(project::EntityId{nodeId}, key);
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

/// The click. Like input monitoring, the node exists exactly when the toggle
/// is on, so switching it is a rebuild rather than a flag the audio thread has
/// to read — and, like input monitoring, it is session state: an export
/// compiles its own graph and never sees it.
- (void)toggleMetronome:(id)sender
{
    (void)sender;

    _metronomeEnabled = !_metronomeEnabled;

    self.controlBar.metronomeOn = _metronomeEnabled;

    [self rebuildGraph];
    [self refreshStatus];
}

/// The only menu validation the shell needs so far: a checkmark that follows
/// the metronome. Everything else is enabled whenever its target responds,
/// which is AppKit's default and what this returns.
- (BOOL)validateMenuItem:(NSMenuItem*)item
{
    if (item.action == @selector(toggleMetronome:))
        item.state = _metronomeEnabled ? NSControlStateValueOn : NSControlStateValueOff;

    if (item.action == @selector(checkForUpdates:))
        return !_updateCheckInFlight;

    // While a text field is being edited, Cmd+Z belongs to the field editor.
    // Undoing a project command out from under someone renaming a channel is
    // the one thing a global Undo must not do — and a DISABLED item does not
    // swallow its key equivalent, so it falls through to the responder chain.
    const BOOL editingText = [self.window.firstResponder isKindOfClass:[NSText class]];

    if (item.action == @selector(undoFromMenu:)) {
        // The title names what will be undone. A verb on its own asks the user
        // to remember what they last did; this tells them.
        item.title = _registry->canUndo()
            ? [NSString stringWithFormat:@"Undo %s", _registry->undoName().c_str()]
            : @"Undo";

        return !editingText && _registry->canUndo();
    }

    if (item.action == @selector(redoFromMenu:)) {
        item.title = _registry->canRedo()
            ? [NSString stringWithFormat:@"Redo %s", _registry->redoName().c_str()]
            : @"Redo";

        return !editingText && _registry->canRedo();
    }

    return YES;
}

/// Tap tempo: four taps set the tempo, and every tap after that refines it.
/// The gaps are averaged rather than taken from the last pair, because a hand
/// is not a clock and one late tap should not become the tempo.
- (void)tapTempo:(id)sender
{
    (void)sender;

    const double now = std::chrono::duration<double>(
                           std::chrono::steady_clock::now().time_since_epoch())
                           .count();

    // A long gap is a new count-in, not a slow tempo.
    if (_tapTimes.empty() || now - _tapTimes.back() > 2.5)
        _tapTimes.clear();

    _tapTimes.push_back(now);

    if (_tapTimes.size() > 8)
        _tapTimes.erase(_tapTimes.begin());

    if (_tapTimes.size() < 2) {
        _lastGraphError = @"Tap tempo: keep tapping…";
        [self refreshStatus];
        return;
    }

    const double span     = _tapTimes.back() - _tapTimes.front();
    const double interval = span / static_cast<double>(_tapTimes.size() - 1);

    if (interval <= 0.0)
        return;

    [self applyTempo:60.0 / interval finished:YES];
}

/// One place where a tempo edit lands, whoever asked for it: the readout's
/// drag, its typed value, or a tap. The project is the source of truth; the
/// transport gets the same map so that its own frame-to-tick answers agree,
/// and the graph is rebuilt so the nodes render against the new tempo — they
/// hold the GRAPH's copy, which is what makes this safe while audio runs
/// (project/ProjectGraphCompiler.h).
- (void)applyTempo:(double)beatsPerMinute finished:(BOOL)finished
{
    const double tempo = app::SetTempoCommand::clampTempo(beatsPerMinute);

    // Merging, so a drag is one undo entry rather than one per mouse move —
    // the same contract a fader drag has (app/CommandRegistry.h).
    const bool changed =
        _registry->executeMerging(std::make_unique<app::SetTempoCommand>(tempo));

    if (changed || finished) {
        if (_audioReady) {
            _audio->transport().tempoMapForEdit()  = _project->tempoMap();
            _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());
        }

        // Every step of a drag rebuilds: the tempo has to be audible while the
        // gesture is happening, which is the entire point of dragging it.
        [self rebuildGraph];
        [self refreshStatus];
        [self.playlist setNeedsDisplay:YES];
        [self.pianoRoll requestRedraw];
    }
}

- (void)applyTimeSignature:(NSInteger)numerator over:(NSInteger)denominator
{
    if (!_registry->execute(std::make_unique<app::SetTimeSignatureCommand>(
            static_cast<int>(numerator), static_cast<int>(denominator))))
        return;

    if (_audioReady) {
        _audio->transport().tempoMapForEdit()  = _project->tempoMap();
        _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());
    }

    [self rebuildGraph];
    [self refreshStatus];
    [self.playlist setNeedsDisplay:YES];
    [self.pianoRoll requestRedraw];
}

/// Lands whatever the session holds. Each touched parameter is its own undo
/// entry, which is what a user riding two faders expects Cmd+Z to peel back.
- (void)landAutomationPass
{
    const project::Tick endTick =
        _audioReady ? _audio->transport().positionInTicks() : project::Tick{0};

    bool landed = false;
    for (auto& command : _autoWrite.finish(endTick))
        landed |= _registry->execute(std::move(command));

    if (landed) {
        [self rebuildGraph];
        [self.playlist setNeedsDisplay:YES];
    }
}

/// The Off / Write / Touch / Latch selector. Switching modes mid-pass lands
/// the pass first — two modes never mix inside one recorded gesture.
- (void)setAutomationMode:(NSMenuItem*)sender
{
    if (_autoWrite.isEnabled())
        [self landAutomationPass];

    switch (sender.tag) {
        case 1: _autoWrite.setMode(app::AutomationWriteSession::WriteMode::write); break;
        case 2: _autoWrite.setMode(app::AutomationWriteSession::WriteMode::touch); break;
        case 3: _autoWrite.setMode(app::AutomationWriteSession::WriteMode::latch); break;
        default: break;
    }

    _autoWrite.setEnabled(sender.tag != 0);

    for (NSMenuItem* item in sender.menu.itemArray)
        item.state = item == sender ? NSControlStateValueOn : NSControlStateValueOff;

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
- (void)setSongMode:(BOOL)songMode
{
    if (_songMode == songMode)
        return;

    _songMode = songMode;
    self.controlBar.songMode = songMode;

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

    // Whatever the user chose last time (app/AppSettings.h). The defaults are
    // still the system default device at 48 kHz and 512 frames — the block
    // size a SHARED Bluetooth output can sustain, not the lowest the hardware
    // admits to — but latency is a preference now rather than a constant.
    platform::AudioDeviceConfig config = _settings.audio;

    // Recording is opt-in: unless the input was asked for explicitly, the
    // device opens output-only and arming a take opens it on demand. Opening
    // the microphone unasked is a permission prompt nobody requested.
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

    _audioReady = YES;
    _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());

    NSLog(@"INCDAW: audio started — %s, %.0f Hz, %lld frames",
          _audio->deviceName().c_str(), _audio->sampleRate(),
          static_cast<long long>(_audio->bufferSize()));

    [self openMidiInputs];
    [self rebuildGraph];
}


// ── Settings, the device and MIDI ────────────────────────────────────────────

- (void)showSettings:(id)sender
{
    (void)sender;

    if (self.settingsWindow == nil) {
        const std::filesystem::path themes = incdawThemesDirectory();

        self.settingsWindow = [[INCDAWSettingsWindow alloc]
            initWithSettings:&_settings
             themesDirectory:themes.empty() ? nil : @(themes.c_str())];

        __weak INCDAWAppDelegate* weakSelf = self;
        self.settingsWindow.onApply        = ^{ [weakSelf applySettings]; };

        // A colour is not a device: changing one must not restart the engine,
        // so it takes its own route out of the window and only writes the file.
        self.settingsWindow.onAppearanceChanged = ^{ [weakSelf persistSettings]; };
        self.settingsWindow.statusProvider = ^NSString*(void) {
            return [weakSelf deviceStatusLine];
        };
    }

    [self.settingsWindow show];
}

/// What the device actually granted, which is not always what was asked for.
///
/// A device may refuse a rate, round a block size, or hand the callback larger
/// blocks than the property query reported. Reporting the request back would
/// look tidy and be a lie; every figure here comes from the open device
/// (docs/AUDIO_ENGINE.md §2).
- (NSString*)deviceStatusLine
{
    if (_lastSettingsMessage != nil)
        return _lastSettingsMessage;

    if (!_audioReady || _audio == nullptr)
        return @"Audio device unavailable — these settings are saved and applied at next launch.";

    const double       rate   = _audio->sampleRate();
    const std::int64_t buffer = _audio->bufferSize();
    const double       millis = rate > 0.0 ? (static_cast<double>(buffer) / rate) * 1000.0 : 0.0;

    NSString* midi = _lastMidiError != nil
                         ? [NSString stringWithFormat:@"  ·  ⚠ %@", _lastMidiError]
                         : @"";

    return [NSString stringWithFormat:
        @"%s  ·  %.0f Hz  ·  %lld frames (%.1f ms)  ·  in %lu / out %lu%@",
        _audio->deviceName().c_str(), rate, static_cast<long long>(buffer), millis,
        static_cast<unsigned long>(_audio->inputChannels()),
        static_cast<unsigned long>(_audio->outputChannels()), midi];
}

/// Reopens the device and the MIDI client with the settings just applied.
///
/// The file is written first and independently of whether the device opens:
/// someone who selects an interface that is currently asleep must still find
/// it selected at the next launch.
- (void)applySettings
{
    [self persistSettings];

    if (_recording.isRecording()) {
        // Restarting the device under a running take would truncate its file.
        _lastSettingsMessage = @"Stop recording before changing the audio device.";
        return;
    }

    _lastSettingsMessage = nil;

    if (_audio == nullptr) {
        [self startAudio];
        [self refreshStatus];
        return;
    }

    // Captured in ticks, not frames: a sample-rate change redefines what a
    // frame position means, and the playhead must stay on the same beat.
    const engine::Tick position   = _audio->transport().positionInTicks();
    const BOOL         wasPlaying = _audioReady && _audio->transport().isPlaying();

    if (wasPlaying)
        _audio->transport().stop();

    _audio->stop();

    platform::AudioDeviceConfig config = _settings.audio;

    // Input stays open if it is open now — changing the block size must not
    // silently drop input monitoring — unless the settings never asked for it.
    if (!_settings.openInputAtLaunch && _audio->inputChannels() == 0)
        config.inputDeviceIdentifier.clear();

    std::string error;
    if (!_audio->start(config, error)) {
        NSLog(@"INCDAW: audio device refused the new settings: %s", error.c_str());
        _lastSettingsMessage = [NSString stringWithFormat:@"⚠ %s", error.c_str()];

        // Fall back to the system default rather than leaving the application
        // silent: a wrong preference must not cost the user their session.
        const platform::AudioDeviceConfig fallback = app::defaultAudioConfig();

        std::string fallbackError;
        if (!_audio->start(fallback, fallbackError)) {
            NSLog(@"INCDAW: audio restart failed: %s", fallbackError.c_str());
            _audioReady = NO;
            [self refreshStatus];
            return;
        }
    }

    _audioReady = YES;
    _audio->transport().tempoMapForEdit().setSampleRate(_audio->sampleRate());
    _audio->transport().seekToTick(position);

    [self openMidiInputs];
    [self rebuildGraph];

    if (wasPlaying) {
        [self retargetLoop];
        _audio->transport().play();
    }

    [self refreshStatus];
}

/// Connects the configured MIDI sources to the engine's input, and the
/// configured destination to its output.
///
/// Until this existed, engine::MidiInput was fed by nothing but the tests: a
/// keyboard plugged into the Mac reached CoreMIDI and stopped there, which
/// made live playing and MIDI learn dead ends in the running application.
///
/// An empty identifier list connects every source — what someone who plugs in
/// a keyboard and presses a key expects, and what the platform layer already
/// means by an empty list (platform/MidiDevice.h).
- (void)openMidiInputs
{
    if (_audio == nullptr)
        return;

    // Detached before anything is closed: the engine's sender thread holds
    // this pointer, and clearing it waits for a send in flight to finish
    // (engine/midi/MidiOutput.h).
    _audio->midiOutput().setDevice(nullptr);

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

    // The destination is separate and equally non-fatal. A synthesiser that
    // was plugged in yesterday and is not today should cost a line in the
    // status bar, not a failure to open MIDI at all.
    if (!_settings.midiOutputIdentifier.empty()) {
        std::string outputError;
        if (!_midiDevice->selectOutput(_settings.midiOutputIdentifier, outputError)) {
            NSLog(@"INCDAW: MIDI output unavailable: %s", outputError.c_str());
            _lastMidiError = [NSString stringWithFormat:@"midi out: %s", outputError.c_str()];
            return;
        }
    }

    _audio->midiOutput().setDevice(_midiDevice.get());
}

// ── The update check ─────────────────────────────────────────────────────────
//
// INCDAW ships as a .dmg with no package manager behind it, so nothing tells a
// user that the version they installed has been superseded except INCDAW. It
// asks its own public release feed, compares, and reports — it never downloads
// a binary and never installs one. "Download" opens a page; what happens next
// is the user's decision, which is the whole difference between an update
// check and an auto-updater (docs/DECISIONS.md D-038).
//
// The decision itself is app::UpdateCheck and has no network in it. This is
// only the wiring: when to ask, and what to say about the answer.

/// The menu's entry. Reports whatever it found, including nothing.
- (void)checkForUpdates:(id)sender
{
    (void)sender;
    [self runUpdateCheckAnnouncing:YES];
}

/// The launch path's entry: at most once a day, silent unless there is
/// something to say.
- (void)checkForUpdatesInBackground
{
    const std::int64_t now = static_cast<std::int64_t>(std::time(nullptr));

    if (!app::automaticCheckIsDue(_settings.updates.checkAtLaunch,
                                  _settings.updates.lastCheckedUnix, now))
        return;

    [self runUpdateCheckAnnouncing:NO];
}

- (void)runUpdateCheckAnnouncing:(BOOL)announce
{
    if (_updateCheckInFlight)
        return;

    _updateCheckInFlight = YES;

    // Read here rather than in the callback: by the time the answer arrives the
    // user may have changed either of them, and the answer is about what was
    // asked, not about what the settings say a moment later.
    const std::string running = app::Version::string();
    const std::string skipped = _settings.updates.skippedVersion;

    __weak INCDAWAppDelegate* weakSelf = self;

    // Twelve seconds: long enough for a slow connection, short enough that a
    // captive portal cannot leave the check hanging for the whole session.
    platform::httpGet(app::releaseFeedUrl(), app::updateUserAgent(), 12.0,
                      [weakSelf, announce, running, skipped](platform::HttpResponse response) {
        INCDAWAppDelegate* delegate = weakSelf;
        if (delegate == nil)
            return;

        app::UpdateResult result;

        if (!response.ok()) {
            // A transport failure and an HTTP status are different things and
            // are reported as different things. Neither is an error the user
            // has to do anything about.
            result.message = !response.error.empty()
                                 ? response.error
                                 : "the release page answered with status "
                                       + std::to_string(response.statusCode);
        } else {
            result = app::evaluateFeed(response.body, running, skipped);
        }

        [delegate updateCheckFinished:result announcing:announce];
    });
}

/// Called on the main thread, exactly once per check (platform/Http.h).
- (void)updateCheckFinished:(const app::UpdateResult&)result announcing:(BOOL)announce
{
    _updateCheckInFlight = NO;

    // Logged the way the device and the MIDI client are, and for the same
    // reason: the silent path is the common one, and a check that is quietly
    // failing every launch would otherwise look identical to one that is
    // quietly succeeding.
    switch (result.verdict) {
        case app::UpdateVerdict::available:
            NSLog(@"INCDAW: update check — %s is available", result.release.version.toString().c_str());
            break;
        case app::UpdateVerdict::skipped:
            NSLog(@"INCDAW: update check — %s is available, skipped by the user",
                  result.release.version.toString().c_str());
            break;
        case app::UpdateVerdict::upToDate:
            NSLog(@"INCDAW: update check — %s is current", app::Version::string());
            break;
        case app::UpdateVerdict::unavailable:
            NSLog(@"INCDAW: update check — no answer: %s", result.message.c_str());
            break;
    }

    // Only a check that got an answer counts as one. Stamping a failed attempt
    // would let a single offline launch postpone the next try by a day.
    if (result.verdict != app::UpdateVerdict::unavailable) {
        _settings.updates.lastCheckedUnix = static_cast<std::int64_t>(std::time(nullptr));
        [self persistSettings];
    }

    switch (result.verdict) {
        case app::UpdateVerdict::available:
            [self presentUpdate:result];
            return;

        case app::UpdateVerdict::skipped:
            // Skipping silences the automatic check, not the question. Someone
            // who walks into the menu and asks gets the real answer.
            if (announce)
                [self presentUpdate:result];
            return;

        case app::UpdateVerdict::upToDate:
        case app::UpdateVerdict::unavailable:
            break;
    }

    if (!announce)
        return;

    NSAlert* alert = [[NSAlert alloc] init];

    if (result.verdict == app::UpdateVerdict::upToDate) {
        alert.messageText     = [NSString stringWithFormat:@"INCDAW %s is up to date",
                                                           app::Version::string()];
        alert.informativeText = @"No newer release has been published.";
    } else {
        alert.messageText     = @"Could not check for updates";
        alert.informativeText = @(result.message.c_str());
    }

    [alert addButtonWithTitle:@"OK"];
    (void)[alert runModal];
}

- (void)presentUpdate:(const app::UpdateResult&)result
{
    const std::string offered = result.release.version.toString();

    NSAlert* alert    = [[NSAlert alloc] init];
    alert.messageText = [NSString stringWithFormat:@"INCDAW %s is available", offered.c_str()];

    NSMutableString* body =
        [NSMutableString stringWithFormat:@"You are running %s.", app::Version::string()];

    if (!result.release.notes.empty()) {
        // Release notes are written for a page, not for a dialog. Enough to
        // decide with, and a link for the rest.
        NSString* notes = @(result.release.notes.c_str());
        if (notes.length > 600)
            notes = [[notes substringToIndex:600] stringByAppendingString:@"…"];

        [body appendFormat:@"\n\n%@", notes];
    }

    alert.informativeText = body;

    [alert addButtonWithTitle:@"Download…"];
    [alert addButtonWithTitle:@"Skip This Version"];
    [alert addButtonWithTitle:@"Later"];

    const NSModalResponse choice = [alert runModal];

    if (choice == NSAlertFirstButtonReturn) {
        // The release PAGE, never a binary. INCDAW does not replace itself, and
        // what is downloaded and when stays the user's decision.
        NSURL* page = [NSURL URLWithString:@(result.release.url.c_str())];
        if (page != nil)
            [[NSWorkspace sharedWorkspace] openURL:page];

        return;
    }

    if (choice == NSAlertSecondButtonReturn) {
        _settings.updates.skippedVersion = offered;
        [self persistSettings];
    }

    // "Later" stores nothing: the next day's check asks again, which is what
    // "later" means.
}

// ── The workspace, remembered ────────────────────────────────────────────────

/// Keeps the shell's own AppKit surfaces in step with the palette.
///
/// Views that draw themselves need nothing: they are invalidated wholesale.
/// What needs this is everything that was handed a colour once — a window's
/// background, the NSAppearance that decides what a scroller looks like — since
/// those are snapshots taken at build time, not bindings.
- (void)observePaletteChanges
{
    __weak INCDAWAppDelegate* weakSelf = self;

    [NSNotificationCenter.defaultCenter
        addObserverForName:ui::theme::kPaletteChangedNotification
                    object:nil
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification* note) {
                    (void)note;
                    [weakSelf paletteDidChange];
                }];
}

- (void)paletteDidChange
{
    if (self.window == nil)
        return;

    NSAppearance* appearance = [NSAppearance appearanceNamed:ui::theme::paletteIsLight()
                                                                 ? NSAppearanceNameAqua
                                                                 : NSAppearanceNameDarkAqua];

    self.window.appearance      = appearance;
    self.window.backgroundColor = ui::theme::ink(ui::theme::Ink::windowBackground);
    ui::theme::refreshViewTree(self.window.contentView);

    // Hosted plugin editors are INCDAW's windows too, but their content is the
    // plugin's: the ground under it follows the theme, the editor itself is
    // left alone because it is not ours to repaint.
    for (NSWindow* window in _editorWindows.allValues) {
        window.appearance      = appearance;
        window.backgroundColor = ui::theme::ink(ui::theme::Ink::windowBackground);
    }

    // Parameter panels, the Tone panel, spectrum windows and instrument panels
    // are ours all the way down, so they follow the palette completely. The
    // ones that draw themselves need only invalidating; the generic panel
    // handed its colours to AppKit controls at build time and has to hand them
    // over again, which is what refreshAppearance: is for.
    for (NSWindow* window in _panelWindows.allValues) {
        window.appearance      = appearance;
        window.backgroundColor = ui::theme::ink(ui::theme::Ink::windowBackground);

        [INCDAWInsertParameterPanel refreshAppearance:window];
        ui::theme::refreshViewTree(window.contentView);
    }
}

- (void)persistSettings
{
    if (!_settingsPath.empty())
        (void)_settings.save(_settingsPath);
}

/// Puts the window back where it was left.
///
/// A DAW window is arranged around the work — a second display, a particular
/// height — and re-centring it every launch undoes that arrangement daily.
- (void)restoreWindowFrame
{
    const app::AppSettings::Workspace& saved = _settings.workspace;

    if (saved.windowWidth < 200.0 || saved.windowHeight < 200.0) {
        [self.window center];
        return;
    }

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
    _settings.workspace.activeEditor = static_cast<int>(self.controlBar.editorIndex);
    _settings.workspace.songMode     = _songMode == YES;
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

    // Builtin insert nodes are recreated from scratch by every compile; the
    // values a panel or a MIDI mapping set on the live ones would reset to
    // defaults without this. Hosted instances persist on their own (D-031).
    const auto carriedInsertState = project::captureBuiltinInsertState(*_project, _live);

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

    options.metronomeEnabled = _metronomeEnabled == YES;

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

    project::restoreBuiltinInsertState(carriedInsertState, _live);

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

        // Parameter panels over vanished entities have no instance to notify;
        // they just close. Panels are keyed by insert slot OR channel (the
        // id space is shared), so both populations are valid keys.
        std::vector<std::uint64_t> panelKeys = slotKeys;
        for (const project::Channel& channel : _project->channels())
            panelKeys.push_back(channel.id.value());

        for (NSNumber* key in _panelWindows.allKeys)
            if (std::find(panelKeys.begin(), panelKeys.end(), key.unsignedLongLongValue)
                == panelKeys.end())
                [_panelWindows[key] close];

        // The zone editor follows its channel; the mapping list follows the
        // mappings (learn and forget mutate them from elsewhere).
        if (_zoneWindow != nil
            && _project->findChannel(project::EntityId{_zoneChannelKey}) == nullptr)
            [_zoneWindow close];

        if (_mappingWindow != nil)
            [self refreshMappingListContent];

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
        const plugins::PluginIdentifier identifier{plugins::Format::clap, located.plugin->id};

        [available addObject:@{
            @"id":   @(identifier.toString().c_str()),
            @"name": @(located.plugin->name.empty() ? located.plugin->id.c_str()
                                                    : located.plugin->name.c_str()),
        }];
    }

    // Audio Units need no scan: the system's component registry IS their
    // catalogue, and enumerating it runs no plugin code (docs/PLUGIN_HOST.md
    // §3). Instruments are left out — an insert slot is an effect.
    for (const platform::AudioUnitDescription& unit : platform::scanAudioUnits()) {
        if (unit.isInstrument)
            continue;

        const plugins::PluginIdentifier identifier{plugins::Format::audioUnit, unit.uid};

        NSString* label = unit.manufacturer.empty()
                              ? @(unit.name.c_str())
                              : [NSString stringWithFormat:@"%s — %s", unit.name.c_str(),
                                                           unit.manufacturer.c_str()];

        [available addObject:@{@"id": @(identifier.toString().c_str()), @"name": label}];
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

    plugins::HostedPlugin* instance =
        _pluginInstances != nullptr ? _pluginInstances->instanceFor(slotKey) : nullptr;

    if (instance == nullptr || !instance->hasEditor()) {
        // A builtin effect, a hosted plugin without an editor of its own, or
        // a bypassed slot: the generic parameter panel is its editor.
        [self openParameterPanelForSlotKey:slotKey];
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
    window.title = [self displayNameForSlotKey:slotKey];

    // A plugin's own window is INCDAW's window: same ground, same scheme, so a
    // hosted editor does not arrive as a light rectangle over a dark session.
    window.backgroundColor = ui::theme::ink(ui::theme::Ink::windowBackground);
    window.appearance      = [NSAppearance appearanceNamed:ui::theme::paletteIsLight()
                                                              ? NSAppearanceNameAqua
                                                              : NSAppearanceNameDarkAqua];

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
        if (plugins::HostedPlugin* instance = _pluginInstances->instanceFor(slotKey))
            instance->closeEditor();
}

// ── The generic parameter panel ──────────────────────────────────────────────

/// What the user loaded, resolved to something a window title can show: the
/// builtin catalogue's display name, the scanned catalogue's, or the uid.
- (NSString*)displayNameForSlotKey:(unsigned long long)slotKey
{
    NSString* title = @"Plugin";

    for (const project::MixerNode& node : _project->mixerNodes())
        for (const project::PluginSlot& slot : node.inserts)
            if (slot.id.value() == slotKey) {
                if (slot.plugin.format == plugins::Format::builtin)
                    if (const auto* info = engine::dsp::findBuiltinEffect(slot.plugin.uid))
                        return @(info->displayName);

                title = @(slot.plugin.uid.c_str());
            }

    for (NSDictionary* plugin in _availableInserts)
        if ([plugin[@"uid"] isEqualToString:title])
            return plugin[@"name"];

    return title;
}

- (const project::PluginSlot*)insertSlotForKey:(unsigned long long)slotKey
{
    for (const project::MixerNode& node : _project->mixerNodes())
        for (const project::PluginSlot& slot : node.inserts)
            if (slot.id.value() == slotKey)
                return &slot;

    return nullptr;
}

/// Rows for the panel: name, range and stepping from the catalogue (builtin)
/// or discovery (hosted); current values from the live node — the builtin's
/// state decoded by the same decoder loadState uses, the hosted plugin asked
/// directly. Defaults fill in where nothing live answers.
- (NSArray<NSDictionary*>*)parameterRowsForSlot:(const project::PluginSlot&)slot
{
    NSMutableArray<NSDictionary*>* rows = [NSMutableArray array];

    if (slot.plugin.format == plugins::Format::builtin) {
        const auto* info = engine::dsp::findBuiltinEffect(slot.plugin.uid);
        if (info == nullptr)
            return rows;

        std::vector<std::pair<std::uint32_t, double>> current;
        if (engine::StateIO* state = _live.insertStateFor(slot.id)) {
            std::vector<std::uint8_t> blob;
            if (state->saveState(blob))
                (void)engine::dsp::BuiltinEffect::decodeState(blob.data(), blob.size(),
                                                              current);
        }

        for (std::size_t index = 0; index < info->parameterCount; ++index) {
            const engine::dsp::EffectParameter& parameter = info->parameters[index];

            double value = parameter.defaultValue;
            for (const auto& [id, live] : current)
                if (id == parameter.id)
                    value = live;

            [rows addObject:@{
                @"id":      @(parameter.id),
                @"name":    @(parameter.name),
                @"min":     @(parameter.minValue),
                @"max":     @(parameter.maxValue),
                @"value":   @(value),
                @"stepped": @(parameter.stepped),
            }];
        }

        return rows;
    }

    const std::vector<plugins::PluginParameterInfo>* discovered =
        _pluginInstances != nullptr ? _pluginInstances->parametersFor(slot.plugin.uid)
                                    : nullptr;
    if (discovered == nullptr)
        return rows;

    plugins::HostedPlugin* instance =
        _pluginInstances != nullptr ? _pluginInstances->instanceFor(slot.id.value()) : nullptr;

    for (const plugins::PluginParameterInfo& parameter : *discovered) {
        double value = parameter.defaultValue;
        if (instance != nullptr)
            (void)instance->readParameter(parameter.id, value);

        [rows addObject:@{
            @"id":      @(parameter.id),
            @"name":    @(parameter.name.c_str()),
            @"min":     @(parameter.minValue),
            @"max":     @(parameter.maxValue),
            @"value":   @(value),
            @"stepped": @(parameter.stepped),
        }];
    }

    return rows;
}

/// Every slider move lands here. Resolving the sink on each write is the
/// point: sinks die with their graph, and the panel outlives rebuilds.
- (void)writeInsertParameter:(unsigned long long)slotKey
                 parameterId:(std::uint32_t)parameterId
                  plainValue:(double)plainValue
{
    engine::ParameterSink* sink = _live.insertSinkFor(project::EntityId{slotKey});
    if (sink == nullptr)
        return;   // bypassed, or rebuilt away: the move has nowhere to land

    sink->setParameter(parameterId, plainValue);

    // The model only records this at save (state capture), but it is project
    // state the user means to keep.
    [self markDirty];
}

- (void)openParameterPanelForSlotKey:(unsigned long long)slotKey
{
    NSNumber* key = @(slotKey);

    if (NSWindow* existing = _panelWindows[key]) {
        [existing makeKeyAndOrderFront:nil];
        return;
    }

    const project::PluginSlot* slot = [self insertSlotForKey:slotKey];
    if (slot == nullptr) {
        NSBeep();
        return;
    }

    // The analyzer's surface is a picture, not sliders.
    if (slot->plugin.format == plugins::Format::builtin
        && slot->plugin.uid == "incdaw.analyzer") {
        [self openSpectrumWindowForSlotKey:slotKey];
        return;
    }

    NSArray<NSDictionary*>* rows = [self parameterRowsForSlot:*slot];
    if (rows.count == 0) {
        // A hosted plugin that reports no parameters: nothing to show.
        NSBeep();
        return;
    }

    __weak INCDAWAppDelegate* weakSelf = self;

    void (^write)(std::uint32_t, double) = ^(std::uint32_t parameterId, double plainValue) {
        [weakSelf writeInsertParameter:slotKey
                           parameterId:parameterId
                            plainValue:plainValue];
    };

    NSWindow* window = nil;

    // Tone is the three-band EQ wearing a mixing desk's face: knobs over a
    // response curve rather than seven sliders. `incdaw.eq` keeps the generic
    // panel, which is the difference between the two catalogue entries.
    if (slot->plugin.format == plugins::Format::builtin
        && slot->plugin.uid == "incdaw.tone")
        window = [INCDAWTonePanel makePanelWithTitle:[self displayNameForSlotKey:slotKey]
                                                rows:rows
                                          sampleRate:_audio != nullptr
                                                         ? _audio->sampleRate() : 48000.0
                                             onWrite:write];

    if (window == nil)
        window = [INCDAWInsertParameterPanel
            makePanelWithTitle:[self displayNameForSlotKey:slotKey]
                          rows:rows
                       onWrite:write];

    [window center];
    [window makeKeyAndOrderFront:nil];

    _panelWindows[key] = window;

    // Self-clearing: unlike a plugin editor there is no instance to notify,
    // so the close only removes the table entries.
    _panelObservers[key] = [NSNotificationCenter.defaultCenter
        addObserverForName:NSWindowWillCloseNotification
                    object:window
                     queue:nil
                usingBlock:^(NSNotification*) {
                    [weakSelf panelWindowClosedForSlotKey:slotKey];
                }];
}

- (void)panelWindowClosedForSlotKey:(unsigned long long)slotKey
{
    NSNumber* key = @(slotKey);

    if (id observer = _panelObservers[key])
        [NSNotificationCenter.defaultCenter removeObserver:observer];

    [_panelObservers removeObjectForKey:key];
    [_panelWindows removeObjectForKey:key];
}

- (void)openSpectrumWindowForSlotKey:(unsigned long long)slotKey
{
    NSNumber* key = @(slotKey);

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 420, 220)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                          | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.releasedWhenClosed = NO;
    window.title              = [self displayNameForSlotKey:slotKey];

    INCDAWSpectrumView* view =
        [[INCDAWSpectrumView alloc] initWithFrame:NSMakeRect(0, 0, 420, 220)];
    view.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    window.contentView    = view;

    [window center];
    [window makeKeyAndOrderFront:nil];

    _panelWindows[key] = window;

    __weak INCDAWAppDelegate* weakSelf = self;
    _panelObservers[key] = [NSNotificationCenter.defaultCenter
        addObserverForName:NSWindowWillCloseNotification
                    object:window
                     queue:nil
                usingBlock:^(NSNotification*) {
                    [weakSelf panelWindowClosedForSlotKey:slotKey];
                }];
}

/// Open panels follow the live values at ~5 Hz, so automation, MIDI knobs
/// and undo become visible without any panel owning an engine pointer.
/// Spectrum windows repaint at the full housekeeping rate — a spectrum at
/// 5 Hz reads as broken.
- (void)refreshOpenPanelValues
{
    if (_panelWindows.count == 0)
        return;

    for (NSNumber* key in _panelWindows.allKeys) {
        NSWindow* window = _panelWindows[key];
        if (![window.contentView isKindOfClass:[INCDAWSpectrumView class]])
            continue;

        auto* node = _live.insertNodeFor(project::EntityId{key.unsignedLongLongValue});
        auto* analyzer = dynamic_cast<engine::dsp::AnalyzerEffect*>(node);
        if (analyzer == nullptr)
            continue;

        std::vector<float> bins;
        if (analyzer->readSpectrum(bins))
            [static_cast<INCDAWSpectrumView*>(window.contentView)
                updateWithBins:bins
                    sampleRate:analyzer->analysisSampleRate()];
    }

    if (++_panelRefreshTick % 6 != 0)
        return;

    for (NSNumber* key in _panelWindows.allKeys) {
        const unsigned long long entityKey = key.unsignedLongLongValue;

        NSMutableDictionary<NSNumber*, NSNumber*>* values = [NSMutableDictionary dictionary];

        if (const project::PluginSlot* slot = [self insertSlotForKey:entityKey]) {
            if (slot->plugin.format == plugins::Format::builtin) {
                std::vector<std::uint8_t>                     blob;
                std::vector<std::pair<std::uint32_t, double>> decoded;

                if (engine::StateIO* state = _live.insertStateFor(slot->id))
                    if (state->saveState(blob)
                        && engine::dsp::BuiltinEffect::decodeState(blob.data(), blob.size(),
                                                                   decoded))
                        for (const auto& [parameterId, value] : decoded)
                            values[@(parameterId)] = @(value);
            } else if (plugins::HostedPlugin* instance =
                           _pluginInstances != nullptr
                               ? _pluginInstances->instanceFor(entityKey)
                               : nullptr) {
                if (const auto* discovered =
                        _pluginInstances->parametersFor(slot->plugin.uid))
                    for (const plugins::PluginParameterInfo& parameter : *discovered) {
                        double value = 0.0;
                        if (instance->readParameter(parameter.id, value))
                            values[@(parameter.id)] = @(value);
                    }
            }
        } else if (const project::Channel* channel =
                       _project->findChannel(project::EntityId{entityKey})) {
            // The MODEL is the instrument panel's truth (D-034): undo and
            // redo show up; an entry undo removed falls back to its default.
            if (const auto* info = [self instrumentInfoForChannel:*channel]) {
                for (std::size_t index = 0; index < info->parameterCount; ++index)
                    values[@(info->parameters[index].id)] =
                        @(info->parameters[index].defaultValue);

                for (const project::ChannelInstrumentParameter& stored :
                     channel->instrumentParameters)
                    values[@(stored.parameterId)] = @(stored.value);
            }
        }

        if (values.count > 0) {
            [INCDAWInsertParameterPanel refreshWindow:_panelWindows[key] values:values];
            [INCDAWTonePanel refreshWindow:_panelWindows[key] values:values];
        }
    }
}

// ── The instrument panel, the zone editor, the mapping list ──────────────────

/// The channel's instrument as a catalogue entry: the builtin it names, or
/// the reference synth for a channel with no instrument yet (which is what
/// the compiler's default factory plays for it).
- (const engine::BuiltinInstrumentInfo*)instrumentInfoForChannel:(const project::Channel&)channel
{
    if (channel.instrument.uid.empty())
        return engine::findBuiltinInstrument(plugins::builtinSimpleSynth().uid);

    if (channel.instrument.format != plugins::Format::builtin)
        return nullptr;

    return engine::findBuiltinInstrument(channel.instrument.uid);
}

- (void)openInstrumentPanelForChannel:(unsigned long long)channelKey
{
    NSNumber* key = @(channelKey);

    if (NSWindow* existing = _panelWindows[key]) {
        [existing makeKeyAndOrderFront:nil];
        return;
    }

    const project::Channel* channel = _project->findChannel(project::EntityId{channelKey});
    if (channel == nullptr) {
        NSBeep();
        return;
    }

    const engine::BuiltinInstrumentInfo* info = [self instrumentInfoForChannel:*channel];
    if (info == nullptr) {
        NSBeep();   // a hosted instrument would edit through its own editor
        return;
    }

    NSMutableArray<NSDictionary*>* rows = [NSMutableArray array];

    for (std::size_t index = 0; index < info->parameterCount; ++index) {
        const engine::dsp::EffectParameter& parameter = info->parameters[index];

        // The model is the panel's truth, exactly as it is the compiler's.
        double value = parameter.defaultValue;
        for (const project::ChannelInstrumentParameter& stored : channel->instrumentParameters)
            if (stored.parameterId == parameter.id)
                value = stored.value;

        [rows addObject:@{
            @"id":      @(parameter.id),
            @"name":    @(parameter.name),
            @"min":     @(parameter.minValue),
            @"max":     @(parameter.maxValue),
            @"value":   @(value),
            @"stepped": @(parameter.stepped),
        }];
    }

    __weak INCDAWAppDelegate* weakSelf = self;

    NSString* title = [NSString stringWithFormat:@"%s — %s", channel->name.c_str(),
                                                 info->displayName];

    NSWindow* window = [INCDAWInsertParameterPanel
        makePanelWithTitle:title
                      rows:rows
                   onWrite:^(std::uint32_t parameterId, double plainValue) {
                       [weakSelf writeInstrumentParameter:channelKey
                                              parameterId:parameterId
                                               plainValue:plainValue];
                   }];

    [window center];
    [window makeKeyAndOrderFront:nil];

    _panelWindows[key] = window;

    _panelObservers[key] = [NSNotificationCenter.defaultCenter
        addObserverForName:NSWindowWillCloseNotification
                    object:window
                     queue:nil
                usingBlock:^(NSNotification*) {
                    [weakSelf panelWindowClosedForSlotKey:channelKey];
                }];
}

/// The undoable half of an instrument slider move: the value lands in the
/// MODEL (merging, so a drag is one entry) and the rebuild applies it — the
/// same source-of-truth rule the mixer's fader follows (D-034).
- (void)writeInstrumentParameter:(unsigned long long)channelKey
                     parameterId:(std::uint32_t)parameterId
                      plainValue:(double)plainValue
{
    if (_registry->executeMerging(std::make_unique<app::SetInstrumentParameterCommand>(
            project::EntityId{channelKey}, parameterId, plainValue)))
        [self rebuildGraph];
}

// ── The zone editor ──────────────────────────────────────────────────────────

- (NSString*)assetDisplayName:(project::EntityId)assetId
{
    for (const project::AudioAsset& asset : _project->audioAssets())
        if (asset.id == assetId)
            return @(std::filesystem::path{asset.absolutePath}.stem().string().c_str());

    return @"missing";
}

- (void)openZoneEditorForChannel:(unsigned long long)channelKey
{
    if (_zoneWindow != nil && _zoneChannelKey == channelKey) {
        [_zoneWindow makeKeyAndOrderFront:nil];
        return;
    }

    [_zoneWindow close];
    _zoneWindow     = nil;
    _zoneChannelKey = channelKey;

    const project::Channel* channel = _project->findChannel(project::EntityId{channelKey});
    if (channel == nullptr) {
        NSBeep();
        return;
    }

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 660, 300)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.releasedWhenClosed = NO;
    window.title = [NSString stringWithFormat:@"%s — Sampler Zones", channel->name.c_str()];

    _zoneWindow = window;
    [self refreshZoneEditorContent];

    [window center];
    [window makeKeyAndOrderFront:nil];

    __weak INCDAWAppDelegate* weakSelf = self;
    [NSNotificationCenter.defaultCenter addObserverForName:NSWindowWillCloseNotification
                                                    object:window
                                                     queue:nil
                                                usingBlock:^(NSNotification*) {
                                                    INCDAWAppDelegate* strongSelf = weakSelf;
                                                    if (strongSelf != nil
                                                        && strongSelf->_zoneWindow != nil)
                                                        strongSelf->_zoneWindow = nil;
                                                }];
}

/// Rebuilds the zone list from the model. Every change routes through a
/// command and back here, so what the window shows is always what the
/// project holds — clamps included.
- (void)refreshZoneEditorContent
{
    if (_zoneWindow == nil)
        return;

    const project::Channel* channel =
        _project->findChannel(project::EntityId{_zoneChannelKey});
    if (channel == nullptr) {
        [_zoneWindow close];
        return;
    }

    constexpr CGFloat rowHeight = 30.0;
    constexpr CGFloat width     = 660.0;
    const CGFloat listHeight    = rowHeight * static_cast<CGFloat>(channel->samplerZones.size());
    const CGFloat contentHeight = 20 + 24 + listHeight + 24 + 38;

    INCDAWFlippedView* document =
        [[INCDAWFlippedView alloc] initWithFrame:NSMakeRect(0, 0, width, contentHeight)];

    _zoneRows = [NSMutableArray array];

    // The header row names the columns once.
    const struct { const char* title; CGFloat x; CGFloat width; } columns[] = {
        {"Sample", 10, 130},  {"Root", 148, 50},   {"Key lo", 202, 50},
        {"Key hi", 256, 50},  {"Vel lo", 310, 50}, {"Vel hi", 364, 50},
        {"Gain", 418, 56},    {"Rev", 482, 40},
    };

    for (const auto& column : columns) {
        NSTextField* header = [NSTextField labelWithString:@(column.title)];
        header.frame        = NSMakeRect(column.x, 4, column.width, 16);
        header.font         = [NSFont systemFontOfSize:10];
        header.textColor    = ui::theme::ink(ui::theme::Ink::textSecondary);
        [document addSubview:header];
    }

    for (std::size_t index = 0; index < channel->samplerZones.size(); ++index) {
        const project::ChannelSamplerZone& zone = channel->samplerZones[index];
        const CGFloat y = 24 + rowHeight * static_cast<CGFloat>(index);

        NSTextField* name = [NSTextField labelWithString:[self assetDisplayName:zone.asset]];
        name.frame         = NSMakeRect(10, y + 5, 130, 18);
        name.lineBreakMode = NSLineBreakByTruncatingTail;
        [document addSubview:name];

        NSMutableDictionary* controls = [NSMutableDictionary dictionary];

        const struct { const char* key; CGFloat x; CGFloat width; double value; } fields[] = {
            {"rootKey", 148, 50, static_cast<double>(zone.rootKey)},
            {"keyLow", 202, 50, static_cast<double>(zone.keyLow)},
            {"keyHigh", 256, 50, static_cast<double>(zone.keyHigh)},
            {"velocityLow", 310, 50, static_cast<double>(zone.velocityLow)},
            {"velocityHigh", 364, 50, static_cast<double>(zone.velocityHigh)},
            {"gain", 418, 56, zone.gain},
        };

        for (const auto& field : fields) {
            NSTextField* editor =
                [[NSTextField alloc] initWithFrame:NSMakeRect(field.x, y, field.width, 22)];
            editor.font        = [NSFont monospacedDigitSystemFontOfSize:11
                                                                  weight:NSFontWeightRegular];
            editor.stringValue = [NSString stringWithFormat:@"%g", field.value];
            editor.tag         = static_cast<NSInteger>(index);
            editor.target      = self;
            editor.action      = @selector(zoneFieldChanged:);
            [document addSubview:editor];
            controls[@(field.key)] = editor;
        }

        NSButton* reverse  = [NSButton checkboxWithTitle:@""
                                                  target:self
                                                  action:@selector(zoneFieldChanged:)];
        reverse.frame = NSMakeRect(482, y + 2, 40, 20);
        reverse.state = zone.reverse ? NSControlStateValueOn : NSControlStateValueOff;
        reverse.tag   = static_cast<NSInteger>(index);
        [document addSubview:reverse];
        controls[@"reverse"] = reverse;

        NSButton* remove = [NSButton buttonWithTitle:@"Remove"
                                              target:self
                                              action:@selector(zoneRemovePressed:)];
        remove.frame         = NSMakeRect(530, y, 90, 24);
        remove.bezelStyle    = NSBezelStyleRounded;
        remove.controlSize   = NSControlSizeSmall;
        remove.tag           = static_cast<NSInteger>(index);
        [document addSubview:remove];

        [_zoneRows addObject:controls];
    }

    NSButton* add = [NSButton buttonWithTitle:@"Add Sample Layer…"
                                       target:self
                                       action:@selector(zoneAddPressed:)];
    add.frame = NSMakeRect(10, 24 + listHeight + 8, 160, 26);
    [document addSubview:add];

    if (channel->samplerZones.empty()) {
        NSTextField* hint = [NSTextField
            labelWithString:@"No zones. Add a layer, or use the rack's Load Sample… "
                            @"to make this channel a sampler first."];
        hint.frame     = NSMakeRect(180, 24 + listHeight + 12, width - 190, 18);
        hint.font      = [NSFont systemFontOfSize:11];
        hint.textColor = ui::theme::ink(ui::theme::Ink::textSecondary);
        [document addSubview:hint];
    }

    const CGFloat visibleHeight = std::min(contentHeight, 420.0);

    NSScrollView* scroll =
        [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, width, visibleHeight)];
    scroll.hasVerticalScroller = contentHeight > 420.0;
    scroll.documentView        = document;

    [_zoneWindow setContentSize:NSMakeSize(width, visibleHeight)];
    _zoneWindow.contentView = scroll;
}

- (void)zoneFieldChanged:(NSControl*)sender
{
    const auto index = static_cast<std::size_t>(sender.tag);

    const project::Channel* channel =
        _project->findChannel(project::EntityId{_zoneChannelKey});
    if (channel == nullptr || index >= channel->samplerZones.size()
        || index >= _zoneRows.count)
        return;

    NSDictionary* controls = _zoneRows[index];

    // The whole row is read back, so one commit can never tear a zone into
    // a half-old, half-new state.
    project::ChannelSamplerZone zone = channel->samplerZones[index];

    const auto intField = [&](NSString* key, int low, int high) {
        NSTextField* field = controls[key];
        return std::clamp(static_cast<int>(field.integerValue), low, high);
    };

    zone.rootKey      = intField(@"rootKey", 0, 127);
    zone.keyLow       = intField(@"keyLow", 0, 127);
    zone.keyHigh      = intField(@"keyHigh", 0, 127);
    zone.velocityLow  = intField(@"velocityLow", 1, 127);
    zone.velocityHigh = intField(@"velocityHigh", 1, 127);
    zone.gain    = std::clamp(static_cast<NSTextField*>(controls[@"gain"]).doubleValue,
                              0.0, 4.0);
    zone.reverse = static_cast<NSButton*>(controls[@"reverse"]).state
                   == NSControlStateValueOn;

    if (_registry->executeMerging(std::make_unique<app::SetSamplerZoneCommand>(
            project::EntityId{_zoneChannelKey}, index, zone)))
        [self rebuildGraph];

    [self refreshZoneEditorContent];
}

- (void)zoneRemovePressed:(NSButton*)sender
{
    if (_registry->execute(std::make_unique<app::RemoveSamplerZoneCommand>(
            project::EntityId{_zoneChannelKey}, static_cast<std::size_t>(sender.tag))))
        [self rebuildGraph];

    [self refreshZoneEditorContent];
}

- (void)zoneAddPressed:(NSButton*)sender
{
    (void)sender;

    const project::Channel* channel =
        _project->findChannel(project::EntityId{_zoneChannelKey});
    if (channel == nullptr)
        return;

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = YES;
    panel.canChooseDirectories    = NO;
    panel.allowsMultipleSelection = NO;
    panel.allowedContentTypes     = @[ [UTType typeWithFilenameExtension:@"wav"] ];
    panel.prompt                  = @"Add";

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    // A channel that is not a sampler yet gets the whole Load Sample gesture;
    // a sampler gets a layer appended.
    std::unique_ptr<app::Command> command;
    if (channel->instrument == plugins::builtinSampler())
        command = std::make_unique<app::AddSamplerZoneCommand>(
            project::EntityId{_zoneChannelKey}, panel.URL.path.UTF8String);
    else
        command = std::make_unique<app::LoadSampleCommand>(project::EntityId{_zoneChannelKey},
                                                           panel.URL.path.UTF8String);

    if (_registry->execute(std::move(command)))
        [self rebuildGraph];

    [self refreshZoneEditorContent];
    [self.channelRack setNeedsDisplay:YES];
}

// ── The mapping list ─────────────────────────────────────────────────────────

- (NSString*)mappingTargetDisplayName:(project::EntityId)target
{
    for (const project::MixerNode& node : _project->mixerNodes())
        if (node.id == target)
            return @(node.name.c_str());

    for (const project::Channel& channel : _project->channels())
        if (channel.id == target)
            return @(channel.name.c_str());

    for (const project::MixerNode& node : _project->mixerNodes())
        for (const project::PluginSlot& slot : node.inserts)
            if (slot.id == target)
                return [self displayNameForSlotKey:target.value()];

    return [NSString stringWithFormat:@"#%llu", target.value()];
}

- (void)showMidiMappings:(id)sender
{
    (void)sender;

    if (_mappingWindow != nil) {
        [self refreshMappingListContent];
        [_mappingWindow makeKeyAndOrderFront:nil];
        return;
    }

    NSWindow* window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 460, 240)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                    backing:NSBackingStoreBuffered
                      defer:NO];
    window.releasedWhenClosed = NO;
    window.title              = @"MIDI Mappings";

    _mappingWindow = window;
    [self refreshMappingListContent];

    [window center];
    [window makeKeyAndOrderFront:nil];

    __weak INCDAWAppDelegate* weakSelf = self;
    [NSNotificationCenter.defaultCenter addObserverForName:NSWindowWillCloseNotification
                                                    object:window
                                                     queue:nil
                                                usingBlock:^(NSNotification*) {
                                                    INCDAWAppDelegate* strongSelf = weakSelf;
                                                    if (strongSelf != nil)
                                                        strongSelf->_mappingWindow = nil;
                                                }];
}

- (void)refreshMappingListContent
{
    if (_mappingWindow == nil)
        return;

    constexpr CGFloat rowHeight = 28.0;
    constexpr CGFloat width     = 460.0;

    const auto& mappings = _project->midiMappings();

    const CGFloat listHeight =
        std::max<CGFloat>(rowHeight * static_cast<CGFloat>(mappings.size()), rowHeight);
    const CGFloat contentHeight = 16 + listHeight;

    INCDAWFlippedView* document =
        [[INCDAWFlippedView alloc] initWithFrame:NSMakeRect(0, 0, width, contentHeight)];

    _mappingRowIds = [NSMutableArray array];

    if (mappings.empty()) {
        NSTextField* hint =
            [NSTextField labelWithString:@"No mappings. Right-click a mixer strip and "
                                         @"choose MIDI Learn to create one."];
        hint.frame     = NSMakeRect(10, 12, width - 20, 18);
        hint.textColor = ui::theme::ink(ui::theme::Ink::textSecondary);
        [document addSubview:hint];
    }

    for (std::size_t index = 0; index < mappings.size(); ++index) {
        const project::MidiMapping& mapping = mappings[index];
        const CGFloat y = 8 + rowHeight * static_cast<CGFloat>(index);

        NSString* channelText = mapping.midiChannel < 0
            ? @"any"
            : [NSString stringWithFormat:@"%d", mapping.midiChannel + 1];

        NSTextField* label = [NSTextField
            labelWithString:[NSString stringWithFormat:@"CC %d (ch %@)  →  %s  ·  %@",
                                                       mapping.controller, channelText,
                                                       mapping.parameterKey.c_str(),
                                                       [self mappingTargetDisplayName:
                                                                 mapping.targetEntity]]];
        label.frame         = NSMakeRect(10, y + 4, width - 110, 18);
        label.font          = [NSFont monospacedDigitSystemFontOfSize:11
                                                               weight:NSFontWeightRegular];
        label.lineBreakMode = NSLineBreakByTruncatingTail;
        [document addSubview:label];

        NSButton* remove = [NSButton buttonWithTitle:@"Remove"
                                              target:self
                                              action:@selector(mappingRemovePressed:)];
        remove.frame       = NSMakeRect(width - 95, y, 85, 24);
        remove.bezelStyle  = NSBezelStyleRounded;
        remove.controlSize = NSControlSizeSmall;
        remove.tag         = static_cast<NSInteger>(index);
        [document addSubview:remove];

        [_mappingRowIds addObject:@(mapping.id.value())];
    }

    const CGFloat visibleHeight = std::min(contentHeight, 380.0);

    NSScrollView* scroll =
        [[NSScrollView alloc] initWithFrame:NSMakeRect(0, 0, width, visibleHeight)];
    scroll.hasVerticalScroller = contentHeight > 380.0;
    scroll.documentView        = document;

    [_mappingWindow setContentSize:NSMakeSize(width, visibleHeight)];
    _mappingWindow.contentView = scroll;
}

- (void)mappingRemovePressed:(NSButton*)sender
{
    const auto index = static_cast<NSUInteger>(sender.tag);
    if (index >= _mappingRowIds.count)
        return;

    const project::EntityId mappingId{[_mappingRowIds[index] unsignedLongLongValue]};

    if (_registry->execute(std::make_unique<app::RemoveMidiMappingCommand>(mappingId)))
        [self rebuildGraph];

    [self refreshMappingListContent];
}

// ── The project as a document ────────────────────────────────────────────────

/// The starter document: one channel, one pattern carrying the demo phrase,
/// one track with the pattern placed. What launch shows, and what File > New
/// resets to.
- (void)seedStarterProject
{
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
}

- (void)markDirty
{
    _dirty                     = YES;
    self.window.documentEdited = YES;
}

- (void)clearDirty
{
    _dirty                     = NO;
    self.window.documentEdited = NO;
}

- (void)refreshWindowTitle
{
    self.window.title = _projectPath.empty()
        ? @"INCDAW — Untitled"
        : [NSString stringWithFormat:@"INCDAW — %s",
                                     _projectPath.stem().string().c_str()];
}

/// The guard in front of anything that discards the open project: quit, New,
/// Open. YES means proceed — the project is clean, was saved here, or the
/// user chose to drop it.
- (BOOL)confirmDiscardChanges
{
    if (!_dirty)
        return YES;

    NSAlert* alert    = [[NSAlert alloc] init];
    alert.messageText = _projectPath.empty()
        ? @"Save changes to Untitled?"
        : [NSString stringWithFormat:@"Save changes to %s?",
                                     _projectPath.stem().string().c_str()];
    alert.informativeText = @"Unsaved changes are lost otherwise.";
    [alert addButtonWithTitle:@"Save"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert addButtonWithTitle:@"Don't Save"];

    const NSModalResponse choice = [alert runModal];

    if (choice == NSAlertFirstButtonReturn)
        return [self saveInteractive];

    return choice == NSAlertThirdButtonReturn;
}

- (void)newProject:(id)sender
{
    (void)sender;

    if (![self confirmDiscardChanges])
        return;

    *_project = project::Project{};
    [self seedStarterProject];

    _registry->clearHistory();
    _projectPath.clear();

    [self adoptLoadedProjectRestoringStateFrom:std::filesystem::path{}];
    [self clearDirty];
}

- (NSArray<NSString*>*)recentProjects
{
    NSArray* stored =
        [[NSUserDefaults standardUserDefaults] arrayForKey:kRecentProjectsKey];

    NSMutableArray<NSString*>* result = [NSMutableArray array];
    for (id entry in stored)
        if ([entry isKindOfClass:[NSString class]])
            [result addObject:entry];

    return result;
}

- (void)storeRecentProjects:(const std::vector<std::string>&)list
{
    NSMutableArray<NSString*>* stored = [NSMutableArray array];
    for (const std::string& entry : list)
        [stored addObject:@(entry.c_str())];

    [[NSUserDefaults standardUserDefaults] setObject:stored
                                              forKey:kRecentProjectsKey];
    [self refreshRecentMenu];
}

- (void)noteRecentProject:(const std::filesystem::path&)path
{
    std::vector<std::string> list;
    for (NSString* entry in [self recentProjects])
        list.emplace_back(entry.UTF8String);

    [self storeRecentProjects:app::session::updatedRecents(std::move(list),
                                                           path.string(),
                                                           kRecentProjectsCap)];
}

- (void)refreshRecentMenu
{
    [_recentMenu removeAllItems];

    for (NSString* stored in [self recentProjects]) {
        const std::filesystem::path path{stored.UTF8String};

        NSMenuItem* item =
            [_recentMenu addItemWithTitle:@(path.stem().string().c_str())
                                   action:@selector(openRecentProject:)
                            keyEquivalent:@""];
        item.target            = self;
        item.representedObject = stored;
    }

    if (_recentMenu.numberOfItems > 0)
        [_recentMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* clear = [_recentMenu addItemWithTitle:@"Clear Menu"
                                               action:@selector(clearRecentProjects:)
                                        keyEquivalent:@""];
    clear.target = self;
}

- (void)clearRecentProjects:(id)sender
{
    (void)sender;
    [self storeRecentProjects:std::vector<std::string>{}];
}

- (void)openRecentProject:(NSMenuItem*)sender
{
    NSString* stored = sender.representedObject;
    if (![stored isKindOfClass:[NSString class]])
        return;

    if (![self confirmDiscardChanges])
        return;

    if ([self loadProjectPackageAtPath:std::filesystem::path{stored.UTF8String}])
        return;

    // Gone or unreadable: a menu that keeps offering it can only fail again.
    std::vector<std::string> remaining;
    for (NSString* entry in [self recentProjects])
        if (![entry isEqualToString:stored])
            remaining.emplace_back(entry.UTF8String);

    [self storeRecentProjects:remaining];
}

- (std::filesystem::path)untitledAutosavePath
{
    return app::session::autosavePathFor({}, incdawSupportDirectory());
}

- (void)removeUntitledAutosave
{
    const std::filesystem::path path = [self untitledAutosavePath];
    if (path.empty())
        return;

    std::error_code ignored;
    std::filesystem::remove_all(path, ignored); // a package is a directory
}

/// The safety net, not a save: touches neither the dirty flag, the title,
/// the recents, nor the user's file. Failures log — an alert every two
/// minutes would be worse than no autosave at all.
- (void)autosaveTick
{
    if (!_dirty)
        return;

    const std::filesystem::path destination =
        app::session::autosavePathFor(_projectPath, incdawSupportDirectory());
    if (destination.empty())
        return;

    std::error_code ignored;
    std::filesystem::create_directories(destination.parent_path(), ignored);

    for (const std::string& warning :
         project::capturePluginState(*_project, _live, destination))
        NSLog(@"INCDAW: autosave: %s", warning.c_str());

    const auto result = project::ProjectFile::save(*_project, destination);
    if (!result)
        NSLog(@"INCDAW: autosave failed: %s", result.error.c_str());
}

/// Roots, favourites and recents, written where the next launch will find
/// them. A failure is logged and dropped: browser state is a convenience,
/// and nothing the user is doing should stop because it could not be saved.
- (void)saveBrowserSettings
{
    if (_browserSettings.empty())
        return;

    std::string error;

    if (!_browser.save(_browserSettings, error))
        NSLog(@"INCDAW: could not save browser settings: %s", error.c_str());
}

/// Auditioning a sample from the Browser.
///
/// The decode goes through the shell's SampleCache, so previewing a file that
/// a channel already uses costs nothing and previewing one repeatedly costs
/// nothing twice. The engine holds only a raw pointer into what the cache
/// owns; housekeeping releases replaced buffers once the block counter has
/// moved past them (engine/audio/AuditionPlayer.h).
- (void)browserPreview:(NSString*)path
{
    if (path == nil || !_audioReady)
        return;

    const std::filesystem::path chosen{path.UTF8String};

    if (!app::Browser::canDecodeAudio(chosen)) {
        _lastGraphError = [NSString stringWithFormat:@"Cannot preview %s — WAV only so far",
                                                     chosen.filename().string().c_str()];
        return;
    }

    std::string error;
    const std::shared_ptr<const engine::AudioFileData> audio = _sampleCache->load(chosen, error);

    if (audio == nullptr) {
        _lastGraphError = @(error.c_str());
        return;
    }

    _audio->audition().play(audio, _audio->sampleRate(), 1.0F, _audio->blockCount());

    _browser.noteRecent(chosen);
    [self saveBrowserSettings];
}

- (void)stopBrowserPreview
{
    if (_audio != nullptr)
        _audio->audition().stop();
}

/// What double-clicking in the Browser means.
///
/// Projects open and MIDI files import. Audio is remembered as recent and
/// otherwise waits for the parts that give it somewhere to go — preview, and
/// dragging into the rack or the playlist. The recents list is about what the
/// user reached for, not about what INCDAW managed to do with it.
- (void)browserActivated:(NSString*)path
{
    if (path == nil)
        return;

    const std::filesystem::path chosen{path.UTF8String};
    const app::BrowserItemKind  kind = app::Browser::classify(chosen);

    _browser.noteRecent(chosen);
    [self saveBrowserSettings];
    [self.browserPane reload];

    if (kind == app::BrowserItemKind::project) {
        if ([self confirmDiscardChanges])
            (void)[self loadProjectPackageAtPath:chosen];

        return;
    }

    if (kind == app::BrowserItemKind::midi) {
        [self importMidiFromPath:chosen];
        return;
    }

    if (kind != app::BrowserItemKind::audio)
        return;

    if (!app::Browser::canDecodeAudio(chosen)) {
        _lastGraphError = [NSString stringWithFormat:@"Cannot load %s — WAV only so far",
                                                     chosen.filename().string().c_str()];
        return;
    }

    // Double-clicking a sample is the keyboard-free version of dragging it
    // into the rack, and lands the same command.
    auto  command  = std::make_unique<app::ImportSampleAsChannelCommand>(chosen.string());
    auto* imported = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    [self selectChannel:imported->channelId().value()];
    [self.channelRack setNeedsDisplay:YES];
    [self rebuildGraph];
}

/// Hiding the Browser is a workspace change, not a mode: the split view
/// re-lays out around it and the panes beside it keep their proportions.
- (void)toggleBrowser:(id)sender
{
    const BOOL hidden       = !self.browserPane.isHidden;
    self.browserPane.hidden = hidden;

    NSView* parent = self.browserPane.superview;

    if ([parent isKindOfClass:[NSSplitView class]])
        [(NSSplitView*)parent adjustSubviews];

    if ([sender isKindOfClass:[NSMenuItem class]])
        ((NSMenuItem*)sender).state = hidden ? NSControlStateValueOff : NSControlStateValueOn;
}

- (void)saveProject:(id)sender
{
    (void)sender;
    (void)[self saveInteractive];
}

- (BOOL)saveInteractive
{
    if (_projectPath.empty())
        return [self saveAsInteractive];

    return [self writeProjectToPath];
}

- (void)saveProjectAs:(id)sender
{
    (void)sender;
    (void)[self saveAsInteractive];
}

- (BOOL)saveAsInteractive
{
    NSSavePanel* panel = [NSSavePanel savePanel];
    panel.nameFieldStringValue = @"Untitled.incdaw";
    panel.canCreateDirectories = YES;

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return NO;

    std::filesystem::path chosen{panel.URL.path.UTF8String};
    if (chosen.extension() != ".incdaw")
        chosen += ".incdaw";

    _projectPath = chosen;
    return [self writeProjectToPath];
}

/// Renders `jobs` in order on a background thread behind a modal progress
/// window with Cancel. The project and tempo map are COPIED first, so edits
/// after the panel closes cannot race the renderer; the shared sample cache
/// is thread-safe by design. Errors are shown; a cancel is silent.
- (void)runExportJobs:(std::vector<ExportJob>)jobs
{
    if (jobs.empty())
        return;

    NSWindow* progress = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 380, 110)
                  styleMask:NSWindowStyleMaskTitled
                    backing:NSBackingStoreBuffered
                      defer:NO];
    progress.title = @"Exporting…";

    NSTextField* label = [NSTextField labelWithString:@(jobs.front().displayName.c_str())];
    label.frame         = NSMakeRect(20, 66, 340, 18);
    label.lineBreakMode = NSLineBreakByTruncatingTail;
    [progress.contentView addSubview:label];

    NSProgressIndicator* bar =
        [[NSProgressIndicator alloc] initWithFrame:NSMakeRect(20, 44, 340, 16)];
    bar.indeterminate = NO;
    bar.minValue      = 0.0;
    bar.maxValue      = 1.0;
    [progress.contentView addSubview:bar];

    NSButton* cancel = [NSButton buttonWithTitle:@"Cancel"
                                          target:self
                                          action:@selector(exportCancelPressed:)];
    cancel.frame = NSMakeRect(280, 8, 80, 28);
    [progress.contentView addSubview:cancel];

    _exportCancel = std::make_shared<std::atomic<bool>>(false);

    // Shared with the renderer thread; the modal loop below only polls.
    auto fraction = std::make_shared<std::atomic<double>>(0.0);
    auto jobIndex = std::make_shared<std::atomic<std::size_t>>(0);
    auto done     = std::make_shared<std::atomic<bool>>(false);
    auto failed   = std::make_shared<std::atomic<bool>>(false);
    auto errorBox = std::make_shared<std::string>();   // written before `done`

    auto projectCopy = std::make_shared<project::Project>(*_project);
    auto tempoCopy = std::make_shared<engine::TempoMap>(_audio->transport().tempoMap());
    auto jobList   = std::make_shared<std::vector<ExportJob>>(std::move(jobs));
    auto cancelled = _exportCancel;

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        for (std::size_t index = 0; index < jobList->size(); ++index) {
            jobIndex->store(index, std::memory_order_relaxed);

            ExportJob& job = (*jobList)[index];

            job.options.progress = [&](double f) {
                fraction->store((static_cast<double>(index) + f)
                                    / static_cast<double>(jobList->size()),
                                std::memory_order_relaxed);
                return !cancelled->load(std::memory_order_relaxed);
            };

            const auto rendered = project::renderProjectToFile(
                *projectCopy, *tempoCopy, job.options, job.destination);

            for (const std::string& warning : rendered.warnings)
                NSLog(@"INCDAW: export %s: %s", job.displayName.c_str(), warning.c_str());

            if (!rendered) {
                if (rendered.error != "cancelled") {
                    *errorBox = job.displayName + ": " + rendered.error;
                    failed->store(true, std::memory_order_release);
                }
                break;
            }
        }

        done->store(true, std::memory_order_release);
    });

    [progress center];
    [progress makeKeyAndOrderFront:nil];

    // Poll rather than dispatch to the main queue: the modal runloop mode is
    // not guaranteed to drain it, and atomics have no such dependency.
    NSModalSession session = [NSApp beginModalSessionForWindow:progress];

    while (!done->load(std::memory_order_acquire)) {
        if ([NSApp runModalSession:session] != NSModalResponseContinue)
            break;

        bar.doubleValue = fraction->load(std::memory_order_relaxed);

        const std::size_t index = jobIndex->load(std::memory_order_relaxed);
        if (index < jobList->size())
            label.stringValue = @((*jobList)[index].displayName.c_str());

        [NSThread sleepForTimeInterval:1.0 / 30.0];
    }

    // The window closes only after the renderer thread is done with the
    // shared state — cancel makes that quick.
    while (!done->load(std::memory_order_acquire))
        [NSThread sleepForTimeInterval:1.0 / 100.0];

    [NSApp endModalSession:session];
    [progress close];
    _exportCancel.reset();

    if (failed->load(std::memory_order_acquire)) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Could not export audio";
        alert.informativeText = @(errorBox->c_str());
        [alert runModal];
    }
}

- (void)exportCancelPressed:(id)sender
{
    (void)sender;

    if (_exportCancel != nullptr)
        _exportCancel->store(true, std::memory_order_relaxed);
}

- (void)exportAudio:(id)sender
{
    (void)sender;

    if (!_audioReady)
        return;

    // ── The options dialog: everything the renderer already implements ──────
    constexpr CGFloat labelWidth   = 110.0;
    constexpr CGFloat controlWidth = 190.0;
    constexpr CGFloat rowStep      = 30.0;
    constexpr int     rowCount     = 8;

    NSView* accessory = [[NSView alloc]
        initWithFrame:NSMakeRect(0, 0, labelWidth + controlWidth + 8, rowStep * rowCount)];

    __block int rowIndex = rowCount;
    NSRect (^nextControlFrame)(void) = ^NSRect {
        rowIndex -= 1;
        return NSMakeRect(labelWidth + 8, rowStep * rowIndex + 4, controlWidth, 24);
    };
    void (^addLabel)(NSString*) = ^(NSString* text) {
        NSTextField* label = [NSTextField labelWithString:text];
        label.frame     = NSMakeRect(0, rowStep * rowIndex + 7, labelWidth, 18);
        label.alignment = NSTextAlignmentRight;
        [accessory addSubview:label];
    };

    NSPopUpButton* targetPopup = [[NSPopUpButton alloc] initWithFrame:nextControlFrame()];
    [targetPopup addItemsWithTitles:@[
        @"Master", @"Stems — one file per mixer track", @"Tracks — one file per channel"
    ]];
    addLabel(@"Export");
    [accessory addSubview:targetPopup];

    NSPopUpButton* formatPopup = [[NSPopUpButton alloc] initWithFrame:nextControlFrame()];
    [formatPopup addItemsWithTitles:@[ @"WAV", @"AIFF" ]];
    addLabel(@"Format");
    [accessory addSubview:formatPopup];

    NSPopUpButton* depthPopup = [[NSPopUpButton alloc] initWithFrame:nextControlFrame()];
    [depthPopup addItemsWithTitles:@[ @"32-bit float", @"24-bit PCM", @"16-bit PCM" ]];
    addLabel(@"Bit depth");
    [accessory addSubview:depthPopup];

    NSPopUpButton* ratePopup = [[NSPopUpButton alloc] initWithFrame:nextControlFrame()];
    [ratePopup addItemWithTitle:[NSString stringWithFormat:@"Engine rate (%.0f Hz)",
                                                           _audio->sampleRate()]];
    [ratePopup addItemsWithTitles:@[ @"44100 Hz", @"48000 Hz", @"88200 Hz", @"96000 Hz" ]];
    addLabel(@"Sample rate");
    [accessory addSubview:ratePopup];

    NSTextField* tailField = [[NSTextField alloc] initWithFrame:nextControlFrame()];
    tailField.stringValue  = @"2.0";
    addLabel(@"Tail (seconds)");
    [accessory addSubview:tailField];

    NSButton* normalizeCheck = [NSButton checkboxWithTitle:@"Normalize peak to 1.0"
                                                    target:nil
                                                    action:nil];
    normalizeCheck.frame = nextControlFrame();
    addLabel(@"");
    [accessory addSubview:normalizeCheck];

    NSButton* ditherCheck = [NSButton checkboxWithTitle:@"TPDF dither (16-bit)"
                                                 target:nil
                                                 action:nil];
    ditherCheck.frame = nextControlFrame();
    ditherCheck.state = NSControlStateValueOn;
    addLabel(@"");
    [accessory addSubview:ditherCheck];

    NSButton* loopCheck = [NSButton checkboxWithTitle:@"Only the loop range"
                                               target:nil
                                               action:nil];
    loopCheck.frame = nextControlFrame();
    addLabel(@"");
    [accessory addSubview:loopCheck];

    NSAlert* dialog        = [[NSAlert alloc] init];
    dialog.messageText     = @"Export Audio";
    dialog.informativeText = @"Rendered offline through the same graph playback uses.";
    dialog.accessoryView   = accessory;
    [dialog addButtonWithTitle:@"Export…"];
    [dialog addButtonWithTitle:@"Cancel"];

    if ([dialog runModal] != NSAlertFirstButtonReturn)
        return;

    // ── Options out of the controls ─────────────────────────────────────────
    project::RenderOptions options;
    options.sampleRate  = _audio->sampleRate();
    options.sampleCache = _sampleCache.get();
    options.parameters  = &_parameters;
    options.normalize   = normalizeCheck.state == NSControlStateValueOn;
    options.dither      = ditherCheck.state == NSControlStateValueOn;
    options.tailSeconds = std::clamp(tailField.doubleValue, 0.0, 60.0);

    switch (depthPopup.indexOfSelectedItem) {
        case 1: options.bitDepth = project::RenderOptions::BitDepth::pcm24; break;
        case 2: options.bitDepth = project::RenderOptions::BitDepth::pcm16; break;
        default: options.bitDepth = project::RenderOptions::BitDepth::float32; break;
    }

    constexpr double rates[] = {0.0, 44100.0, 48000.0, 88200.0, 96000.0};
    options.targetSampleRate = rates[ratePopup.indexOfSelectedItem];

    if (loopCheck.state == NSControlStateValueOn) {
        const auto start = _audio->transport().loopStart();
        const auto end   = _audio->transport().loopEnd();
        if (end > start) {
            options.regionStart  = start;
            options.regionLength = static_cast<engine::FrameCount>(end - start);
        }
    }

    NSString* extension = formatPopup.indexOfSelectedItem == 1 ? @".aiff" : @".wav";

    // ── Master: one file through the save panel ─────────────────────────────
    if (targetPopup.indexOfSelectedItem == 0) {
        NSSavePanel* panel = [NSSavePanel savePanel];
        panel.nameFieldStringValue = [@"Master" stringByAppendingString:extension];
        panel.canCreateDirectories = YES;
        panel.allowedContentTypes  = @[
            [UTType typeWithFilenameExtension:@"wav"],
            [UTType typeWithFilenameExtension:@"aiff"]
        ];

        if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
            return;

        std::vector<ExportJob> jobs;
        jobs.push_back({std::filesystem::path{panel.URL.path.UTF8String}, options, "Master"});
        [self runExportJobs:std::move(jobs)];
        return;
    }

    // ── Stems or tracks: one file per source into a chosen directory ────────
    const bool stems = targetPopup.indexOfSelectedItem == 1;

    std::vector<std::pair<std::string, project::EntityId>> sources;

    if (stems) {
        for (const project::MixerNode& node : _project->mixerNodes())
            if (node.id != _project->masterMixerNode())
                sources.emplace_back(node.name, node.id);
    } else {
        for (const project::Channel& channel : _project->channels())
            sources.emplace_back(channel.name, channel.id);
    }

    if (sources.empty()) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Nothing to export";
        alert.informativeText = stems ? @"The project has no mixer tracks besides the master."
                                      : @"The project has no channels.";
        [alert runModal];
        return;
    }

    NSOpenPanel* directoryPanel        = [NSOpenPanel openPanel];
    directoryPanel.canChooseFiles      = NO;
    directoryPanel.canChooseDirectories = YES;
    directoryPanel.canCreateDirectories = YES;
    directoryPanel.prompt              = @"Export Here";

    if ([directoryPanel runModal] != NSModalResponseOK || directoryPanel.URL == nil)
        return;

    const std::filesystem::path directory{directoryPanel.URL.path.UTF8String};

    std::vector<std::string> taken;
    std::vector<ExportJob>   jobs;

    for (const auto& [name, entity] : sources) {
        project::RenderOptions perSource = options;
        if (stems)
            perSource.stemMixerNode = entity;
        else
            perSource.soloChannel = entity;

        const std::string fileName =
            app::session::exportFileName(name, extension.UTF8String, taken);
        taken.push_back(fileName);

        jobs.push_back({directory / fileName, perSource, fileName});
    }

    // The first failure stops the run with its name; finished files stay.
    [self runExportJobs:std::move(jobs)];
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

/// Importing a Standard MIDI File, from the panel or from the Browser.
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
    // result visible immediately. No command also means the undo-depth watch
    // cannot see this mutation — the document is marked dirty here.
    [self markDirty];

    [self.patternList setNeedsDisplay:YES];
    [self.channelRack setNeedsDisplay:YES];
    [self rebuildGraph];
}

/// The save itself. Live plugin state is captured FIRST, so the stateFile
/// paths land in the project.json this save writes (docs/PLUGIN_HOST.md §6).
- (BOOL)writeProjectToPath
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
        return NO;
    }

    // What was protecting unsaved work is stale now that the work is saved.
    [self removeUntitledAutosave];

    [self noteRecentProject:_projectPath];
    [self clearDirty];
    [self refreshWindowTitle];
    return YES;
}

- (void)openProject:(id)sender
{
    (void)sender;

    if (![self confirmDiscardChanges])
        return;

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = YES;
    panel.canChooseDirectories    = YES;   // a package is a directory
    panel.allowsMultipleSelection = NO;

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    (void)[self loadProjectPackageAtPath:std::filesystem::path{
                                             panel.URL.path.UTF8String}];
}

/// Shared by Open… and Open Recent: validates, prefers a newer autosave when
/// one exists, loads, adopts. NO means the alert was already shown.
- (BOOL)loadProjectPackageAtPath:(const std::filesystem::path&)chosen
{
    if (!project::ProjectFile::isProjectPackage(chosen)) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Not an INCDAW project";
        alert.informativeText = @(chosen.string().c_str());
        [alert runModal];
        return NO;
    }

    // A sibling autosave newer than the project means the last session ended
    // without a save. Offer it — ⌘S still points at the real project either
    // way, which is also why opening the autosave marks the document dirty.
    std::filesystem::path source       = chosen;
    bool                  fromAutosave = false;

    const std::filesystem::path autosave =
        app::session::autosavePathFor(chosen, incdawSupportDirectory());

    if (app::session::autosaveIsNewer(chosen, autosave)) {
        NSAlert* ask        = [[NSAlert alloc] init];
        ask.messageText     = @"Open the newer autosave?";
        ask.informativeText = @"An autosave newer than this project exists, "
                              @"so the last session probably ended without a "
                              @"save. Nothing is overwritten until you save.";
        [ask addButtonWithTitle:@"Open Autosave"];
        [ask addButtonWithTitle:@"Open Saved Version"];

        if ([ask runModal] == NSAlertFirstButtonReturn) {
            source       = autosave;
            fromAutosave = true;
        }
    }

    // Loaded IN PLACE: every view holds a pointer to this project object, and
    // ProjectFile::load replaces its contents wholesale. Undo history against
    // the previous contents no longer applies to what is now here.
    const auto result = project::ProjectFile::load(*_project, source);

    if (!result) {
        NSAlert* alert        = [[NSAlert alloc] init];
        alert.messageText     = @"Could not open the project";
        alert.informativeText = @(result.error.c_str());
        [alert runModal];
        return NO;
    }

    if (result.migrated)
        NSLog(@"INCDAW: project migrated from format %s", result.migratedFrom.c_str());

    _registry->clearHistory();
    _projectPath = chosen;

    [self noteRecentProject:chosen];
    [self adoptLoadedProjectRestoringStateFrom:source];

    if (fromAutosave)
        [self markDirty];
    else
        [self clearDirty];

    return YES;
}

/// Points everything at what was just loaded: tempo, views, the graph — and
/// hands hosted plugins their state back once the graph that owns them
/// exists. `statePackage` is the package the contents actually came from
/// (the project, or its autosave); empty means there is no state to restore,
/// which is what File > New wants.
- (void)adoptLoadedProjectRestoringStateFrom:(const std::filesystem::path&)statePackage
{
    // Cross-project safety: _live's handles belong to the OLD contents, and
    // entity ids restart per project, so a stale table could alias a new
    // slot id onto an old node — the rebuild below must start from nothing.
    // (This is also what keeps the builtin-state carry-over from leaking
    // values across projects.) Open parameter panels show the old project;
    // they close rather than mislead.
    _live = project::CompiledProjectGraph{};
    for (NSWindow* window in [_panelWindows.allValues copy])
        [window close];
    [_zoneWindow close];
    [_mappingWindow close];

    // The undo stack was cleared along with the old contents; syncing the
    // watch keeps housekeeping from reading that as an edit. The caches the
    // watch would have invalidated are invalidated here instead.
    _undoDepthSeen = _registry->undoDepth();
    [self.playlist invalidateWaveformCache];
    if (!self.audioEditor.hidden && self.audioEditor.assetIdValue != 0)
        [self.audioEditor reloadWaveform];

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

    [self rebuildGraph];

    if (_audioReady && !statePackage.empty())
        for (const std::string& warning :
             project::restorePluginState(*_project, _live, statePackage))
            NSLog(@"INCDAW: open: %s", warning.c_str());

    [self retargetLoop];

    [self.pianoRoll setNeedsDisplay:YES];
    [self.channelRack setNeedsDisplay:YES];
    [self.patternList setNeedsDisplay:YES];
    [self.playlist setNeedsDisplay:YES];
    [self.mixer setNeedsDisplay:YES];
    [self.audioEditor setNeedsDisplay:YES];

    [self refreshWindowTitle];
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
    // opens the microphone when the settings chose not to open it at launch.
    platform::AudioDeviceConfig config = _settings.audio;
    if (config.inputDeviceIdentifier.empty())
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

    // The preview's decoded audio, released on the same terms as a retired
    // graph: the audio thread held a raw pointer into it.
    _audio->audition().collect(_audio->blockCount(), !_audio->isRunning());

    // A plugin that changed its latency mid-life (clap_host_latency.changed)
    // needs a recompile before delay compensation is honest again.
    if (_pluginInstances != nullptr && _pluginInstances->refreshChangedLatencies())
        [self rebuildGraph];

    [self refreshOpenPanelValues];

    if (_learnArmed) {
        const std::uint64_t packed = _audio->midiInput().lastControlChange();
        if (packed != _learnControlSeen && packed != 0)
            [self completeMidiLearnWithPacked:packed];
    }

    // An undo or redo may have rewritten the file the editor and the
    // playlist's clip waveforms are showing.
    if (_registry->undoDepth() != _undoDepthSeen) {
        _undoDepthSeen = _registry->undoDepth();

        // Every command-based mutation passes here, undo and redo included —
        // undoing away from the last save leaves the document dirty too.
        [self markDirty];

        // Whatever moved history — a pane's own Cmd+Z, the menu, the palette —
        // the Piano Roll's selection indices may no longer exist.
        [self.pianoRoll pruneSelectionAfterHistoryChange];

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

    // A MIDI client that would not open is not fatal — the DAW plays without a
    // keyboard — but it must not be silent either, or a controller that has
    // stopped working is indistinguishable from one that never worked.
    if (_lastMidiError != nil)
        audio = [NSString stringWithFormat:@"%@ · ⚠ %@", audio, _lastMidiError];

    const project::Channel* channel =
        _project->findChannel(project::EntityId{self.pianoRoll.channelIdValue});

    self.statusBar.text = [NSString stringWithFormat:
        @"INCDAW %s  ·  %s / %s  ·  %lu notes  ·  %lu clips  ·  %@  ·  %@  ·  "
        @"space: play   r: rec   ⌘K: commands",
        app::Version::string(),
        pattern != nullptr ? pattern->name.c_str() : "—",
        channel != nullptr ? channel->name.c_str() : "—",
        static_cast<unsigned long>(noteCount),
        static_cast<unsigned long>(_project->clips().size()), audio, undo];

    [self refreshControlBarWithPattern:pattern];
}

/// Hands the chrome the handful of numbers it displays. Everything here is read
/// from the engine or the project — the bar caches none of it, so a stale
/// display is impossible.
- (void)refreshControlBarWithPattern:(const project::Pattern*)pattern
{
    INCDAWControlBarView* bar = self.controlBar;
    if (bar == nil)
        return;

    bar.songMode    = _songMode;
    bar.recording   = _recording.isRecording();
    bar.contextName = _songMode
        ? @"Arrangement"
        : @(pattern != nullptr ? pattern->name.c_str() : "—");

    bar.alert       = _lastGraphError != nil ? _lastGraphError : _lastRecordError;
    bar.metronomeOn = _metronomeEnabled;

    // The signature the display counts bars in. Read from the project rather
    // than assumed: it has been in the tempo map and in the file format since
    // the beginning, and the display was the only thing still saying 4/4.
    const engine::TimeSignature signature = _project->tempoMap().timeSignatureAtTick(0);
    bar.beatsPerBar = signature.numerator;
    bar.beatValue   = signature.denominator;

    if (!_audioReady) {
        bar.playing = NO;
        bar.tempo   = _project->tempoMap().tempoAtTick(0);
        [bar setNeedsDisplay:YES];
        return;
    }

    const auto& transport = _audio->transport();

    bar.playing      = transport.isPlaying();
    bar.looping      = transport.isLoopEnabled();
    bar.playheadTick = transport.isPlaying()
                           ? transport.tempoMap().tickForFrame(transport.position())
                           : -1;

    bar.tempo   = transport.tempoMap().tempoAtFrame(transport.position());
    bar.cpuLoad = _audio->profiler().peakLoad();

    // The output meter reads the master strip of the graph that is rendering
    // right now; between rebuilds there is simply nothing to read.
    if (engine::dsp::MixerStripNode* master = _live.stripFor(_project->masterMixerNode())) {
        bar.masterPeak = std::min(1.0, static_cast<double>(master->meter().peak()));
        bar.masterRms  = std::min(1.0, static_cast<double>(master->meter().rms()));
    } else {
        bar.masterPeak = 0.0;
        bar.masterRms  = 0.0;
    }

    [bar setNeedsDisplay:YES];
}

// ── Actions, command search and history ──────────────────────────────────────

/// Everything the palette can run, gathered fresh each time it opens.
///
/// The menu bar is walked rather than duplicated: an action that has a menu
/// entry is listed with the same title and the same shortcut, and can never
/// drift from it. Registered project actions come next, and undo/redo last —
/// those two are synthesised because their titles depend on what is on the
/// stack right now.
- (NSArray<INCDAWCommandEntry*>*)paletteEntries
{
    NSMutableArray<INCDAWCommandEntry*>* entries = [NSMutableArray array];

    for (NSMenuItem* top in NSApp.mainMenu.itemArray)
        [self collectMenuItemsOf:top.submenu into:entries category:top.submenu.title];

    __weak INCDAWAppDelegate* weakSelf = self;

    for (const app::CommandRegistry::Entry& action : _registry->actions()) {
        NSString* identifier = @(action.id.c_str());

        [entries addObject:[INCDAWCommandEntry
            entryWithTitle:@(action.displayName.c_str())
                  category:@(action.category.c_str())
                  shortcut:@(action.defaultShortcut.c_str())
                       run:^{ [weakSelf invokeAction:identifier]; }]];
    }

    if (_registry->canUndo()) {
        [entries addObject:[INCDAWCommandEntry
            entryWithTitle:[NSString stringWithFormat:@"Undo %s", _registry->undoName().c_str()]
                  category:@"Edit"
                  shortcut:@"⌘Z"
                       run:^{ [weakSelf undoFromMenu:nil]; }]];
    }

    if (_registry->canRedo()) {
        [entries addObject:[INCDAWCommandEntry
            entryWithTitle:[NSString stringWithFormat:@"Redo %s", _registry->redoName().c_str()]
                  category:@"Edit"
                  shortcut:@"⇧⌘Z"
                       run:^{ [weakSelf redoFromMenu:nil]; }]];
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
        [self historyChanged];
}

- (void)showCommandPalette:(id)sender
{
    (void)sender;

    if (self.palette == nil)
        self.palette = [[INCDAWCommandPalette alloc] init];

    [self.palette showWithEntries:[self paletteEntries] relativeToWindow:self.window];
}

- (void)undoFromMenu:(id)sender
{
    (void)sender;

    if (_registry->undo())
        [self historyChanged];
}

- (void)redoFromMenu:(id)sender
{
    (void)sender;

    if (_registry->redo())
        [self historyChanged];
}

/// One place history lands, whichever route moved it — the menu, the palette,
/// or a registered action.
///
/// The Piano Roll indexes its selection by position in the pattern's note
/// vector, so a step that removed notes leaves those indices pointing past the
/// end and the next draw would read them. It used to be pruned only by the
/// Piano Roll's own key handler, which meant an undo taken from anywhere else
/// left it stale.
- (void)historyChanged
{
    [self.pianoRoll pruneSelectionAfterHistoryChange];

    [self markDirty];
    [self rebuildGraph];

    [self.channelRack setNeedsDisplay:YES];
    [self.patternList setNeedsDisplay:YES];
    [self.playlist setNeedsDisplay:YES];
    [self.mixer setNeedsDisplay:YES];
    [self.pianoRoll requestRedraw];

    [self refreshStatus];
}

- (void)buildMenu
{
    NSMenu* menuBar = [[NSMenu alloc] init];

    NSMenuItem* appItem = [[NSMenuItem alloc] init];
    [menuBar addItem:appItem];

    NSMenu* appMenu = [[NSMenu alloc] init];

    // Cmd+, — where every macOS application keeps this, and the first place
    // anyone looks when the sound comes out of the wrong interface.
    NSMenuItem* settingsItem = [appMenu addItemWithTitle:@"Settings…"
                                                  action:@selector(showSettings:)
                                           keyEquivalent:@","];
    settingsItem.target = self;

    // Command search (CLAUDE.md §26). A DAW accumulates hundreds of actions,
    // and a menu bar stops being a way to find them long before that.
    NSMenuItem* paletteItem = [appMenu addItemWithTitle:@"Command Search…"
                                                 action:@selector(showCommandPalette:)
                                          keyEquivalent:@"k"];
    paletteItem.target = self;

    [appMenu addItem:[NSMenuItem separatorItem]];

    // Asking is always available, whatever the launch preference says.
    NSMenuItem* updateItem = [appMenu addItemWithTitle:@"Check for Updates…"
                                                action:@selector(checkForUpdates:)
                                         keyEquivalent:@""];
    updateItem.target = self;

    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:@"Quit INCDAW" action:@selector(terminate:) keyEquivalent:@"q"];
    appItem.submenu = appMenu;

    // File: the project as a document. Until this menu existed, everything a
    // session produced was lost on quit — ProjectFile worked and was tested,
    // but nothing called it.
    NSMenuItem* fileItem = [[NSMenuItem alloc] init];
    [menuBar addItem:fileItem];

    NSMenu* fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    [fileMenu addItemWithTitle:@"New" action:@selector(newProject:) keyEquivalent:@"n"];
    [fileMenu addItemWithTitle:@"Open…" action:@selector(openProject:) keyEquivalent:@"o"];

    NSMenuItem* openRecentItem = [[NSMenuItem alloc] initWithTitle:@"Open Recent"
                                                            action:nil
                                                     keyEquivalent:@""];
    _recentMenu            = [[NSMenu alloc] initWithTitle:@"Open Recent"];
    openRecentItem.submenu = _recentMenu;
    [fileMenu addItem:openRecentItem];
    [self refreshRecentMenu];

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

    NSMenuItem* browserItem = [viewMenu addItemWithTitle:@"Browser"
                                                  action:@selector(toggleBrowser:)
                                           keyEquivalent:@"b"];
    browserItem.target = self;
    browserItem.state  = NSControlStateValueOn;

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

    // Undo and redo route to app::CommandRegistry rather than to whichever pane
    // has focus. They used to be inert placeholders that only worked because
    // every view implemented Cmd+Z itself — five copies of one behaviour, and
    // a menu that could not say what it would undo (docs/ARCHITECTURE.md §6).
    // Select All stays on the responder chain: what "all" means is the pane's
    // question, not the registry's.
    NSMenu* editMenu = [[NSMenu alloc] initWithTitle:@"Edit"];

    NSMenuItem* undoItem = [editMenu addItemWithTitle:@"Undo"
                                               action:@selector(undoFromMenu:)
                                        keyEquivalent:@"z"];
    undoItem.target = self;

    NSMenuItem* redoItem = [editMenu addItemWithTitle:@"Redo"
                                               action:@selector(redoFromMenu:)
                                        keyEquivalent:@"Z"];
    redoItem.target = self;

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
        {@"Cut",               @selector(editCut:)},
        {@"Copy",              @selector(editCopy:)},
        {@"Paste",             @selector(editPaste:)},
        {@"Delete Selection",  @selector(editDelete:)},
        {@"Trim to Selection", @selector(editTrim:)},
        {@"Normalize",         @selector(editNormalize:)},
        {@"Reverse",           @selector(editReverse:)},
        {@"Silence",           @selector(editSilence:)},
        {@"Fade In",           @selector(editFadeIn:)},
        {@"Fade Out",          @selector(editFadeOut:)},
        {@"Gain +3 dB",        @selector(editGainUp:)},
        {@"Gain -3 dB",        @selector(editGainDown:)},
        {@"Time Stretch…",     @selector(editTimeStretch:)},
        {@"Pitch Shift…",      @selector(editPitchShift:)},
        {@"Slice to New Channel", @selector(sliceToNewChannel:)},
    };

    for (const auto& verb : verbs) {
        NSMenuItem* item = [audioMenu addItemWithTitle:verb.title
                                                action:verb.action
                                         keyEquivalent:@""];
        item.target = self;
    }

    [audioMenu addItem:[NSMenuItem separatorItem]];

    // Automation recording: while a mode is active and the transport rolls,
    // every fader and pan move in the mixer is captured; selecting Off (or
    // switching modes) lands the passes as lanes and clips. Write spans the
    // whole pass, touch records only while a control is held, latch holds
    // from the first touch to the stop.
    NSMenuItem* recordAutomationItem = [[NSMenuItem alloc] initWithTitle:@"Record Automation"
                                                                  action:nil
                                                           keyEquivalent:@""];
    NSMenu* automationModeMenu = [[NSMenu alloc] initWithTitle:@"Record Automation"];

    const struct { NSString* title; NSInteger tag; } automationModes[] = {
        {@"Off", 0}, {@"Write", 1}, {@"Touch", 2}, {@"Latch", 3},
    };

    for (const auto& mode : automationModes) {
        NSMenuItem* item = [automationModeMenu addItemWithTitle:mode.title
                                                         action:@selector(setAutomationMode:)
                                                  keyEquivalent:@""];
        item.target = self;
        item.tag    = mode.tag;
        item.state  = mode.tag == 0 ? NSControlStateValueOn : NSControlStateValueOff;
    }

    recordAutomationItem.submenu = automationModeMenu;
    [audioMenu addItem:recordAutomationItem];

    NSMenuItem* metronomeItem = [audioMenu addItemWithTitle:@"Metronome"
                                                     action:@selector(toggleMetronome:)
                                              keyEquivalent:@"m"];
    metronomeItem.target = self;
    metronomeItem.keyEquivalentModifierMask = NSEventModifierFlagCommand
                                            | NSEventModifierFlagShift;

    NSMenuItem* tapItem = [audioMenu addItemWithTitle:@"Tap Tempo"
                                               action:@selector(tapTempo:)
                                        keyEquivalent:@"t"];
    tapItem.target = self;
    tapItem.keyEquivalentModifierMask = NSEventModifierFlagCommand
                                      | NSEventModifierFlagShift;

    [audioMenu addItem:[NSMenuItem separatorItem]];

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

    [audioMenu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* mappingsItem = [audioMenu addItemWithTitle:@"MIDI Mappings…"
                                                    action:@selector(showMidiMappings:)
                                             keyEquivalent:@""];
    mappingsItem.target = self;

    audioItem.submenu = audioMenu;

    NSApp.mainMenu = menuBar;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
    (void)sender;

    if (![self confirmDiscardChanges])
        return NSTerminateCancel;

    // A normal quit leaves nothing to recover; only a crash leaves the
    // unsaved-project autosave behind for the next launch to offer.
    [self removeUntitledAutosave];
    return NSTerminateNow;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    (void)notification;

    // Stop the device before the graph it is reading goes away. A recording
    // in flight is finished first — quitting must not truncate a take's file.
    [_housekeeping invalidate];
    _housekeeping = nil;

    [_autosave invalidate];
    _autosave = nil;

    if (_audio != nullptr) {
        if (_recording.isRecording())
            (void)_recording.finish(*_audio);

        _audio->stop();
    }

    // The MIDI client delivers on its own thread and holds a reference to the
    // engine's input: it must be closed before the engine goes away. The
    // engine's sender holds a pointer the other way, so that is detached
    // first — _audio->stop() has already flushed and stopped it.
    if (_midiDevice != nullptr) {
        if (_audio != nullptr)
            _audio->midiOutput().setDevice(nullptr);

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
