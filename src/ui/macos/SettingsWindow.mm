#include "ui/macos/SettingsWindow.h"

#import "ui/macos/Theme.h"

#include "app/AppSettings.h"
#include "platform/AudioDevice.h"
#include "platform/MidiDevice.h"
#include "ui/ThemeLibrary.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
namespace theme = incdaw::ui::theme;
using incdaw::ui::theme::Ink;

namespace {

constexpr CGFloat windowWidth  = 560.0;
constexpr CGFloat windowHeight = 620.0;
constexpr CGFloat margin       = 18.0;
constexpr CGFloat labelWidth   = 110.0;
constexpr CGFloat rowHeight    = 26.0;
constexpr CGFloat rowGap       = 10.0;
constexpr CGFloat midiRow      = 22.0;
constexpr CGFloat pageInset    = 14.0;

/// The Appearance tab's colour list: one row per role, grouped under headings.
constexpr CGFloat swatchRow    = 26.0;
constexpr CGFloat swatchHeader = 24.0;

/// Text fields carry the role they are tinted with in their tag, offset past
/// anything else that uses a tag, so that a palette change can find them again.
/// A colour is a snapshot the moment it is handed to an AppKit control, and
/// this window is full of controls that took one.
constexpr NSInteger tintTagBase = 9000;

/// Block sizes offered. Anything the hardware refuses is corrected by the
/// device on open, and the status line then reports what was actually granted
/// — never what was asked for, because those differ often enough to matter.
constexpr std::int64_t bufferSizes[] = {64, 128, 256, 512, 1024, 2048};

/// Used only when a device declines to report its supported rates.
constexpr double fallbackSampleRates[] = {44100.0, 48000.0, 88200.0, 96000.0};

/// Labels through the shell's design language rather than AppKit's defaults:
/// this window sits beside panes that draw every pixel themselves, and a
/// system-grey label next to them reads as a different application.
/// Remembers the role so that `retintLabels` can reapply it after a theme
/// change. Returns the field for chaining.
NSTextField* tint(NSTextField* field, Ink which)
{
    field.textColor = theme::ink(which);
    field.tag       = tintTagBase + static_cast<NSInteger>(which);
    return field;
}

NSTextField* makeLabel(NSString* text, NSRect frame, BOOL heading)
{
    NSTextField* field = [[NSTextField alloc] initWithFrame:frame];
    field.stringValue   = text;
    field.editable      = NO;
    field.selectable    = NO;
    field.bordered      = NO;
    field.drawsBackground = NO;
    field.font          = heading ? theme::labelFont(11.0, NSFontWeightSemibold)
                                  : theme::labelFont(12.0);
    return tint(field, heading ? Ink::textSecondary : Ink::textPrimary);
}

/// Walks a view tree reapplying every tinted label. Cheaper than rebuilding
/// the window, and it keeps the settings window honest about the theme the
/// rest of the shell just switched to.
void retintLabels(NSView* root)
{
    if (root == nil)
        return;

    if (NSTextField* field = [root isKindOfClass:NSTextField.class] ? static_cast<NSTextField*>(root) : nil;
        field != nil && !field.editable) {
        const NSInteger role = field.tag - tintTagBase;
        if (role >= 0 && role < static_cast<NSInteger>(theme::inkCount))
            field.textColor = theme::ink(static_cast<Ink>(role));
    }

    for (NSView* child in root.subviews)
        retintLabels(child);
}

std::uint32_t argbFromColour(NSColor* colour)
{
    NSColor* converted = [colour colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    if (converted == nil)
        return 0xFF000000u;

    const auto channel = [](CGFloat value) -> std::uint32_t {
        return static_cast<std::uint32_t>(std::lround(std::clamp(value, CGFloat{0.0}, CGFloat{1.0}) * 255.0));
    };

    return (channel(converted.alphaComponent) << 24) | (channel(converted.redComponent) << 16)
           | (channel(converted.greenComponent) << 8) | channel(converted.blueComponent);
}

NSColor* colourFromArgbValue(std::uint32_t argb)
{
    return [NSColor colorWithSRGBRed:static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0
                               green:static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0
                                blue:static_cast<CGFloat>(argb & 0xFFu) / 255.0
                               alpha:static_cast<CGFloat>((argb >> 24) & 0xFFu) / 255.0];
}

NSString* fromUtf8(const std::string& text) { return @(text.c_str()); }

} // namespace

@implementation INCDAWSettingsWindow {
    app::AppSettings* _settings;

    NSWindow*      _window;
    NSPopUpButton* _outputDevice;
    NSPopUpButton* _inputDevice;
    NSPopUpButton* _sampleRate;
    NSPopUpButton* _bufferSize;
    NSButton*      _openInputAtLaunch;
    NSButton*      _checkForUpdates;
    NSButton*      _allMidiSources;
    NSView*        _midiList;
    NSTextField*   _status;

    NSMutableArray<NSButton*>* _midiChecks;

    NSPopUpButton*                _themePicker;
    NSButton*                     _deleteTheme;
    NSView*                       _colourList;
    NSTextField*                  _themeNote;
    NSMutableArray<NSColorWell*>* _colourWells;
    NSMutableArray<NSTextField*>* _hexFields;

    std::vector<platform::AudioDeviceInfo> _devices;
    std::vector<platform::MidiDeviceInfo>  _midiInputs;

    std::filesystem::path _themesDirectory;

    /// The scheme being edited. Held rather than re-read from the palette so
    /// that its name survives a switch to a theme that fails to load.
    theme::ThemePalette _editing;
}

- (instancetype)initWithSettings:(app::AppSettings*)settings
                 themesDirectory:(NSString*)themesDirectory
{
    self = [super init];
    if (self == nil)
        return nil;

    _settings    = settings;
    _midiChecks  = [NSMutableArray array];
    _colourWells = [NSMutableArray array];
    _hexFields   = [NSMutableArray array];

    if (themesDirectory.length > 0)
        _themesDirectory = std::filesystem::path(themesDirectory.UTF8String);

    _editing = [self library].resolve(settings->appearance.themeName);
    return self;
}

- (theme::ThemeLibrary)library
{
    return theme::ThemeLibrary(_themesDirectory);
}

// ── Window ───────────────────────────────────────────────────────────────────

- (void)show
{
    if (_window == nil)
        [self buildWindow];

    [self reloadDevices];

    // The Themes folder is the user's, and it can change while INCDAW is not
    // looking — a file dropped in from another machine, one deleted in Finder.
    // Both menus are therefore rebuilt from the folder every time the window
    // opens rather than once at construction.
    [self rebuildThemePicker];
    [self rebuildColourList];

    // Two of the roles are hairlines that exist only as a partial alpha, so
    // the picker must be able to express one.
    NSColorPanel.sharedColorPanel.showsAlpha = YES;

    [self refreshStatus];

    [_window center];
    [_window makeKeyAndOrderFront:nil];
}

- (void)buildWindow
{
    _window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, windowWidth, windowHeight)
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable
                            | NSWindowStyleMaskMiniaturizable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    // ARC owns this object; letting AppKit release it on close would leave the
    // shell holding a dangling window it intends to reopen.
    _window.releasedWhenClosed = NO;
    _window.title              = @"Settings";
    _window.delegate           = self;
    _window.backgroundColor    = theme::ink(Ink::windowBackground);

    NSView* content = _window.contentView;

    // Four tabs rather than one column. The window held audio, MIDI and
    // updates in a single scroll and was already full when it did; Appearance
    // adds thirty-seven colours, and thirty-seven colours below a device list
    // is a settings window nobody finds the bottom of.
    const CGFloat buttonBand = margin + rowHeight + rowGap;

    NSTabView* tabs = [[NSTabView alloc]
        initWithFrame:NSMakeRect(10, buttonBand, windowWidth - 20, windowHeight - buttonBand - 10)];
    [content addSubview:tabs];

    const NSSize page = tabs.contentRect.size;

    [tabs addTabViewItem:[self tabNamed:@"Audio" view:[self buildAudioPage:page]]];
    [tabs addTabViewItem:[self tabNamed:@"MIDI" view:[self buildMidiPage:page]]];
    [tabs addTabViewItem:[self tabNamed:@"Appearance" view:[self buildAppearancePage:page]]];
    [tabs addTabViewItem:[self tabNamed:@"Updates" view:[self buildUpdatesPage:page]]];

    NSButton* refresh = [[NSButton alloc] initWithFrame:NSMakeRect(margin, margin, 130, rowHeight)];
    refresh.title      = @"Rescan Devices";
    refresh.bezelStyle = NSBezelStyleRounded;
    refresh.target     = self;
    refresh.action     = @selector(rescan:);
    [content addSubview:refresh];

    NSButton* apply = [[NSButton alloc]
        initWithFrame:NSMakeRect(windowWidth - margin - 100, margin, 100, rowHeight)];
    apply.title         = @"Apply";
    apply.bezelStyle    = NSBezelStyleRounded;
    apply.keyEquivalent = @"\r";
    apply.target        = self;
    apply.action        = @selector(apply:);
    [content addSubview:apply];

    NSButton* close = [[NSButton alloc]
        initWithFrame:NSMakeRect(windowWidth - margin - 210, margin, 100, rowHeight)];
    close.title         = @"Close";
    close.bezelStyle    = NSBezelStyleRounded;
    close.keyEquivalent = @"\033";
    close.target        = self;
    close.action        = @selector(closeWindow:);
    [content addSubview:close];
}

- (NSTabViewItem*)tabNamed:(NSString*)title view:(NSView*)view
{
    NSTabViewItem* item = [[NSTabViewItem alloc] initWithIdentifier:title];
    item.label = title;
    item.view  = view;
    return item;
}

// ── Audio ────────────────────────────────────────────────────────────────────

- (NSView*)buildAudioPage:(NSSize)size
{
    NSView* page = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];

    CGFloat y = size.height - rowHeight - 6.0;

    [page addSubview:makeLabel(@"DEVICE", NSMakeRect(pageInset, y, 200, rowHeight), YES)];
    y -= rowHeight;

    const CGFloat wide = size.width - pageInset * 2 - labelWidth;

    _outputDevice = [self addRow:@"Output" toContent:page atY:&y width:wide];
    _outputDevice.target = self;
    _outputDevice.action = @selector(outputDeviceChanged:);

    _inputDevice = [self addRow:@"Input" toContent:page atY:&y width:wide];

    _sampleRate = [self addRow:@"Sample rate" toContent:page atY:&y width:180.0];
    _sampleRate.target = self;
    _sampleRate.action = @selector(sampleRateChanged:);

    _bufferSize = [self addRow:@"Buffer size" toContent:page atY:&y width:180.0];

    _openInputAtLaunch = [[NSButton alloc]
        initWithFrame:NSMakeRect(pageInset + labelWidth, y, 320, rowHeight)];
    _openInputAtLaunch.buttonType = NSButtonTypeSwitch;
    _openInputAtLaunch.title      = @"Open the input device at launch";
    [page addSubview:_openInputAtLaunch];
    y -= rowHeight + rowGap;

    _status = makeLabel(@"", NSMakeRect(pageInset, y - rowHeight * 2,
                                        size.width - pageInset * 2, rowHeight * 3), NO);
    _status.font                 = theme::numericFont(11.0, NSFontWeightRegular);
    _status.maximumNumberOfLines = 3;
    tint(_status, Ink::textDim);
    [page addSubview:_status];

    return page;
}

// ── MIDI ─────────────────────────────────────────────────────────────────────

- (NSView*)buildMidiPage:(NSSize)size
{
    NSView* page = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];

    CGFloat y = size.height - rowHeight - 6.0;

    [page addSubview:makeLabel(@"MIDI INPUT", NSMakeRect(pageInset, y, 200, rowHeight), YES)];
    y -= rowHeight;

    _allMidiSources = [[NSButton alloc] initWithFrame:NSMakeRect(pageInset, y, 320, rowHeight)];
    _allMidiSources.buttonType = NSButtonTypeSwitch;
    _allMidiSources.title      = @"Connect every available source";
    _allMidiSources.target     = self;
    _allMidiSources.action     = @selector(allSourcesToggled:);
    [page addSubview:_allMidiSources];
    y -= rowHeight + 4.0;

    NSScrollView* scroll = [[NSScrollView alloc]
        initWithFrame:NSMakeRect(pageInset, pageInset,
                                 size.width - pageInset * 2, y - pageInset)];
    scroll.hasVerticalScroller = YES;
    scroll.borderType          = NSBezelBorder;
    scroll.drawsBackground     = NO;

    _midiList = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, scroll.contentSize.width, 0)];
    scroll.documentView = _midiList;
    [page addSubview:scroll];

    return page;
}

// ── Updates ──────────────────────────────────────────────────────────────────

- (NSView*)buildUpdatesPage:(NSSize)size
{
    NSView* page = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];

    CGFloat y = size.height - rowHeight - 6.0;

    [page addSubview:makeLabel(@"UPDATES", NSMakeRect(pageInset, y, 200, rowHeight), YES)];
    y -= rowHeight;

    _checkForUpdates = [[NSButton alloc]
        initWithFrame:NSMakeRect(pageInset, y, size.width - pageInset * 2, rowHeight)];
    _checkForUpdates.buttonType = NSButtonTypeSwitch;
    _checkForUpdates.title      = @"Check for a newer version at launch";
    [page addSubview:_checkForUpdates];
    y -= rowHeight;

    // Worded so that what leaves the machine is stated rather than implied: a
    // launch-time request is a privacy decision, and one made silently is one
    // made badly.
    NSTextField* caption = makeLabel(@"Reads INCDAW's public release page, at most once a day. "
                                     @"No account, nothing uploaded, nothing installed.",
                                     NSMakeRect(pageInset, y - rowHeight,
                                                size.width - pageInset * 2, rowHeight * 2),
                                     NO);
    caption.font                 = theme::labelFont(11.0);
    caption.maximumNumberOfLines = 2;
    tint(caption, Ink::textDim);
    [page addSubview:caption];

    return page;
}

// ── Appearance ───────────────────────────────────────────────────────────────
//
// A theme is a file, and this tab is a folder browser with a colour picker
// attached (ui/ThemeLibrary.h). The built-in schemes are read-only: the first
// edit to one copies it to a theme of the user's own and continues there, so
// that the ground INCDAW ships with is always one selection away.
//
// Every change is live. There is no preview strip because the application is
// the preview — a colour moved here is drawn by every open pane before the
// picker is released, which is the only honest way to judge a palette that has
// to survive a running transport.

- (NSView*)buildAppearancePage:(NSSize)size
{
    NSView* page = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, size.width, size.height)];

    CGFloat y = size.height - rowHeight - 6.0;

    [page addSubview:makeLabel(@"THEME", NSMakeRect(pageInset, y, 200, rowHeight), YES)];
    y -= rowHeight;

    [page addSubview:makeLabel(@"Theme", NSMakeRect(pageInset, y, labelWidth, rowHeight), NO)];

    const CGFloat pickerX = pageInset + labelWidth;
    const CGFloat pickerW = size.width - pickerX - pageInset - 184.0;

    _themePicker = [[NSPopUpButton alloc]
        initWithFrame:NSMakeRect(pickerX, y, std::max<CGFloat>(pickerW, 120.0), rowHeight)
            pullsDown:NO];
    _themePicker.target = self;
    _themePicker.action = @selector(themeChanged:);
    [page addSubview:_themePicker];

    NSButton* duplicate = [[NSButton alloc]
        initWithFrame:NSMakeRect(NSMaxX(_themePicker.frame) + 8.0, y, 92, rowHeight)];
    duplicate.title      = @"Duplicate";
    duplicate.bezelStyle = NSBezelStyleRounded;
    duplicate.target     = self;
    duplicate.action     = @selector(duplicateTheme:);
    [page addSubview:duplicate];

    _deleteTheme = [[NSButton alloc]
        initWithFrame:NSMakeRect(NSMaxX(duplicate.frame) + 6.0, y, 78, rowHeight)];
    _deleteTheme.title      = @"Delete";
    _deleteTheme.bezelStyle = NSBezelStyleRounded;
    _deleteTheme.target     = self;
    _deleteTheme.action     = @selector(deleteTheme:);
    [page addSubview:_deleteTheme];
    y -= rowHeight;

    _themeNote = makeLabel(@"", NSMakeRect(pageInset, y - rowHeight,
                                           size.width - pageInset * 2, rowHeight * 2), NO);
    _themeNote.font                 = theme::labelFont(11.0);
    _themeNote.maximumNumberOfLines = 2;
    tint(_themeNote, Ink::textDim);
    [page addSubview:_themeNote];
    y -= rowHeight * 2 + 4.0;

    const CGFloat footer = pageInset + rowHeight + 8.0;

    NSScrollView* scroll = [[NSScrollView alloc]
        initWithFrame:NSMakeRect(pageInset, footer,
                                 size.width - pageInset * 2, std::max<CGFloat>(y - footer, 80.0))];
    scroll.hasVerticalScroller = YES;
    scroll.borderType          = NSBezelBorder;
    scroll.drawsBackground     = NO;

    _colourList = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, scroll.contentSize.width, 0)];
    scroll.documentView = _colourList;
    [page addSubview:scroll];

    NSButton* reveal = [[NSButton alloc] initWithFrame:NSMakeRect(pageInset, pageInset, 170, rowHeight)];
    reveal.title      = @"Reveal Themes Folder";
    reveal.bezelStyle = NSBezelStyleRounded;
    reveal.target     = self;
    reveal.action     = @selector(revealThemes:);
    [page addSubview:reveal];

    NSButton* reset = [[NSButton alloc]
        initWithFrame:NSMakeRect(size.width - pageInset - 150, pageInset, 150, rowHeight)];
    reset.title      = @"Reset This Theme";
    reset.bezelStyle = NSBezelStyleRounded;
    reset.target     = self;
    reset.action     = @selector(resetColours:);
    [page addSubview:reset];

    return page;
}

/// Refills the theme menu from the folder and reselects what is being edited.
- (void)rebuildThemePicker
{
    [_themePicker removeAllItems];

    const theme::ThemeLibrary library = [self library];
    NSMenuItem*               chosen  = nil;

    bool separated = false;
    for (const theme::ThemeLibrary::Entry& entry : library.entries()) {
        if (!entry.builtin && !separated) {
            [_themePicker.menu addItem:NSMenuItem.separatorItem];
            separated = true;
        }

        [_themePicker addItemWithTitle:fromUtf8(entry.name)];
        _themePicker.lastItem.representedObject = fromUtf8(entry.name);

        if (theme::namesMatch(entry.name, _editing.name))
            chosen = _themePicker.lastItem;
    }

    if (chosen != nil)
        [_themePicker selectItem:chosen];
    else if (_themePicker.numberOfItems > 0)
        [_themePicker selectItemAtIndex:0];

    const bool builtin = theme::isBuiltinName(_editing.name);
    _deleteTheme.enabled = !builtin && !_themesDirectory.empty();

    if (_themesDirectory.empty())
        _themeNote.stringValue = @"No writable support folder was found, so themes cannot be "
                                  "saved here. The built-in schemes still apply.";
    else if (builtin)
        _themeNote.stringValue = @"Built-in themes are read-only. Editing a colour makes a copy "
                                  "you own and continues there.";
    else
        _themeNote.stringValue = [NSString stringWithFormat:@"Saved as %s.json — a file you can "
                                                             "copy, edit or send to somebody else.",
                                                            _editing.name.c_str()];
}

/// One row per role, grouped under the headings the palette declares.
- (void)rebuildColourList
{
    for (NSView* row in [_colourList.subviews copy])
        [row removeFromSuperview];

    [_colourWells removeAllObjects];
    [_hexFields removeAllObjects];

    const CGFloat width = _colourList.superview != nil ? _colourList.superview.frame.size.width : 400.0;

    // Measured before anything is placed, because the list is laid out from
    // its own top edge downwards and needs to know where that is.
    std::string previousGroup;
    CGFloat     height = 8.0;
    for (const Ink which : theme::allInks()) {
        if (const std::string group = theme::inkGroup(which); group != previousGroup) {
            previousGroup = group;
            height += swatchHeader;
        }

        height += swatchRow;
    }

    _colourList.frame = NSMakeRect(0, 0, width, height);

    const CGFloat swatchX = std::min<CGFloat>(196.0, width * 0.45);

    previousGroup.clear();
    CGFloat y = height - 4.0;

    for (const Ink which : theme::allInks()) {
        if (const std::string group = theme::inkGroup(which); group != previousGroup) {
            previousGroup = group;
            y -= swatchHeader;

            NSTextField* heading = makeLabel(fromUtf8(group),
                                             NSMakeRect(8, y, width - 16, swatchHeader - 4.0), YES);
            [_colourList addSubview:heading];
        }

        y -= swatchRow;

        NSTextField* label = makeLabel(fromUtf8(theme::inkLabel(which)),
                                       NSMakeRect(8, y + 3.0, swatchX - 16.0, rowHeight - 6.0), NO);
        label.font = theme::labelFont(11.5);
        [_colourList addSubview:label];

        NSColorWell* well = [[NSColorWell alloc]
            initWithFrame:NSMakeRect(swatchX, y + 2.0, 56, swatchRow - 6.0)];
        well.tag    = static_cast<NSInteger>(which);
        well.target = self;
        well.action = @selector(colourWellChanged:);
        [_colourList addSubview:well];
        [_colourWells addObject:well];

        NSTextField* hex = [[NSTextField alloc]
            initWithFrame:NSMakeRect(swatchX + 64.0, y + 2.0, 96, swatchRow - 6.0)];
        hex.font                   = theme::numericFont(11.0, NSFontWeightRegular);
        hex.alignment              = NSTextAlignmentCenter;
        hex.tag                    = static_cast<NSInteger>(which);
        hex.target                 = self;
        hex.action                 = @selector(hexFieldChanged:);
        hex.placeholderString      = @"#AARRGGBB";
        [_colourList addSubview:hex];
        [_hexFields addObject:hex];
    }

    [self refreshSwatches];
}

/// Pushes `_editing` into the wells and the hex fields, without touching the
/// palette. Called after a theme switch, a reset, or a hand-typed colour.
- (void)refreshSwatches
{
    for (NSColorWell* well in _colourWells)
        well.color = colourFromArgbValue(_editing.colours[static_cast<std::size_t>(well.tag)]);

    for (NSTextField* hex in _hexFields)
        hex.stringValue = fromUtf8(theme::toHex(_editing.colours[static_cast<std::size_t>(hex.tag)]));
}

// ── Appearance actions ───────────────────────────────────────────────────────

/// Makes `_editing` a theme that may be written to, copying a built-in first.
/// Returns false only when there is nowhere to write, in which case the change
/// still applies for the session but is not saved.
- (BOOL)ensureEditableTheme
{
    if (!theme::isBuiltinName(_editing.name))
        return YES;

    if (_themesDirectory.empty())
        return NO;

    const std::string preferred = _editing.name + " Custom";

    std::string       error;
    const std::string created = [self library].duplicate(_editing, preferred, error);
    if (created.empty()) {
        NSLog(@"INCDAW: the theme could not be copied: %s", error.c_str());
        return NO;
    }

    _editing.name                     = created;
    _settings->appearance.themeName   = created;

    [self rebuildThemePicker];
    return YES;
}

/// Hands `_editing` to the shell and redraws everything that draws with it.
- (void)applyEditedPalette
{
    theme::setPalette(_editing);

    _window.backgroundColor = theme::ink(Ink::windowBackground);
    retintLabels(_window.contentView);
    theme::refreshViewTree(_window.contentView);

    // Coalesced: a colour well sends its action continuously while the picker
    // is dragged, and a theme file written sixty times a second is a file
    // written sixty times a second.
    [NSObject cancelPreviousPerformRequestsWithTarget:self
                                             selector:@selector(persistTheme)
                                               object:nil];
    [self performSelector:@selector(persistTheme) withObject:nil afterDelay:0.4];
}

- (void)persistTheme
{
    if (!theme::isBuiltinName(_editing.name) && !_themesDirectory.empty()) {
        std::string error;
        if (![self library].store(_editing, error))
            NSLog(@"INCDAW: the theme could not be saved: %s", error.c_str());
    }

    if (self.onAppearanceChanged != nil)
        self.onAppearanceChanged();
}

- (void)themeChanged:(id)sender
{
    (void)sender;

    NSString* chosen = _themePicker.selectedItem.representedObject;
    if (chosen.length == 0)
        return;

    _editing                        = [self library].resolve(chosen.UTF8String);
    _settings->appearance.themeName = _editing.name;

    [self rebuildThemePicker];
    [self refreshSwatches];
    [self applyEditedPalette];
}

- (void)duplicateTheme:(id)sender
{
    (void)sender;

    if (_themesDirectory.empty()) {
        NSBeep();
        return;
    }

    std::string       error;
    const std::string created = [self library].duplicate(_editing, _editing.name + " Copy", error);
    if (created.empty()) {
        NSLog(@"INCDAW: the theme could not be copied: %s", error.c_str());
        NSBeep();
        return;
    }

    _editing.name                   = created;
    _settings->appearance.themeName = created;

    [self rebuildThemePicker];
    [self persistTheme];
}

- (void)deleteTheme:(id)sender
{
    (void)sender;

    if (theme::isBuiltinName(_editing.name))
        return;

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = [NSString stringWithFormat:@"Delete the theme “%s”?", _editing.name.c_str()];
    alert.informativeText = @"Its file is removed from the Themes folder. This cannot be undone.";
    [alert addButtonWithTitle:@"Delete"];
    [alert addButtonWithTitle:@"Cancel"];
    alert.alertStyle = NSAlertStyleWarning;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    std::string error;
    if (![self library].remove(_editing.name, error)) {
        NSLog(@"INCDAW: the theme could not be deleted: %s", error.c_str());
        return;
    }

    _editing                        = theme::defaultPalette();
    _settings->appearance.themeName = _editing.name;

    [self rebuildThemePicker];
    [self refreshSwatches];
    [self applyEditedPalette];
}

- (void)colourWellChanged:(id)sender
{
    NSColorWell* well = sender;
    if (well == nil)
        return;

    if (![self ensureEditableTheme]) {
        // Nowhere to save it. The change still applies, because refusing to
        // draw what the user just chose would be stranger than not keeping it.
        NSLog(@"INCDAW: the theme change applies for this session only");
    }

    const auto which = static_cast<Ink>(well.tag);
    _editing.setColour(which, argbFromColour(well.color));

    for (NSTextField* hex in _hexFields)
        if (hex.tag == well.tag)
            hex.stringValue = fromUtf8(theme::toHex(_editing.colour(which)));

    [self applyEditedPalette];
}

- (void)hexFieldChanged:(id)sender
{
    NSTextField* hex = sender;
    if (hex == nil)
        return;

    const auto which = static_cast<Ink>(hex.tag);

    std::uint32_t value = 0;
    if (!theme::fromHex(hex.stringValue.UTF8String, value)) {
        // A typo puts the field back to what is actually being drawn, rather
        // than leaving a colour on screen that the text disagrees with.
        hex.stringValue = fromUtf8(theme::toHex(_editing.colour(which)));
        NSBeep();
        return;
    }

    (void)[self ensureEditableTheme];

    _editing.setColour(which, value);
    hex.stringValue = fromUtf8(theme::toHex(value));

    for (NSColorWell* well in _colourWells)
        if (well.tag == hex.tag)
            well.color = colourFromArgbValue(value);

    [self applyEditedPalette];
}

- (void)resetColours:(id)sender
{
    (void)sender;

    // Back to the scheme this one came from where that is knowable, and to the
    // default otherwise. The name is kept: resetting a theme is not deleting it.
    const std::string kept = _editing.name;

    theme::ThemePalette source = theme::defaultPalette();
    for (std::size_t slot = 0; slot < theme::builtinCount(); ++slot) {
        const std::string builtin = theme::builtinName(slot);
        if (kept.rfind(builtin, 0) == 0) {
            source = theme::builtinPalette(slot);
            break;
        }
    }

    _editing.colours = source.colours;
    _editing.name    = kept;

    [self refreshSwatches];
    [self applyEditedPalette];
}

- (void)revealThemes:(id)sender
{
    (void)sender;

    if (_themesDirectory.empty()) {
        NSBeep();
        return;
    }

    std::error_code failed;
    std::filesystem::create_directories(_themesDirectory, failed);

    NSURL* url = [NSURL fileURLWithPath:@(_themesDirectory.c_str()) isDirectory:YES];
    [NSWorkspace.sharedWorkspace activateFileViewerSelectingURLs:@[url]];
}

/// Adds a labelled pop-up row and moves `y` down past it.
- (NSPopUpButton*)addRow:(NSString*)title
               toContent:(NSView*)content
                     atY:(CGFloat*)y
                   width:(CGFloat)width
{
    [content addSubview:makeLabel(title, NSMakeRect(pageInset, *y, labelWidth, rowHeight), NO)];

    NSPopUpButton* popup = [[NSPopUpButton alloc]
        initWithFrame:NSMakeRect(pageInset + labelWidth, *y, width, rowHeight)
            pullsDown:NO];
    [content addSubview:popup];

    *y -= rowHeight + rowGap;
    return popup;
}

// ── Devices ──────────────────────────────────────────────────────────────────

- (void)reloadDevices
{
    if (const std::unique_ptr<platform::AudioDevice> device = platform::AudioDevice::create())
        _devices = device->enumerateDevices();
    else
        _devices.clear();

    if (const std::unique_ptr<platform::MidiDevice> midi = platform::MidiDevice::create())
        _midiInputs = midi->enumerateInputs();
    else
        _midiInputs.clear();

    [self rebuildDevicePopups];
    [self rebuildRatePopups];
    [self rebuildMidiList];

    _openInputAtLaunch.state = _settings->openInputAtLaunch ? NSControlStateValueOn
                                                            : NSControlStateValueOff;

    _checkForUpdates.state = _settings->updates.checkAtLaunch ? NSControlStateValueOn
                                                              : NSControlStateValueOff;
}

- (void)rebuildDevicePopups
{
    [_outputDevice removeAllItems];
    [_outputDevice addItemWithTitle:@"System Default"];
    _outputDevice.itemArray.firstObject.representedObject = @"";

    for (const platform::AudioDeviceInfo& info : _devices) {
        if (info.outputChannels == 0)
            continue;

        [_outputDevice addItemWithTitle:fromUtf8(info.name)];
        _outputDevice.lastItem.representedObject = fromUtf8(info.identifier);
    }

    [self selectIdentifier:fromUtf8(_settings->audio.outputDeviceIdentifier)
                   inPopup:_outputDevice
              missingTitle:fromUtf8(_settings->audio.outputDeviceIdentifier)];

    [_inputDevice removeAllItems];

    // "None" is first and is the default: recording is opt-in, and opening a
    // microphone that nobody asked for is a permission prompt, not a feature.
    [_inputDevice addItemWithTitle:@"None"];
    _inputDevice.itemArray.firstObject.representedObject = @"";

    [_inputDevice addItemWithTitle:@"System Default"];
    _inputDevice.lastItem.representedObject = @(platform::AudioDeviceConfig::defaultInput);

    for (const platform::AudioDeviceInfo& info : _devices) {
        if (info.inputChannels == 0)
            continue;

        [_inputDevice addItemWithTitle:fromUtf8(info.name)];
        _inputDevice.lastItem.representedObject = fromUtf8(info.identifier);
    }

    [self selectIdentifier:fromUtf8(_settings->audio.inputDeviceIdentifier)
                   inPopup:_inputDevice
              missingTitle:fromUtf8(_settings->audio.inputDeviceIdentifier)];
}

/// Selects the item carrying `identifier`, adding a placeholder for a device
/// that is not currently present.
///
/// A saved interface that is unplugged must stay selected rather than silently
/// becoming the built-in speakers: plugging it back in should restore the
/// session, and a preference that quietly rewrites itself never does.
- (void)selectIdentifier:(NSString*)identifier
                 inPopup:(NSPopUpButton*)popup
            missingTitle:(NSString*)missingTitle
{
    for (NSMenuItem* item in popup.itemArray) {
        NSString* represented = item.representedObject;
        if (represented != nil && [represented isEqualToString:identifier]) {
            [popup selectItem:item];
            return;
        }
    }

    if (identifier.length == 0) {
        [popup selectItemAtIndex:0];
        return;
    }

    [popup addItemWithTitle:[NSString stringWithFormat:@"%@ (not connected)", missingTitle]];
    popup.lastItem.representedObject = identifier;
    [popup selectItem:popup.lastItem];
}

- (NSString*)selectedIdentifierOf:(NSPopUpButton*)popup
{
    NSString* represented = popup.selectedItem.representedObject;
    return represented != nil ? represented : @"";
}

- (void)outputDeviceChanged:(id)sender
{
    (void)sender;
    [self rebuildRatePopups];
}

- (void)sampleRateChanged:(id)sender
{
    (void)sender;
    [self rebuildBufferPopup];
}

/// The rates offered follow the selected output device, because a rate the
/// device does not support is a failed open, not a preference.
- (void)rebuildRatePopups
{
    NSString* identifier = [self selectedIdentifierOf:_outputDevice];

    std::vector<double> rates;
    for (const platform::AudioDeviceInfo& info : _devices) {
        const bool matches = identifier.length == 0
                                 ? info.isDefaultOutput
                                 : [fromUtf8(info.identifier) isEqualToString:identifier];
        if (matches && info.outputChannels > 0)
            rates = info.sampleRates;
    }

    if (rates.empty())
        rates.assign(std::begin(fallbackSampleRates), std::end(fallbackSampleRates));

    std::sort(rates.begin(), rates.end());
    rates.erase(std::unique(rates.begin(), rates.end()), rates.end());

    const double current =
        _sampleRate.selectedItem != nil
            ? static_cast<NSNumber*>(_sampleRate.selectedItem.representedObject).doubleValue
            : _settings->audio.sampleRate;

    [_sampleRate removeAllItems];
    for (const double rate : rates) {
        [_sampleRate addItemWithTitle:[NSString stringWithFormat:@"%.0f Hz", rate]];
        _sampleRate.lastItem.representedObject = @(rate);
    }

    if (![self selectNumber:current inPopup:_sampleRate]) {
        // The saved rate is not on this device's list. Offer it anyway: the
        // device may be back later, and a silent downgrade is worse than an
        // open that fails with a reason.
        [_sampleRate addItemWithTitle:[NSString stringWithFormat:@"%.0f Hz (unsupported)", current]];
        _sampleRate.lastItem.representedObject = @(current);
        [_sampleRate selectItem:_sampleRate.lastItem];
    }

    [self rebuildBufferPopup];
}

- (void)rebuildBufferPopup
{
    const double rate =
        _sampleRate.selectedItem != nil
            ? static_cast<NSNumber*>(_sampleRate.selectedItem.representedObject).doubleValue
            : _settings->audio.sampleRate;

    const std::int64_t current =
        _bufferSize.selectedItem != nil
            ? static_cast<NSNumber*>(_bufferSize.selectedItem.representedObject).longLongValue
            : _settings->audio.bufferSize;

    [_bufferSize removeAllItems];
    for (const std::int64_t frames : bufferSizes) {
        // One block of latency, spelled out. It is the number the choice is
        // actually about, and leaving the user to divide by the sample rate is
        // how block size ends up looking like an arbitrary number.
        const double millis = rate > 0.0 ? (static_cast<double>(frames) / rate) * 1000.0 : 0.0;

        [_bufferSize addItemWithTitle:[NSString stringWithFormat:@"%lld  (%.1f ms)",
                                                                 static_cast<long long>(frames), millis]];
        _bufferSize.lastItem.representedObject = @(frames);
    }

    if (![self selectNumber:static_cast<double>(current) inPopup:_bufferSize]) {
        [_bufferSize addItemWithTitle:[NSString stringWithFormat:@"%lld", static_cast<long long>(current)]];
        _bufferSize.lastItem.representedObject = @(current);
        [_bufferSize selectItem:_bufferSize.lastItem];
    }
}

- (BOOL)selectNumber:(double)value inPopup:(NSPopUpButton*)popup
{
    for (NSMenuItem* item in popup.itemArray) {
        NSNumber* represented = item.representedObject;
        if (represented != nil && std::abs(represented.doubleValue - value) < 0.5) {
            [popup selectItem:item];
            return YES;
        }
    }

    return NO;
}

// ── MIDI ─────────────────────────────────────────────────────────────────────

- (void)rebuildMidiList
{
    for (NSButton* check in _midiChecks)
        [check removeFromSuperview];
    [_midiChecks removeAllObjects];

    const BOOL all = _settings->midiInputIdentifiers.empty();
    _allMidiSources.state = all ? NSControlStateValueOn : NSControlStateValueOff;

    const CGFloat width  = _midiList.superview != nil ? _midiList.superview.frame.size.width : 400.0;
    const CGFloat height = std::max<CGFloat>(static_cast<CGFloat>(_midiInputs.size()) * midiRow + 8.0, 40.0);

    _midiList.frame = NSMakeRect(0, 0, width, height);

    CGFloat y = height - midiRow - 4.0;

    for (const platform::MidiDeviceInfo& info : _midiInputs) {
        NSButton* check = [[NSButton alloc] initWithFrame:NSMakeRect(8, y, width - 16, midiRow)];
        check.buttonType = NSButtonTypeSwitch;
        check.title      = fromUtf8(info.name);
        check.enabled    = !all;

        const bool connected =
            std::find(_settings->midiInputIdentifiers.begin(),
                      _settings->midiInputIdentifiers.end(),
                      info.identifier) != _settings->midiInputIdentifiers.end();

        check.state = (all || connected) ? NSControlStateValueOn : NSControlStateValueOff;
        check.identifier = fromUtf8(info.identifier);

        [_midiList addSubview:check];
        [_midiChecks addObject:check];

        y -= midiRow;
    }

    if (_midiInputs.empty()) {
        NSTextField* empty = makeLabel(@"No MIDI sources found.",
                                       NSMakeRect(8, height - midiRow - 4.0, width - 16, midiRow), NO);
        empty.textColor = theme::ink(Ink::textDim);
        [_midiList addSubview:empty];
    }
}

- (void)allSourcesToggled:(id)sender
{
    (void)sender;

    const BOOL all = _allMidiSources.state == NSControlStateValueOn;
    for (NSButton* check in _midiChecks) {
        check.enabled = !all;
        if (all)
            check.state = NSControlStateValueOn;
    }
}

// ── Actions ──────────────────────────────────────────────────────────────────

- (void)rescan:(id)sender
{
    (void)sender;
    [self reloadDevices];
}

- (void)apply:(id)sender
{
    (void)sender;

    _settings->audio.outputDeviceIdentifier = [self selectedIdentifierOf:_outputDevice].UTF8String;
    _settings->audio.inputDeviceIdentifier  = [self selectedIdentifierOf:_inputDevice].UTF8String;

    if (_sampleRate.selectedItem != nil)
        _settings->audio.sampleRate =
            static_cast<NSNumber*>(_sampleRate.selectedItem.representedObject).doubleValue;

    if (_bufferSize.selectedItem != nil)
        _settings->audio.bufferSize =
            static_cast<NSNumber*>(_bufferSize.selectedItem.representedObject).longLongValue;

    _settings->openInputAtLaunch = _openInputAtLaunch.state == NSControlStateValueOn;

    // Turning the check back on clears the skip with it: a user who asks to be
    // told about new versions has stopped declining the one they last passed on.
    const bool checkForUpdates = _checkForUpdates.state == NSControlStateValueOn;
    if (checkForUpdates && !_settings->updates.checkAtLaunch)
        _settings->updates.skippedVersion.clear();

    _settings->updates.checkAtLaunch = checkForUpdates;

    // An empty list means "every source" — the same convention the platform
    // layer already uses, so an all-checked list is stored as no list at all
    // and keeps working when the hardware changes.
    _settings->midiInputIdentifiers.clear();
    if (_allMidiSources.state != NSControlStateValueOn) {
        for (NSButton* check in _midiChecks) {
            if (check.state == NSControlStateValueOn && check.identifier.length > 0)
                _settings->midiInputIdentifiers.push_back(check.identifier.UTF8String);
        }
    }

    if (self.onApply != nil)
        self.onApply();

    [self refreshStatus];
}

- (void)closeWindow:(id)sender
{
    (void)sender;
    [_window orderOut:nil];
}

- (void)refreshStatus
{
    if (_status == nil)
        return;

    _status.stringValue = self.statusProvider != nil ? self.statusProvider() : @"";
}

@end
