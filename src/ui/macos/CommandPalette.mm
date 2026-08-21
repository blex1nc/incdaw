#include "ui/macos/CommandPalette.h"

#include <algorithm>

namespace {

constexpr CGFloat paletteWidth = 560.0;
constexpr CGFloat queryHeight  = 38.0;
constexpr CGFloat rowHeight    = 26.0;
constexpr CGFloat padding      = 12.0;
constexpr NSUInteger visibleRows = 10;

NSColor* grey(CGFloat white) { return [NSColor colorWithCalibratedWhite:white alpha:1.0]; }

void drawText(NSString* text, NSRect rect, NSColor* colour, CGFloat size, NSTextAlignment alignment)
{
    NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
    style.lineBreakMode = NSLineBreakByTruncatingTail;
    style.alignment     = alignment;

    [text drawInRect:rect
      withAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:size],
                       NSForegroundColorAttributeName: colour,
                       NSParagraphStyleAttributeName: style}];
}

/// How well `entry` answers `query`, or `NSNotFound` for no match.
///
/// Ranking, not just filtering: with a hundred actions, "mix" must offer the
/// Mixer before "Import MIDI" — a title that starts with what was typed is
/// almost always the one meant.
NSUInteger scoreOf(INCDAWCommandEntry* entry, NSString* query)
{
    if (query.length == 0)
        return 100;

    const NSRange inTitle = [entry.title rangeOfString:query options:NSCaseInsensitiveSearch];
    if (inTitle.location == 0)
        return 0;

    if (inTitle.location != NSNotFound)
        return 10 + inTitle.location;

    const NSRange inCategory = [entry.category rangeOfString:query options:NSCaseInsensitiveSearch];
    if (inCategory.location != NSNotFound)
        return 1000 + inCategory.location;

    return NSNotFound;
}

} // namespace

@implementation INCDAWCommandEntry

+ (instancetype)entryWithTitle:(NSString*)title
                      category:(NSString*)category
                      shortcut:(NSString*)shortcut
                           run:(void (^)(void))run
{
    INCDAWCommandEntry* entry = [[INCDAWCommandEntry alloc] init];
    entry.title    = title;
    entry.category = category != nil ? category : @"";
    entry.shortcut = shortcut != nil ? shortcut : @"";
    entry.run      = run;
    return entry;
}

@end

// ── The view: query line and results, drawn and keyed by hand ────────────────

@interface INCDAWCommandPaletteView : NSView
@property (nonatomic, weak) INCDAWCommandPalette* palette;
@property (nonatomic, strong) NSArray<INCDAWCommandEntry*>* entries;
@end

@implementation INCDAWCommandPaletteView {
    NSMutableString*                    _query;
    NSArray<INCDAWCommandEntry*>*       _matches;
    NSUInteger                          _selected;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _query    = [NSMutableString string];
    _matches  = @[];
    _selected = 0;
    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)reset
{
    [_query setString:@""];
    _selected = 0;
    [self refilter];
}

/// Ranks and orders the matches. A stable sort keeps equally-scored entries in
/// the order the shell gathered them, which is menu order — the order the user
/// already knows.
- (void)refilter
{
    NSMutableArray<INCDAWCommandEntry*>* matched = [NSMutableArray array];
    NSMutableArray<NSNumber*>*           scores  = [NSMutableArray array];

    for (INCDAWCommandEntry* entry in self.entries) {
        const NSUInteger score = scoreOf(entry, _query);
        if (score == NSNotFound)
            continue;

        [matched addObject:entry];
        [scores addObject:@(score)];
    }

    NSMutableArray<NSNumber*>* order = [NSMutableArray array];
    for (NSUInteger index = 0; index < matched.count; ++index)
        [order addObject:@(index)];

    [order sortWithOptions:NSSortStable usingComparator:^NSComparisonResult(NSNumber* left, NSNumber* right) {
        const NSUInteger leftScore  = scores[left.unsignedIntegerValue].unsignedIntegerValue;
        const NSUInteger rightScore = scores[right.unsignedIntegerValue].unsignedIntegerValue;

        if (leftScore < rightScore) return NSOrderedAscending;
        if (leftScore > rightScore) return NSOrderedDescending;
        return NSOrderedSame;
    }];

    NSMutableArray<INCDAWCommandEntry*>* ordered = [NSMutableArray array];
    for (NSNumber* index in order)
        [ordered addObject:matched[index.unsignedIntegerValue]];

    _matches = ordered;
    if (_selected >= _matches.count)
        _selected = _matches.count > 0 ? _matches.count - 1 : 0;

    [self setNeedsDisplay:YES];
}

- (NSUInteger)matchCount { return _matches.count; }

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    [grey(0.14) setFill];
    NSRectFill(self.bounds);

    [grey(0.30) setStroke];
    NSFrameRect(self.bounds);

    // The query line, with a caret so an empty palette does not look inert.
    NSString* line = _query.length > 0 ? [NSString stringWithFormat:@"%@|", _query]
                                       : @"Type a command…";

    drawText(line, NSMakeRect(padding, 9.0, self.bounds.size.width - padding * 2, queryHeight - 12.0),
             _query.length > 0 ? grey(0.95) : grey(0.45), 15.0, NSTextAlignmentLeft);

    [grey(0.24) setFill];
    NSRectFill(NSMakeRect(0, queryHeight, self.bounds.size.width, 1.0));

    const NSUInteger shown = std::min<NSUInteger>(_matches.count, visibleRows);

    // Scrolled so the selection stays visible without the palette growing.
    const NSUInteger firstRow = _selected >= shown ? _selected - shown + 1 : 0;

    for (NSUInteger offset = 0; offset < shown; ++offset) {
        const NSUInteger index = firstRow + offset;
        if (index >= _matches.count)
            break;

        INCDAWCommandEntry* entry = _matches[index];

        const NSRect rect = NSMakeRect(0, queryHeight + 1.0 + static_cast<CGFloat>(offset) * rowHeight,
                                       self.bounds.size.width, rowHeight);

        if (index == _selected) {
            [[NSColor colorWithCalibratedRed:0.20 green:0.34 blue:0.52 alpha:1.0] setFill];
            NSRectFill(rect);
        }

        drawText(entry.title,
                 NSMakeRect(padding, rect.origin.y + 5.0, rect.size.width - 260.0, rowHeight - 7.0),
                 grey(0.92), 12.0, NSTextAlignmentLeft);

        drawText(entry.category,
                 NSMakeRect(rect.size.width - 250.0, rect.origin.y + 5.0, 150.0, rowHeight - 7.0),
                 grey(0.50), 11.0, NSTextAlignmentRight);

        drawText(entry.shortcut,
                 NSMakeRect(rect.size.width - 90.0, rect.origin.y + 5.0, 90.0 - padding, rowHeight - 7.0),
                 grey(0.62), 11.0, NSTextAlignmentRight);
    }

    if (_matches.count == 0) {
        drawText(@"No matching command",
                 NSMakeRect(padding, queryHeight + 8.0, self.bounds.size.width - padding * 2, rowHeight),
                 grey(0.40), 12.0, NSTextAlignmentLeft);
    }
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    if (point.y < queryHeight)
        return;

    const NSUInteger shown    = std::min<NSUInteger>(_matches.count, visibleRows);
    const NSUInteger firstRow = _selected >= shown ? _selected - shown + 1 : 0;
    const NSUInteger offset   = static_cast<NSUInteger>((point.y - queryHeight) / rowHeight);

    if (firstRow + offset >= _matches.count)
        return;

    _selected = firstRow + offset;
    [self runSelected];
}

- (void)keyDown:(NSEvent*)event
{
    const unichar key = event.charactersIgnoringModifiers.length > 0
                            ? [event.charactersIgnoringModifiers characterAtIndex:0]
                            : 0;

    switch (key) {
        case '\033':                                   // Esc
            [self.palette close];
            return;

        case '\r':
        case '\n':
            [self runSelected];
            return;

        case NSUpArrowFunctionKey:
            if (_selected > 0)
                --_selected;
            [self setNeedsDisplay:YES];
            return;

        case NSDownArrowFunctionKey:
            if (_selected + 1 < _matches.count)
                ++_selected;
            [self setNeedsDisplay:YES];
            return;

        case NSDeleteCharacter:
        case NSBackspaceCharacter:
            if (_query.length > 0)
                [_query deleteCharactersInRange:NSMakeRange(_query.length - 1, 1)];
            _selected = 0;
            [self refilter];
            return;

        default:
            break;
    }

    // Typing. Modifier combinations are left alone so that ⌘Q still quits.
    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        [super keyDown:event];
        return;
    }

    NSString* typed = event.characters;
    if (typed.length == 0 || [typed characterAtIndex:0] < ' ')
        return;

    [_query appendString:typed];
    _selected = 0;
    [self refilter];
}

- (void)runSelected
{
    if (_selected >= _matches.count)
        return;

    INCDAWCommandEntry* entry = _matches[_selected];

    // Closed first: an action that opens a window or a modal panel must not
    // find the palette still floating in front of it.
    [self.palette close];

    if (entry.run != nil)
        entry.run();
}

@end

// ── The panel ────────────────────────────────────────────────────────────────

@implementation INCDAWCommandPalette {
    NSPanel*                    _panel;
    INCDAWCommandPaletteView*   _view;
    id                          _resignObserver;
}

- (void)showWithEntries:(NSArray<INCDAWCommandEntry*>*)entries relativeToWindow:(NSWindow*)window
{
    const CGFloat height = queryHeight + 1.0 + rowHeight * visibleRows;

    if (_panel == nil) {
        _panel = [[NSPanel alloc]
            initWithContentRect:NSMakeRect(0, 0, paletteWidth, height)
                      styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
                        backing:NSBackingStoreBuffered
                          defer:NO];

        _panel.releasedWhenClosed = NO;
        _panel.hidesOnDeactivate  = YES;
        _panel.level              = NSFloatingWindowLevel;
        _panel.backgroundColor    = [NSColor clearColor];
        _panel.opaque             = NO;

        _view = [[INCDAWCommandPaletteView alloc]
            initWithFrame:NSMakeRect(0, 0, paletteWidth, height)];
        _view.palette      = self;
        _panel.contentView = _view;
    }

    _view.entries = entries;
    [_view reset];

    // Centred on the upper third of the main window: where the eye already is
    // when reaching for a command, and clear of the panes being worked on.
    if (window != nil) {
        const NSRect frame = window.frame;
        [_panel setFrameOrigin:NSMakePoint(NSMidX(frame) - paletteWidth / 2.0,
                                           NSMaxY(frame) - height - frame.size.height * 0.22)];
    }

    [_panel makeKeyAndOrderFront:nil];
    [_panel makeFirstResponder:_view];

    // Clicking away dismisses it, like every palette: a floating window that
    // survives losing focus is one the user has to go back and close.
    if (_resignObserver == nil) {
        __weak INCDAWCommandPalette* weakSelf = self;
        _resignObserver = [[NSNotificationCenter defaultCenter]
            addObserverForName:NSWindowDidResignKeyNotification
                        object:_panel
                         queue:[NSOperationQueue mainQueue]
                    usingBlock:^(NSNotification*) { [weakSelf close]; }];
    }
}

- (void)close
{
    [_panel orderOut:nil];
}

- (void)dealloc
{
    if (_resignObserver != nil)
        [[NSNotificationCenter defaultCenter] removeObserver:_resignObserver];
}

@end
