#include "ui/macos/SettingsWindow.h"

#import "ui/macos/Theme.h"

#include "app/AppSettings.h"
#include "platform/AudioDevice.h"
#include "platform/MidiDevice.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
namespace theme = incdaw::ui::theme;
using incdaw::ui::theme::Ink;

namespace {

constexpr CGFloat windowWidth  = 540.0;
constexpr CGFloat windowHeight = 648.0;
constexpr CGFloat margin       = 18.0;
constexpr CGFloat labelWidth   = 110.0;
constexpr CGFloat rowHeight    = 26.0;
constexpr CGFloat rowGap       = 10.0;
constexpr CGFloat midiRow      = 22.0;

/// The UPDATES block at the foot of the window: a heading, the switch, and the
/// line that says what the switch actually does. Reserved before the MIDI list
/// is sized, because the list is what takes whatever is left over.
constexpr CGFloat updatesBand  = 26.0 * 3.0 + 10.0;

/// Block sizes offered. Anything the hardware refuses is corrected by the
/// device on open, and the status line then reports what was actually granted
/// — never what was asked for, because those differ often enough to matter.
constexpr std::int64_t bufferSizes[] = {64, 128, 256, 512, 1024, 2048};

/// Used only when a device declines to report its supported rates.
constexpr double fallbackSampleRates[] = {44100.0, 48000.0, 88200.0, 96000.0};

/// Labels through the shell's design language rather than AppKit's defaults:
/// this window sits beside panes that draw every pixel themselves, and a
/// system-grey label next to them reads as a different application.
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
    field.textColor     = theme::ink(heading ? Ink::textSecondary : Ink::textPrimary);
    return field;
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

    std::vector<platform::AudioDeviceInfo> _devices;
    std::vector<platform::MidiDeviceInfo>  _midiInputs;
}

- (instancetype)initWithSettings:(app::AppSettings*)settings
{
    self = [super init];
    if (self == nil)
        return nil;

    _settings   = settings;
    _midiChecks = [NSMutableArray array];
    return self;
}

// ── Window ───────────────────────────────────────────────────────────────────

- (void)show
{
    if (_window == nil)
        [self buildWindow];

    [self reloadDevices];
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

    CGFloat y = windowHeight - margin - rowHeight;

    [content addSubview:makeLabel(@"AUDIO", NSMakeRect(margin, y, 200, rowHeight), YES)];
    y -= rowHeight;

    _outputDevice = [self addRow:@"Output" toContent:content atY:&y width:windowWidth - margin * 2 - labelWidth];
    _outputDevice.target = self;
    _outputDevice.action = @selector(outputDeviceChanged:);

    _inputDevice = [self addRow:@"Input" toContent:content atY:&y width:windowWidth - margin * 2 - labelWidth];

    _sampleRate = [self addRow:@"Sample rate" toContent:content atY:&y width:180.0];
    _sampleRate.target = self;
    _sampleRate.action = @selector(sampleRateChanged:);

    _bufferSize = [self addRow:@"Buffer size" toContent:content atY:&y width:180.0];

    _openInputAtLaunch = [[NSButton alloc]
        initWithFrame:NSMakeRect(margin + labelWidth, y, 320, rowHeight)];
    _openInputAtLaunch.buttonType = NSButtonTypeSwitch;
    _openInputAtLaunch.title      = @"Open the input device at launch";
    [content addSubview:_openInputAtLaunch];
    y -= rowHeight + rowGap;

    _status = makeLabel(@"", NSMakeRect(margin, y - rowHeight, windowWidth - margin * 2, rowHeight * 2), NO);
    _status.textColor = theme::ink(Ink::textDim);
    _status.font      = theme::numericFont(11.0, NSFontWeightRegular);
    _status.maximumNumberOfLines = 2;
    [content addSubview:_status];
    y -= rowHeight * 2 + rowGap;

    [content addSubview:makeLabel(@"MIDI INPUT", NSMakeRect(margin, y, 200, rowHeight), YES)];
    y -= rowHeight;

    _allMidiSources = [[NSButton alloc] initWithFrame:NSMakeRect(margin, y, 320, rowHeight)];
    _allMidiSources.buttonType = NSButtonTypeSwitch;
    _allMidiSources.title      = @"Connect every available source";
    _allMidiSources.target     = self;
    _allMidiSources.action     = @selector(allSourcesToggled:);
    [content addSubview:_allMidiSources];
    y -= rowHeight;

    const CGFloat listBottom = margin + rowHeight + rowGap + updatesBand;
    const CGFloat listHeight  = y - listBottom;

    NSScrollView* scroll = [[NSScrollView alloc]
        initWithFrame:NSMakeRect(margin, listBottom,
                                 windowWidth - margin * 2, std::max<CGFloat>(listHeight, 60.0))];
    scroll.hasVerticalScroller = YES;
    scroll.borderType          = NSBezelBorder;
    scroll.drawsBackground     = NO;

    _midiList = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, scroll.contentSize.width, 0)];
    scroll.documentView = _midiList;
    [content addSubview:scroll];

    // Updates. Placed where a user looks for it and worded so that what leaves
    // the machine is stated rather than implied — a launch-time request is a
    // privacy decision, and one made silently is one made badly.
    CGFloat updatesY = listBottom - rowHeight;
    [content addSubview:makeLabel(@"UPDATES", NSMakeRect(margin, updatesY, 200, rowHeight), YES)];
    updatesY -= rowHeight;

    _checkForUpdates = [[NSButton alloc]
        initWithFrame:NSMakeRect(margin, updatesY, windowWidth - margin * 2, rowHeight)];
    _checkForUpdates.buttonType = NSButtonTypeSwitch;
    _checkForUpdates.title      = @"Check for a newer version at launch";
    [content addSubview:_checkForUpdates];
    updatesY -= rowHeight;

    NSTextField* caption = makeLabel(@"Reads INCDAW's public release page, at most once a day. "
                                     @"No account, nothing uploaded, nothing installed.",
                                     NSMakeRect(margin, updatesY, windowWidth - margin * 2, rowHeight),
                                     NO);
    caption.font      = theme::labelFont(11.0);
    caption.textColor = theme::ink(Ink::textDim);
    [content addSubview:caption];

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

/// Adds a labelled pop-up row and moves `y` down past it.
- (NSPopUpButton*)addRow:(NSString*)title
               toContent:(NSView*)content
                     atY:(CGFloat*)y
                   width:(CGFloat)width
{
    [content addSubview:makeLabel(title, NSMakeRect(margin, *y, labelWidth, rowHeight), NO)];

    NSPopUpButton* popup = [[NSPopUpButton alloc]
        initWithFrame:NSMakeRect(margin + labelWidth, *y, width, rowHeight)
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
