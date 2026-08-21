#include "ui/macos/BrowserView.h"

#include "app/BrowserModel.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

using namespace incdaw;

namespace {

constexpr CGFloat rowHeight    = 22.0;
constexpr CGFloat searchHeight = 24.0;
constexpr CGFloat padding      = 6.0;
constexpr CGFloat glyphWidth   = 16.0;

NSColor* grey(CGFloat white) { return [NSColor colorWithCalibratedWhite:white alpha:1.0]; }

/// One glyph per kind. Text rather than images: the pane must stay legible at
/// any row height, and a browser that ships an icon set is a browser that has
/// to redraw it for every theme.
NSString* glyphFor(app::BrowserItemKind kind)
{
    switch (kind) {
        case app::BrowserItemKind::folder:  return @"▸";
        case app::BrowserItemKind::audio:   return @"∿";
        case app::BrowserItemKind::midi:    return @"♪";
        case app::BrowserItemKind::project: return @"◈";
        case app::BrowserItemKind::preset:  return @"◇";
        case app::BrowserItemKind::other:   break;
    }

    return @"·";
}

NSColor* colourFor(app::BrowserItemKind kind)
{
    switch (kind) {
        case app::BrowserItemKind::folder:  return [NSColor colorWithCalibratedRed:0.55 green:0.70 blue:0.95 alpha:1.0];
        case app::BrowserItemKind::audio:   return [NSColor colorWithCalibratedRed:0.55 green:0.85 blue:0.65 alpha:1.0];
        case app::BrowserItemKind::midi:    return [NSColor colorWithCalibratedRed:0.90 green:0.75 blue:0.45 alpha:1.0];
        case app::BrowserItemKind::project: return [NSColor colorWithCalibratedRed:0.85 green:0.60 blue:0.90 alpha:1.0];
        case app::BrowserItemKind::preset:  return grey(0.70);
        case app::BrowserItemKind::other:   break;
    }

    return grey(0.55);
}

/// Sizes as a musician reads them, not as bytes.
NSString* describeSize(std::uintmax_t bytes)
{
    if (bytes == 0)
        return @"";

    const double kilobytes = static_cast<double>(bytes) / 1024.0;
    if (kilobytes < 1000.0)
        return [NSString stringWithFormat:@"%.0f KB", kilobytes];

    return [NSString stringWithFormat:@"%.1f MB", kilobytes / 1024.0];
}

void drawText(NSString* text, NSRect rect, NSColor* colour, NSTextAlignment alignment)
{
    NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
    style.lineBreakMode = NSLineBreakByTruncatingMiddle;
    style.alignment     = alignment;

    [text drawInRect:rect
      withAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:11.0],
                       NSForegroundColorAttributeName: colour,
                       NSParagraphStyleAttributeName: style}];
}

/// What a row stands for. Roots and the parent entry are not files, and the
/// difference decides what a double-click does.
enum class RowRole { root, parent, item };

struct Row {
    RowRole          role = RowRole::item;
    app::BrowserItem item;
};

} // namespace

/// The seam the list view calls back through. Declared rather than published:
/// row indices are this file's business, not the shell's.
@interface INCDAWBrowserView (ListSupport)
- (void)activateRowAtIndex:(std::size_t)index;
- (NSMenu*)menuForRowAtIndex:(std::size_t)index;
@end

// ── The scrolling list ───────────────────────────────────────────────────────

/// Drawn by hand, like the rest of INCDAW's panes, and flipped so that row 0 is
/// at the top. It lives inside an NSScrollView, which is what makes a folder of
/// ten thousand samples scroll without this view knowing anything about it.
@interface INCDAWBrowserListView : NSView <NSDraggingSource>
@property (nonatomic, weak) INCDAWBrowserView* owner;

- (void)setRows:(std::vector<Row>)rows;
- (const std::vector<Row>&)rows;
- (std::size_t)selectedRow;
- (void)selectRow:(std::size_t)row;
@end

@implementation INCDAWBrowserListView {
    std::vector<Row> _rows;
    std::size_t      _selected;
    NSPoint          _mouseDownAt;
    BOOL             _dragging;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _selected = static_cast<std::size_t>(-1);
    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)setRows:(std::vector<Row>)rows
{
    _rows = std::move(rows);
    if (_selected >= _rows.size())
        _selected = static_cast<std::size_t>(-1);

    const CGFloat height = static_cast<CGFloat>(_rows.size()) * rowHeight;
    NSSize        size   = self.frame.size;

    // At least the clip view's height, so that clicks in the empty area below
    // the last row still land on this view rather than on the scroll view.
    size.height = std::max(height, self.superview != nil ? self.superview.bounds.size.height : height);
    [self setFrameSize:size];

    [self setNeedsDisplay:YES];
}

- (const std::vector<Row>&)rows { return _rows; }
- (std::size_t)selectedRow { return _selected; }

- (void)selectRow:(std::size_t)row
{
    _selected = row;
    [self setNeedsDisplay:YES];

    if (row < _rows.size())
        [self scrollRectToVisible:NSMakeRect(0, static_cast<CGFloat>(row) * rowHeight,
                                             self.bounds.size.width, rowHeight)];
}

- (void)drawRect:(NSRect)dirtyRect
{
    [grey(0.105) setFill];
    NSRectFill(dirtyRect);

    const CGFloat width = self.bounds.size.width;

    const std::size_t first = static_cast<std::size_t>(std::max<CGFloat>(dirtyRect.origin.y / rowHeight, 0.0));
    const std::size_t last  = std::min(_rows.size(),
                                       static_cast<std::size_t>(NSMaxY(dirtyRect) / rowHeight) + 1);

    for (std::size_t index = first; index < last; ++index) {
        const Row&    row  = _rows[index];
        const NSRect  rect = NSMakeRect(0, static_cast<CGFloat>(index) * rowHeight, width, rowHeight);
        const BOOL    selected = index == _selected;

        if (selected) {
            [grey(0.22) setFill];
            NSRectFill(rect);
        }

        NSString* name   = @(row.item.name.c_str());
        NSColor*  colour = grey(0.82);

        if (row.role == RowRole::parent) {
            name   = @"‹ back";
            colour = grey(0.55);
        } else if (row.role == RowRole::root) {
            colour = grey(0.92);
        }

        drawText(glyphFor(row.item.kind),
                 NSMakeRect(padding, rect.origin.y + 3.0, glyphWidth, rowHeight - 4.0),
                 colourFor(row.item.kind), NSTextAlignmentCenter);

        // A favourite is marked where the eye already is — beside the name,
        // not in a column that only exists when something is starred.
        NSString* label = row.item.favourite ? [NSString stringWithFormat:@"★ %@", name] : name;

        const CGFloat sizeWidth = 56.0;
        drawText(label,
                 NSMakeRect(padding + glyphWidth + 4.0, rect.origin.y + 3.0,
                            width - padding * 2 - glyphWidth - sizeWidth - 4.0, rowHeight - 4.0),
                 colour, NSTextAlignmentLeft);

        if (row.role == RowRole::item && !row.item.isFolder()) {
            drawText(describeSize(row.item.sizeBytes),
                     NSMakeRect(width - padding - sizeWidth, rect.origin.y + 3.0, sizeWidth, rowHeight - 4.0),
                     grey(0.45), NSTextAlignmentRight);
        }
    }
}

- (std::size_t)rowAtPoint:(NSPoint)point
{
    if (point.y < 0.0)
        return static_cast<std::size_t>(-1);

    const std::size_t row = static_cast<std::size_t>(point.y / rowHeight);
    return row < _rows.size() ? row : static_cast<std::size_t>(-1);
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint    point = [self convertPoint:event.locationInWindow fromView:nil];
    const std::size_t row  = [self rowAtPoint:point];

    _mouseDownAt = point;
    _dragging    = NO;

    [self.window makeFirstResponder:self];

    if (row == static_cast<std::size_t>(-1))
        return;

    [self selectRow:row];

    if (event.clickCount >= 2)
        [self.owner activateRowAtIndex:row];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_dragging || _selected >= _rows.size())
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    if (std::abs(point.x - _mouseDownAt.x) < 4.0 && std::abs(point.y - _mouseDownAt.y) < 4.0)
        return;

    const Row& row = _rows[_selected];
    if (row.role == RowRole::parent || row.item.isFolder())
        return;

    // Dragging out a file URL is what lets the same drop handler serve the
    // browser and the Finder: the Channel Rack accepts a file, and does not
    // care which pane it came from.
    NSURL* url = [NSURL fileURLWithPath:@(row.item.path.c_str())];

    NSPasteboardItem* pasteboardItem = [[NSPasteboardItem alloc] init];
    [pasteboardItem setString:url.absoluteString forType:NSPasteboardTypeFileURL];

    NSDraggingItem* draggingItem = [[NSDraggingItem alloc] initWithPasteboardWriter:pasteboardItem];

    const NSRect rect = NSMakeRect(point.x - 60.0, point.y - rowHeight / 2.0, 160.0, rowHeight);

    NSString* label = @(row.item.name.c_str());

    NSImage* image = [NSImage imageWithSize:rect.size flipped:NO drawingHandler:^BOOL(NSRect bounds) {
        [[NSColor colorWithCalibratedWhite:0.25 alpha:0.9] setFill];
        NSRectFill(bounds);
        drawText(label, NSMakeRect(4.0, 3.0, bounds.size.width - 8.0, bounds.size.height - 5.0),
                 [NSColor whiteColor], NSTextAlignmentLeft);
        return YES;
    }];

    [draggingItem setDraggingFrame:rect contents:image];

    _dragging = YES;
    [self beginDraggingSessionWithItems:@[draggingItem] event:event source:self];
}

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
    (void)session;
    (void)context;
    return NSDragOperationCopy;
}

- (void)draggingSession:(NSDraggingSession*)session
           endedAtPoint:(NSPoint)screenPoint
              operation:(NSDragOperation)operation
{
    (void)session;
    (void)screenPoint;
    (void)operation;
    _dragging = NO;
}

- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint     point = [self convertPoint:event.locationInWindow fromView:nil];
    const std::size_t row   = [self rowAtPoint:point];
    if (row == static_cast<std::size_t>(-1))
        return;

    [self selectRow:row];
    [NSMenu popUpContextMenu:[self.owner menuForRowAtIndex:row] withEvent:event forView:self];
}

- (void)keyDown:(NSEvent*)event
{
    const unichar key = event.charactersIgnoringModifiers.length > 0
                            ? [event.charactersIgnoringModifiers characterAtIndex:0]
                            : 0;

    switch (key) {
        case NSUpArrowFunctionKey:
            if (_selected > 0 && _selected != static_cast<std::size_t>(-1))
                [self selectRow:_selected - 1];
            else if (!_rows.empty())
                [self selectRow:0];
            return;

        case NSDownArrowFunctionKey:
            if (_selected == static_cast<std::size_t>(-1))
                [self selectRow:0];
            else if (_selected + 1 < _rows.size())
                [self selectRow:_selected + 1];
            return;

        case '\r':
        case '\n':
            if (_selected < _rows.size())
                [self.owner activateRowAtIndex:_selected];
            return;

        case ' ':
            // The transport, from whichever pane has focus.
            if (self.owner.onTransportToggle != nil)
                self.owner.onTransportToggle();
            return;

        default:
            break;
    }

    [super keyDown:event];
}

@end

// ── The pane ─────────────────────────────────────────────────────────────────

@interface INCDAWBrowserView () <NSSearchFieldDelegate>
@end

@implementation INCDAWBrowserView {
    app::BrowserModel* _browser;

    NSSearchField*           _search;
    NSScrollView*            _scroll;
    INCDAWBrowserListView*   _list;

    /// Empty means the places level: the roots and Favourites. Otherwise the
    /// folder being shown.
    std::filesystem::path _location;

    /// True while showing the Favourites pseudo-folder.
    BOOL _showingFavourites;
}

- (instancetype)initWithFrame:(NSRect)frame browser:(app::BrowserModel*)browser
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _browser = browser;

    _search = [[NSSearchField alloc] initWithFrame:NSMakeRect(padding, frame.size.height - searchHeight - padding,
                                                              frame.size.width - padding * 2, searchHeight)];
    _search.placeholderString = @"Search libraries";
    _search.font              = [NSFont systemFontOfSize:11.0];
    _search.delegate          = self;
    _search.target            = self;
    _search.action            = @selector(searchChanged:);
    _search.autoresizingMask  = NSViewWidthSizable | NSViewMinYMargin;
    [self addSubview:_search];

    const NSRect listFrame = NSMakeRect(0, 0, frame.size.width,
                                        frame.size.height - searchHeight - padding * 2);

    _scroll = [[NSScrollView alloc] initWithFrame:listFrame];
    _scroll.hasVerticalScroller = YES;
    _scroll.drawsBackground     = NO;
    _scroll.autoresizingMask    = NSViewWidthSizable | NSViewHeightSizable;

    _list = [[INCDAWBrowserListView alloc] initWithFrame:NSMakeRect(0, 0, listFrame.size.width, 0)];
    _list.owner              = self;
    _list.autoresizingMask   = NSViewWidthSizable;
    _scroll.documentView     = _list;

    [self addSubview:_scroll];

    [self reload];
    return self;
}

- (BOOL)isFlipped { return YES; }

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;
    [grey(0.105) setFill];
    NSRectFill(self.bounds);
}

// ── Contents ─────────────────────────────────────────────────────────────────

- (void)reload
{
    if (_browser == nullptr)
        return;

    std::vector<Row> rows;

    const std::string query = _search.stringValue.UTF8String;

    if (!query.empty()) {
        for (const app::BrowserItem& item : _browser->search(query))
            rows.push_back(Row{RowRole::item, item});

        if (_browser->lastSearchWasTruncated()) {
            app::BrowserItem more;
            more.name = "… narrow the search to see more";
            more.kind = app::BrowserItemKind::other;
            rows.push_back(Row{RowRole::item, more});
        }
    } else if (_showingFavourites) {
        app::BrowserItem back;
        back.kind = app::BrowserItemKind::folder;
        rows.push_back(Row{RowRole::parent, back});

        for (const app::BrowserItem& item : _browser->favourites())
            rows.push_back(Row{RowRole::item, item});
    } else if (_location.empty()) {
        app::BrowserItem favourites;
        favourites.name = "Favourites";
        favourites.kind = app::BrowserItemKind::folder;
        rows.push_back(Row{RowRole::root, favourites});

        for (const app::BrowserModel::Root& root : _browser->roots()) {
            app::BrowserItem item;
            item.path = root.path;
            item.name = root.name;
            item.kind = app::BrowserItemKind::folder;
            rows.push_back(Row{RowRole::root, item});
        }
    } else {
        app::BrowserItem back;
        back.path = _location.parent_path();
        back.kind = app::BrowserItemKind::folder;
        rows.push_back(Row{RowRole::parent, back});

        for (const app::BrowserItem& item : _browser->childrenOf(_location))
            rows.push_back(Row{RowRole::item, item});
    }

    [_list setRows:std::move(rows)];
}

- (void)searchChanged:(id)sender
{
    (void)sender;
    [self reload];
}

- (void)controlTextDidChange:(NSNotification*)notification
{
    (void)notification;
    [self reload];
}

// ── Gestures ─────────────────────────────────────────────────────────────────

- (void)activateRowAtIndex:(std::size_t)index
{
    const std::vector<Row>& rows = [_list rows];
    if (index >= rows.size())
        return;

    const Row& row = rows[index];

    // The truncation notice, and anything else that stands for no file.
    if (row.role == RowRole::item && row.item.path.empty())
        return;

    if (row.role == RowRole::parent) {
        if (_showingFavourites) {
            _showingFavourites = NO;
            _location.clear();
        } else {
            // A root's parent is the places level, not the folder above it:
            // the browser shows the libraries the user added, and walking out
            // of one into the file system at large is not navigation, it is
            // getting lost.
            _location = [self isRootPath:_location] ? std::filesystem::path{} : row.item.path;
        }

        [self reload];
        return;
    }

    if (row.role == RowRole::root && row.item.path.empty()) {
        _showingFavourites = YES;
        [self reload];
        return;
    }

    if (row.item.isFolder()) {
        _location          = row.item.path;
        _showingFavourites = NO;
        _search.stringValue = @"";
        [self reload];
        return;
    }

    if (self.onActivateFile != nil)
        self.onActivateFile(@(row.item.path.c_str()));
}

- (BOOL)isRootPath:(const std::filesystem::path&)path
{
    for (const app::BrowserModel::Root& root : _browser->roots())
        if (root.path == path)
            return YES;

    return NO;
}

- (NSMenu*)menuForRowAtIndex:(std::size_t)index
{
    NSMenu* menu = [[NSMenu alloc] init];

    const std::vector<Row>& rows = [_list rows];
    if (index >= rows.size())
        return menu;

    const Row& row = rows[index];

    if (row.role != RowRole::parent && !row.item.path.empty()) {
        NSMenuItem* favourite = [menu addItemWithTitle:row.item.favourite ? @"Remove from Favourites"
                                                                          : @"Add to Favourites"
                                                action:@selector(toggleFavourite:)
                                         keyEquivalent:@""];
        favourite.target           = self;
        favourite.representedObject = @(row.item.path.c_str());

        NSMenuItem* reveal = [menu addItemWithTitle:@"Reveal in Finder"
                                             action:@selector(revealInFinder:)
                                      keyEquivalent:@""];
        reveal.target            = self;
        reveal.representedObject = @(row.item.path.c_str());

        [menu addItem:[NSMenuItem separatorItem]];
    }

    if (row.role == RowRole::root && !row.item.path.empty()) {
        NSMenuItem* remove = [menu addItemWithTitle:@"Remove Library"
                                             action:@selector(removeRoot:)
                                      keyEquivalent:@""];
        remove.target            = self;
        remove.representedObject = @(row.item.path.c_str());
    } else if (row.item.isFolder() && !row.item.path.empty()) {
        NSMenuItem* add = [menu addItemWithTitle:@"Add as Library"
                                          action:@selector(addRootFromMenu:)
                                   keyEquivalent:@""];
        add.target            = self;
        add.representedObject = @(row.item.path.c_str());
    }

    NSMenuItem* addFolder = [menu addItemWithTitle:@"Add Library Folder…"
                                            action:@selector(addLibrary:)
                                     keyEquivalent:@""];
    addFolder.target = self;

    return menu;
}

- (void)toggleFavourite:(NSMenuItem*)item
{
    _browser->toggleFavourite(std::filesystem::path{[item.representedObject UTF8String]});
    [self libraryChanged];
}

- (void)revealInFinder:(NSMenuItem*)item
{
    NSURL* url = [NSURL fileURLWithPath:item.representedObject];
    [[NSWorkspace sharedWorkspace] activateFileViewerSelectingURLs:@[url]];
}

- (void)removeRoot:(NSMenuItem*)item
{
    _browser->removeRoot(std::filesystem::path{[item.representedObject UTF8String]});
    _location.clear();
    [self libraryChanged];
}

- (void)addRootFromMenu:(NSMenuItem*)item
{
    _browser->addRoot(std::filesystem::path{[item.representedObject UTF8String]});
    [self libraryChanged];
}

- (void)addLibrary:(id)sender
{
    (void)sender;

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = NO;
    panel.canChooseDirectories    = YES;
    panel.allowsMultipleSelection = NO;
    panel.prompt                  = @"Add Library";

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    _browser->addRoot(std::filesystem::path{panel.URL.path.UTF8String});
    [self libraryChanged];
}

- (void)libraryChanged
{
    [self reload];

    if (self.onLibraryChanged != nil)
        self.onLibraryChanged();
}

@end
