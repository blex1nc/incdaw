#include "ui/macos/BrowserView.h"

#include "app/Browser.h"
#include "ui/macos/Theme.h"

#include <filesystem>
#include <string>

using namespace incdaw;
using incdaw::ui::theme::Ink;

namespace theme = incdaw::ui::theme;

namespace {

constexpr CGFloat searchHeight = 26.0;
constexpr CGFloat padding      = 6.0;
constexpr CGFloat headerHeight = 26.0;   ///< the pane's own title band

/// Group headers are items too, so they need identities the outline can hold.
enum class GroupKind { none, favourites, recent };

NSString* symbolFor(app::BrowserItemKind kind)
{
    switch (kind) {
        case app::BrowserItemKind::folder:  return @"folder";
        case app::BrowserItemKind::project: return @"square.stack";
        case app::BrowserItemKind::audio:   return @"waveform";
        case app::BrowserItemKind::midi:    return @"music.note";
        case app::BrowserItemKind::plugin:  return @"puzzlepiece.extension";
        case app::BrowserItemKind::unknown: break;
    }

    return @"doc";
}

std::filesystem::path pathOf(NSString* text)
{
    return text == nil ? std::filesystem::path{} : std::filesystem::path{text.UTF8String};
}

} // namespace

/// One row. Children are loaded when a folder is first opened and kept until
/// the pane is reloaded, so scrolling never re-reads the disk.
@interface INCDAWBrowserNode : NSObject
@property (nonatomic, copy)   NSString*  name;
@property (nonatomic, copy)   NSString*  path;        ///< nil for group headers
@property (nonatomic, assign) app::BrowserItemKind kind;
@property (nonatomic, assign) GroupKind  group;
@property (nonatomic, assign) BOOL       container;
@property (nonatomic, assign) BOOL       missing;
@property (nonatomic, assign) BOOL       root;
@property (nonatomic, strong) NSMutableArray<INCDAWBrowserNode*>* children;
@end

@implementation INCDAWBrowserNode
@end

@interface INCDAWBrowserView () <NSOutlineViewDataSource, NSOutlineViewDelegate, NSMenuDelegate>
@end

/// The row behind a selected item. AppKit's own highlight is the system blue,
/// which is the one colour in this window nobody chose: the accent belongs to
/// the palette like every other surface (docs/DECISIONS.md D-035).
@interface INCDAWBrowserRowView : NSTableRowView
@end

@implementation INCDAWBrowserRowView

- (void)drawSelectionInRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    const NSRect rect = NSInsetRect(self.bounds, 2.0, 1.0);

    theme::fillRounded(rect, theme::metrics::radiusPad,
                       theme::withAlpha(theme::ink(Ink::accent), 0.35));
    theme::strokeRounded(rect, theme::metrics::radiusPad,
                         theme::withAlpha(theme::ink(Ink::accent), 0.75));
}

@end

@implementation INCDAWBrowserView {
    app::Browser*   _browser;
    NSOutlineView*  _outline;
    NSSearchField*  _search;

    NSMutableArray<INCDAWBrowserNode*>* _top;
    NSMutableArray<INCDAWBrowserNode*>* _results;
    BOOL                                _searching;
}

- (instancetype)initWithFrame:(NSRect)frame browser:(app::Browser*)browser
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _browser   = browser;
    _top       = [NSMutableArray array];
    _results   = [NSMutableArray array];
    _searching = NO;

    // The rest of the shell draws itself dark whatever the system is set to.
    // Asking AppKit for the dark appearance keeps this pane's stock controls
    // in the same room as the custom-drawn ones.
    self.appearance = [NSAppearance appearanceNamed:theme::paletteIsLight()
                                                        ? NSAppearanceNameAqua
                                                        : NSAppearanceNameDarkAqua];

    _search = [[NSSearchField alloc]
        initWithFrame:NSMakeRect(padding,
                                 frame.size.height - headerHeight - searchHeight - padding,
                                 frame.size.width - 2 * padding, searchHeight)];
    _search.placeholderString  = @"Search";
    _search.target             = self;
    _search.action             = @selector(searchChanged:);
    _search.autoresizingMask   = NSViewWidthSizable | NSViewMinYMargin;
    // Fires per keystroke rather than on Return: a browser search that makes
    // you commit before showing anything is a dialog, not a browser.
    _search.sendsSearchStringImmediately = YES;
    [self addSubview:_search];

    const NSRect treeFrame =
        NSMakeRect(0, 0, frame.size.width,
                   frame.size.height - headerHeight - searchHeight - 2 * padding);

    NSScrollView* scroll = [[NSScrollView alloc] initWithFrame:treeFrame];
    scroll.hasVerticalScroller = YES;
    scroll.drawsBackground     = NO;
    scroll.autoresizingMask    = NSViewWidthSizable | NSViewHeightSizable;

    _outline                  = [[NSOutlineView alloc] initWithFrame:treeFrame];
    _outline.headerView       = nil;
    _outline.rowSizeStyle     = NSTableViewRowSizeStyleSmall;
    _outline.indentationPerLevel = 12.0;
    _outline.backgroundColor  = theme::ink(Ink::panel);
    _outline.dataSource       = self;
    _outline.delegate         = self;
    _outline.target           = self;
    _outline.action           = @selector(rowClicked:);
    _outline.doubleAction     = @selector(rowDoubleClicked:);

    NSTableColumn* column = [[NSTableColumn alloc] initWithIdentifier:@"name"];
    column.width          = treeFrame.size.width - 4.0;
    column.resizingMask   = NSTableColumnAutoresizingMask;
    [_outline addTableColumn:column];
    _outline.outlineTableColumn = column;

    NSMenu* menu = [[NSMenu alloc] init];
    menu.delegate = self;
    _outline.menu = menu;

    scroll.documentView = _outline;
    [self addSubview:scroll];

    [self reload];

    // The outline's background is a colour handed to AppKit once, not a
    // binding: without this, a theme change repaints every pane the browser
    // sits beside and leaves the browser's own ground behind (D-039).
    __weak INCDAWBrowserView* weakSelf = self;
    [NSNotificationCenter.defaultCenter
        addObserverForName:theme::kPaletteChangedNotification
                    object:nil
                     queue:NSOperationQueue.mainQueue
                usingBlock:^(NSNotification* note) {
                    (void)note;
                    weakSelf.appearance =
                        [NSAppearance appearanceNamed:theme::paletteIsLight()
                                                          ? NSAppearanceNameAqua
                                                          : NSAppearanceNameDarkAqua];
                    weakSelf.outlineView.backgroundColor = theme::ink(Ink::panel);
                    [weakSelf.outlineView reloadData];
                }];

    return self;
}

/// The outline, for the palette observer above. Not part of the public
/// interface: nothing outside this file has any business reaching into it.
- (NSOutlineView*)outlineView { return _outline; }

// ── The tree ────────────────────────────────────────────────────────────────

- (INCDAWBrowserNode*)nodeForItem:(const app::BrowserItem&)item
{
    INCDAWBrowserNode* node = [[INCDAWBrowserNode alloc] init];
    node.name      = @(item.name.c_str());
    node.path      = @(item.path.string().c_str());
    node.kind      = item.kind;
    node.group     = GroupKind::none;
    node.container = item.kind == app::BrowserItemKind::folder && item.exists;
    node.missing   = !item.exists;
    return node;
}

- (void)reload
{
    [_top removeAllObjects];

    INCDAWBrowserNode* favourites = [[INCDAWBrowserNode alloc] init];
    favourites.name      = @"Favourites";
    favourites.group     = GroupKind::favourites;
    favourites.container = YES;
    [_top addObject:favourites];

    INCDAWBrowserNode* recent = [[INCDAWBrowserNode alloc] init];
    recent.name      = @"Recent";
    recent.group     = GroupKind::recent;
    recent.container = YES;
    [_top addObject:recent];

    for (const app::BrowserRoot& root : _browser->roots()) {
        INCDAWBrowserNode* node = [[INCDAWBrowserNode alloc] init];
        node.name      = @(root.name.c_str());
        node.path      = @(root.path.string().c_str());
        node.kind      = app::BrowserItemKind::folder;
        node.group     = GroupKind::none;
        node.container = YES;
        node.root      = YES;
        [_top addObject:node];
    }

    [_outline reloadData];
}

- (NSMutableArray<INCDAWBrowserNode*>*)childrenOf:(INCDAWBrowserNode*)node
{
    if (node.children != nil)
        return node.children;

    NSMutableArray<INCDAWBrowserNode*>* children = [NSMutableArray array];

    if (node.group == GroupKind::favourites) {
        for (const app::BrowserItem& item : _browser->favouriteItems())
            [children addObject:[self nodeForItem:item]];
    } else if (node.group == GroupKind::recent) {
        for (const app::BrowserItem& item : _browser->recentItems())
            [children addObject:[self nodeForItem:item]];
    } else if (node.path != nil) {
        std::string error;

        for (const app::BrowserItem& item : _browser->list(pathOf(node.path), error))
            [children addObject:[self nodeForItem:item]];

        if (!error.empty()) {
            INCDAWBrowserNode* message = [[INCDAWBrowserNode alloc] init];
            message.name    = @(error.c_str());
            message.missing = YES;
            [children addObject:message];
        }
    }

    node.children = children;
    return children;
}

// ── Data source ─────────────────────────────────────────────────────────────

- (NSInteger)outlineView:(NSOutlineView*)outlineView numberOfChildrenOfItem:(id)item
{
    (void)outlineView;

    if (_searching)
        return item == nil ? static_cast<NSInteger>(_results.count) : 0;

    if (item == nil)
        return static_cast<NSInteger>(_top.count);

    INCDAWBrowserNode* node = item;
    return node.container ? static_cast<NSInteger>([self childrenOf:node].count) : 0;
}

- (id)outlineView:(NSOutlineView*)outlineView child:(NSInteger)index ofItem:(id)item
{
    (void)outlineView;

    if (_searching)
        return _results[static_cast<NSUInteger>(index)];

    if (item == nil)
        return _top[static_cast<NSUInteger>(index)];

    return [self childrenOf:item][static_cast<NSUInteger>(index)];
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isItemExpandable:(id)item
{
    (void)outlineView;

    if (_searching)
        return NO;

    INCDAWBrowserNode* node = item;
    return node.container;
}

- (BOOL)outlineView:(NSOutlineView*)outlineView isGroupItem:(id)item
{
    (void)outlineView;

    INCDAWBrowserNode* node = item;
    return node.group != GroupKind::none;
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    theme::drawText(@"BROWSER",
                    NSMakeRect(padding, self.bounds.size.height - headerHeight + 8.0,
                               self.bounds.size.width - padding * 2.0, 14.0),
                    theme::ink(Ink::textDim), theme::labelFont(9.5, NSFontWeightBold));

    // The hard edge against the pane beside it, drawn here rather than left to
    // the split view, which has no palette.
    theme::drawSeparator(NSMakeRect(self.bounds.size.width - 1.0, 0, 1.0,
                                    self.bounds.size.height));
}

- (NSTableRowView*)outlineView:(NSOutlineView*)outlineView rowViewForItem:(id)item
{
    (void)outlineView;
    (void)item;

    return [[INCDAWBrowserRowView alloc] init];
}

- (NSView*)outlineView:(NSOutlineView*)outlineView
    viewForTableColumn:(NSTableColumn*)tableColumn
                  item:(id)item
{
    (void)tableColumn;

    INCDAWBrowserNode* node = item;

    NSTableCellView* cell = [outlineView makeViewWithIdentifier:@"browserCell" owner:self];

    if (cell == nil) {
        cell            = [[NSTableCellView alloc] initWithFrame:NSMakeRect(0, 0, 200, 18)];
        cell.identifier = @"browserCell";

        NSImageView* image = [[NSImageView alloc] initWithFrame:NSMakeRect(0, 1, 14, 14)];
        [cell addSubview:image];
        cell.imageView = image;

        NSTextField* text     = [NSTextField labelWithString:@""];
        text.frame            = NSMakeRect(18, 0, 180, 16);
        text.font             = [NSFont systemFontOfSize:11.0];
        text.lineBreakMode    = NSLineBreakByTruncatingMiddle;
        text.autoresizingMask = NSViewWidthSizable;
        [cell addSubview:text];
        cell.textField = text;
    }

    cell.textField.stringValue = node.name != nil ? node.name : @"";
    cell.textField.textColor   = node.missing ? theme::ink(Ink::textDim)
                                              : theme::ink(Ink::textPrimary);

    if (node.group != GroupKind::none) {
        cell.imageView.image     = nil;
        cell.textField.font      = theme::labelFont(9.5, NSFontWeightBold);
        cell.textField.textColor = theme::ink(Ink::textDim);
    } else {
        cell.textField.font  = theme::labelFont(11.0, NSFontWeightRegular);
        cell.imageView.image = [NSImage imageWithSystemSymbolName:symbolFor(node.kind)
                                         accessibilityDescription:nil];
    }

    return cell;
}

/// Rows are drag sources: an audio file leaves the pane as a file URL, which
/// is exactly what the Channel Rack and the Playlist read from the pasteboard.
/// Nothing INCDAW-specific rides along, so a drop into another application is
/// a file drop and a drop from Finder is indistinguishable from one of these.
- (id<NSPasteboardWriting>)outlineView:(NSOutlineView*)outlineView
               pasteboardWriterForItem:(id)item
{
    (void)outlineView;

    INCDAWBrowserNode* node = item;

    if (node == nil || node.missing || node.path == nil
        || node.kind != app::BrowserItemKind::audio)
        return nil;

    return [NSURL fileURLWithPath:node.path];
}

// ── Search ──────────────────────────────────────────────────────────────────

- (void)searchChanged:(id)sender
{
    (void)sender;

    const std::string query = _search.stringValue.UTF8String != nullptr
                                  ? std::string(_search.stringValue.UTF8String)
                                  : std::string{};

    _searching = !query.empty();
    [_results removeAllObjects];

    if (_searching) {
        // The cap is shared across roots: a search is capped by how much a
        // person will read, not by how many libraries they happen to have.
        std::size_t remaining = app::Browser::defaultSearchLimit;

        for (const app::BrowserRoot& root : _browser->roots()) {
            if (remaining == 0)
                break;

            const auto found = _browser->search(root.path, query, remaining);

            for (const app::BrowserItem& item : found)
                [_results addObject:[self nodeForItem:item]];

            remaining -= found.size();
        }
    }

    [_outline reloadData];
}

// ── Activation and the context menu ─────────────────────────────────────────

- (INCDAWBrowserNode*)clickedNode
{
    const NSInteger row = _outline.clickedRow;
    return row < 0 ? nil : [_outline itemAtRow:row];
}

/// A single click. Audio auditions; anything else stops whatever was
/// auditioning, so that clicking away is the same gesture as stopping.
- (void)rowClicked:(id)sender
{
    (void)sender;

    INCDAWBrowserNode* node = [self clickedNode];

    if (node == nil || node.missing || node.path == nil
        || node.kind != app::BrowserItemKind::audio) {
        if (self.onStopPreview != nil)
            self.onStopPreview();

        return;
    }

    if (self.onPreview != nil)
        self.onPreview(node.path);
}

- (void)rowDoubleClicked:(id)sender
{
    (void)sender;

    INCDAWBrowserNode* node = [self clickedNode];

    if (node == nil || node.missing)
        return;

    if (node.container && !_searching) {
        if ([_outline isItemExpanded:node])
            [_outline collapseItem:node];
        else
            [_outline expandItem:node];

        return;
    }

    if (node.path != nil && node.kind != app::BrowserItemKind::folder && self.onActivate != nil)
        self.onActivate(node.path);
}

- (void)menuNeedsUpdate:(NSMenu*)menu
{
    [menu removeAllItems];

    INCDAWBrowserNode* node = [self clickedNode];

    if (node != nil && node.path != nil && node.kind == app::BrowserItemKind::audio) {
        NSMenuItem* previewItem = [menu addItemWithTitle:@"Preview"
                                                  action:@selector(previewClicked:)
                                           keyEquivalent:@""];
        previewItem.target = self;

        NSMenuItem* stopItem = [menu addItemWithTitle:@"Stop Preview"
                                               action:@selector(stopPreviewClicked:)
                                        keyEquivalent:@""];
        stopItem.target = self;

        [menu addItem:[NSMenuItem separatorItem]];
    }

    if (node != nil && node.path != nil) {
        const bool favourite = _browser->isFavourite(pathOf(node.path));

        NSMenuItem* favouriteItem =
            [menu addItemWithTitle:favourite ? @"Remove from Favourites" : @"Add to Favourites"
                            action:@selector(toggleFavourite:)
                     keyEquivalent:@""];
        favouriteItem.target = self;

        NSMenuItem* revealItem = [menu addItemWithTitle:@"Reveal in Finder"
                                                 action:@selector(revealInFinder:)
                                          keyEquivalent:@""];
        revealItem.target = self;

        if (node.root) {
            NSMenuItem* removeItem = [menu addItemWithTitle:@"Remove Folder"
                                                     action:@selector(removeRoot:)
                                              keyEquivalent:@""];
            removeItem.target = self;
        }

        [menu addItem:[NSMenuItem separatorItem]];
    }

    NSMenuItem* addItem = [menu addItemWithTitle:@"Add Folder…"
                                          action:@selector(addRoot:)
                                   keyEquivalent:@""];
    addItem.target = self;

    NSMenuItem* refreshItem = [menu addItemWithTitle:@"Refresh"
                                              action:@selector(refresh:)
                                       keyEquivalent:@""];
    refreshItem.target = self;
}

- (void)previewClicked:(id)sender
{
    (void)sender;

    INCDAWBrowserNode* node = [self clickedNode];

    if (node != nil && !node.missing && node.path != nil && self.onPreview != nil)
        self.onPreview(node.path);
}

- (void)stopPreviewClicked:(id)sender
{
    (void)sender;

    if (self.onStopPreview != nil)
        self.onStopPreview();
}

- (void)toggleFavourite:(id)sender
{
    (void)sender;

    INCDAWBrowserNode* node = [self clickedNode];

    if (node == nil || node.path == nil)
        return;

    _browser->toggleFavourite(pathOf(node.path));
    [self reload];

    if (self.onSettingsChanged != nil)
        self.onSettingsChanged();
}

- (void)revealInFinder:(id)sender
{
    (void)sender;

    INCDAWBrowserNode* node = [self clickedNode];

    if (node == nil || node.path == nil)
        return;

    [[NSWorkspace sharedWorkspace]
        activateFileViewerSelectingURLs:@[ [NSURL fileURLWithPath:node.path] ]];
}

- (void)addRoot:(id)sender
{
    (void)sender;

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = NO;
    panel.canChooseDirectories    = YES;
    panel.allowsMultipleSelection = NO;
    panel.prompt                  = @"Add";

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    _browser->addRoot(std::string{}, pathOf(panel.URL.path));
    [self reload];

    if (self.onSettingsChanged != nil)
        self.onSettingsChanged();
}

- (void)removeRoot:(id)sender
{
    (void)sender;

    INCDAWBrowserNode* node = [self clickedNode];

    if (node == nil || node.path == nil || !node.root)
        return;

    _browser->removeRoot(pathOf(node.path));
    [self reload];

    if (self.onSettingsChanged != nil)
        self.onSettingsChanged();
}

- (void)refresh:(id)sender
{
    (void)sender;
    [self reload];
}

@end
