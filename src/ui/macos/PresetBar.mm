#import "ui/macos/PresetBar.h"

#include "ui/macos/Theme.h"

#import <objc/runtime.h>

namespace theme = incdaw::ui::theme;
using incdaw::ui::theme::Ink;

const CGFloat INCDAWPresetBarHeight = 34.0;

namespace {

constexpr CGFloat margin = 10.0;

/// Tags on the action items, so one selector serves the whole menu without
/// comparing titles — a title is a translation away from being wrong.
enum : NSInteger {
    tagPresetBase = 1000,   ///< + index into the entry list
    tagSave       = 1,
    tagRename     = 2,
    tagDuplicate  = 3,
    tagDelete     = 4,
};

/// The window keeps its bar reachable through this association, the same way
/// the parameter panel keeps its own controller.
const void* presetBarKey = &presetBarKey;

constexpr const char* unsavedTitle = "—";

/// A one-field prompt. AppKit has no stock "ask for a name", and a panel that
/// grew its own sheet would be a second one to keep in step.
NSString* askForName(NSString* message, NSString* suggestion, NSWindow* parent)
{
    NSAlert* alert      = [[NSAlert alloc] init];
    alert.messageText   = message;
    alert.alertStyle    = NSAlertStyleInformational;
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 240, 24)];
    field.stringValue  = suggestion != nil ? suggestion : @"";
    alert.accessoryView = field;

    if (parent != nil)
        [alert.window setInitialFirstResponder:field];

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return nil;

    NSString* typed = [field.stringValue
        stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];

    return typed.length > 0 ? typed : nil;
}

} // namespace

@implementation INCDAWPresetBar {
    NSPopUpButton*          _popup;
    NSTextField*            _caption;
    NSArray<NSDictionary*>* _entries;
    NSString*               _selected;
}

+ (INCDAWPresetBar*)attachToWindow:(NSWindow*)window
{
    if (INCDAWPresetBar* existing = objc_getAssociatedObject(window, presetBarKey))
        return existing;

    NSView* content = window.contentView;

    const CGFloat width     = content.frame.size.width;
    const CGFloat oldHeight = content.frame.size.height;
    const CGFloat newHeight = oldHeight + INCDAWPresetBarHeight;

    // The container goes in FIRST, empty. Assigning a content view resizes it
    // to the window's current content size, so the window is grown while the
    // panel's own view is detached and cannot be dragged through a resize it
    // never asked for; it is put back at its original size afterwards.
    NSView* container = [[NSView alloc] initWithFrame:NSMakeRect(0, 0, width, oldHeight)];
    window.contentView = container;
    [window setContentSize:NSMakeSize(width, newHeight)];

    content.frame            = NSMakeRect(0, 0, width, oldHeight);
    content.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [container addSubview:content];

    INCDAWPresetBar* bar = [[INCDAWPresetBar alloc]
        initWithFrame:NSMakeRect(0, oldHeight, width, INCDAWPresetBarHeight)];
    bar.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
    [container addSubview:bar];

    objc_setAssociatedObject(window, presetBarKey, bar, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    return bar;
}

+ (nullable INCDAWPresetBar*)barInWindow:(NSWindow*)window
{
    return objc_getAssociatedObject(window, presetBarKey);
}

+ (void)refreshAppearanceInWindow:(NSWindow*)window
{
    [[self barInWindow:window] refreshAppearance];
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _entries = @[];

    _caption           = [NSTextField labelWithString:@"Preset"];
    _caption.frame     = NSMakeRect(margin, 8, 46, 18);
    _caption.font      = theme::labelFont(11.0);
    _caption.textColor = theme::ink(Ink::textSecondary);
    [self addSubview:_caption];

    _popup = [[NSPopUpButton alloc]
        initWithFrame:NSMakeRect(margin + 50, 5, frame.size.width - margin * 2 - 50, 24)
            pullsDown:NO];
    _popup.autoresizingMask = NSViewWidthSizable;
    _popup.target           = self;
    _popup.action           = @selector(menuChosen:);
    _popup.font             = theme::labelFont(11.0);
    [self addSubview:_popup];

    [self rebuildMenu];
    return self;
}

- (BOOL)isFlipped
{
    return NO;
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;
    theme::fillRect(self.bounds, theme::ink(Ink::panelRaised));

    // A hairline where the bar stops and the parameters start.
    theme::fillRect(NSMakeRect(0, 0, self.bounds.size.width, 1),
                    theme::ink(Ink::separator));
}

- (nullable NSString*)selectedName
{
    return _selected;
}

- (void)setEntries:(NSArray<NSDictionary*>*)entries selected:(nullable NSString*)selected
{
    _entries  = entries != nil ? entries : @[];
    _selected = [selected copy];
    [self rebuildMenu];
}

- (BOOL)selectedIsFactory
{
    if (_selected == nil)
        return NO;

    for (NSDictionary* entry in _entries)
        if ([entry[@"name"] isEqualToString:_selected])
            return [entry[@"factory"] boolValue];

    return NO;
}

- (void)rebuildMenu
{
    NSMenu* menu       = [[NSMenu alloc] init];
    menu.autoenablesItems = NO;

    // The title item: what is loaded, shown when nothing in the list matches
    // (a slider has been moved since the recall, or nothing was recalled).
    NSMenuItem* current = [[NSMenuItem alloc]
        initWithTitle:_selected != nil ? _selected : @(unsavedTitle)
               action:nil
        keyEquivalent:@""];
    current.tag     = -1;
    current.enabled = YES;
    [menu addItem:current];
    [menu addItem:[NSMenuItem separatorItem]];

    __block BOOL wroteUserHeading = NO;
    [_entries enumerateObjectsUsingBlock:^(NSDictionary* entry, NSUInteger index, BOOL*) {
        if (![entry[@"factory"] boolValue] && !wroteUserHeading) {
            [menu addItem:[NSMenuItem separatorItem]];
            wroteUserHeading = YES;
        }

        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:entry[@"name"]
                                                      action:@selector(menuChosen:)
                                               keyEquivalent:@""];
        item.target = self;
        item.tag    = tagPresetBase + static_cast<NSInteger>(index);
        item.state  = [entry[@"name"] isEqualToString:_selected != nil ? _selected : @""]
                          ? NSControlStateValueOn
                          : NSControlStateValueOff;
        item.enabled = YES;
        [menu addItem:item];
    }];

    [menu addItem:[NSMenuItem separatorItem]];

    const BOOL factory   = [self selectedIsFactory];
    const BOOL haveLoaded = _selected != nil;

    const auto addAction = [&menu, self](NSString* title, NSInteger tag, BOOL enabled) {
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title
                                                      action:@selector(menuChosen:)
                                               keyEquivalent:@""];
        item.target  = self;
        item.tag     = tag;
        item.enabled = enabled;
        [menu addItem:item];
    };

    addAction(@"Save As…",  tagSave,      YES);
    addAction(@"Duplicate", tagDuplicate, haveLoaded);
    addAction(@"Rename…",   tagRename,    haveLoaded && !factory);
    addAction(@"Delete",    tagDelete,    haveLoaded && !factory);

    _popup.menu = menu;
    [_popup selectItemAtIndex:0];
}

- (void)menuChosen:(NSMenuItem*)item
{
    // The popup reports through its own action as well as the items'; take
    // the selected item either way.
    if ([item isKindOfClass:NSPopUpButton.class])
        item = _popup.selectedItem;

    const NSInteger tag = item.tag;

    if (tag >= tagPresetBase) {
        const NSUInteger index = static_cast<NSUInteger>(tag - tagPresetBase);
        if (index >= _entries.count)
            return;

        NSString* name = _entries[index][@"name"];
        _selected      = [name copy];
        [self rebuildMenu];

        if (_onRecall != nil)
            _onRecall(name);
        return;
    }

    switch (tag) {
        case tagSave: {
            NSString* name = askForName(@"Save preset as:",
                                        _selected != nil ? _selected : @"My Preset",
                                        self.window);
            if (name != nil && _onSave != nil)
                _onSave(name);
            break;
        }

        case tagDuplicate:
            if (_selected != nil && _onDuplicate != nil)
                _onDuplicate(_selected);
            break;

        case tagRename: {
            if (_selected == nil)
                break;

            NSString* name = askForName(@"Rename preset to:", _selected, self.window);
            if (name != nil && _onRename != nil)
                _onRename(_selected, name);
            break;
        }

        case tagDelete: {
            if (_selected == nil)
                break;

            NSAlert* confirm    = [[NSAlert alloc] init];
            confirm.messageText = [NSString stringWithFormat:@"Delete the preset “%@”?",
                                                             _selected];
            confirm.informativeText = @"The file is removed from your presets folder.";
            [confirm addButtonWithTitle:@"Delete"];
            [confirm addButtonWithTitle:@"Cancel"];

            if ([confirm runModal] == NSAlertFirstButtonReturn && _onDelete != nil)
                _onDelete(_selected);
            break;
        }

        default:
            break;
    }

    // Whatever happened, the popup shows the loaded preset again rather than
    // the verb the user just used.
    [_popup selectItemAtIndex:0];
}

- (void)refreshAppearance
{
    _caption.textColor = theme::ink(Ink::textSecondary);
    self.needsDisplay  = YES;
}

@end
