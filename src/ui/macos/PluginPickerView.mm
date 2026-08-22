#include "ui/macos/PluginPickerView.h"

#include "app/PluginPickerModel.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

using namespace incdaw;

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;

constexpr CGFloat searchHeight = 24.0;
constexpr CGFloat padding      = 6.0;

/// A drag begins only after the pointer has actually travelled: without this a
/// click that wobbles by a pixel starts a drag instead of choosing a row.
constexpr CGFloat dragThreshold = 4.0;

} // namespace

NSPasteboardType const INCDAWPluginPasteboardType = @"com.incdaw.plugin";

@interface INCDAWPluginPickerView () <NSDraggingSource>
@end

@implementation INCDAWPluginPickerView {
    std::unique_ptr<app::PluginPickerModel> _model;

    NSSearchField* _search;
    CGFloat        _scroll;

    NSPoint _mouseDownAt;
    bool    _mouseDownOnRow;
}

- (instancetype)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _model = std::make_unique<app::PluginPickerModel>();
    _model->addBuiltinEffects();
    _scroll = 0.0;

    _search = [[NSSearchField alloc]
        initWithFrame:NSMakeRect(padding, padding, frame.size.width - padding * 2.0,
                                 searchHeight)];
    _search.placeholderString = @"Search plugins";
    _search.font              = theme::labelFont(11.0, NSFontWeightRegular);
    _search.target            = self;
    _search.action            = @selector(searchChanged:);
    _search.autoresizingMask  = NSViewWidthSizable | NSViewMinYMargin;
    _search.sendsSearchStringImmediately = YES;
    _search.sendsWholeSearchString       = NO;
    [self addSubview:_search];

    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)focusSearch
{
    [self.window makeFirstResponder:_search];
}

- (void)setHostedPlugins:(NSArray<NSDictionary*>*)hostedPlugins
{
    _hostedPlugins = [hostedPlugins copy];

    // Rebuild from scratch: the scan can change under us (a rescan, a
    // blacklist edit), and merging would leave plugins the registry has
    // forgotten still on offer.
    _model->clear();
    _model->addBuiltinEffects();

    for (NSDictionary* plugin in _hostedPlugins) {
        NSString* identifier = plugin[@"id"];
        NSString* name       = plugin[@"name"];
        if (identifier == nil)
            continue;

        plugins::PluginIdentifier parsed;
        if (!plugins::PluginIdentifier::fromString(identifier.UTF8String, parsed))
            continue;

        _model->addHosted(parsed, name != nil ? name.UTF8String : "");
    }

    _model->setSearch(_search.stringValue.UTF8String);
    [self setNeedsDisplay:YES];
}

// ── Geometry ─────────────────────────────────────────────────────────────────

- (NSRect)listRect
{
    const CGFloat top = padding * 2.0 + searchHeight;
    return NSMakeRect(padding, top, self.bounds.size.width - padding * 2.0,
                      std::max<CGFloat>(0.0, self.bounds.size.height - top - padding));
}

- (NSRect)rowRect:(std::size_t)row
{
    const NSRect list = [self listRect];
    const auto   height = static_cast<CGFloat>(app::PluginPickerModel::rowHeight);

    return NSMakeRect(NSMinX(list), NSMinY(list) + static_cast<CGFloat>(row) * height - _scroll,
                      list.size.width, height);
}

/// Keeps the scroll offset inside what there is to scroll. Called after every
/// change to the filter as well as after a wheel: a search that shortens the
/// list must not leave the view parked past its end.
- (void)clampScroll
{
    const CGFloat visible = [self listRect].size.height;
    const auto content    = static_cast<CGFloat>(_model->contentHeight());
    const CGFloat maximum = std::max<CGFloat>(0.0, content - visible);

    _scroll = std::clamp(_scroll, static_cast<CGFloat>(0.0), maximum);
}

/// Scrolls the highlight into view — arrow keys are useless if the row they
/// move to is off the bottom.
- (void)revealHighlight
{
    const std::size_t row = _model->highlight();
    if (row == app::PluginPickerModel::noRow)
        return;

    const auto height  = static_cast<CGFloat>(app::PluginPickerModel::rowHeight);
    const CGFloat top    = static_cast<CGFloat>(row) * height;
    const CGFloat bottom = top + height;
    const CGFloat visible = [self listRect].size.height;

    if (top < _scroll)
        _scroll = top;
    else if (bottom > _scroll + visible)
        _scroll = bottom - visible;

    [self clampScroll];
}

// ── Search ───────────────────────────────────────────────────────────────────

- (void)searchChanged:(id)sender
{
    (void)sender;
    _model->setSearch(_search.stringValue.UTF8String);
    _scroll = 0.0;
    [self setNeedsDisplay:YES];
}

// ── Choosing ─────────────────────────────────────────────────────────────────

- (void)chooseRow:(std::size_t)row
{
    const app::PluginPickerEntry* entry = _model->entryAtRow(row);
    if (entry == nullptr || self.onChoose == nil)
        return;

    self.onChoose(@(entry->plugin.toString().c_str()));
}

- (void)keyDown:(NSEvent*)event
{
    const unichar key = event.charactersIgnoringModifiers.length > 0
                            ? [event.charactersIgnoringModifiers characterAtIndex:0]
                            : 0;

    if (key == NSDownArrowFunctionKey || key == NSUpArrowFunctionKey) {
        _model->moveHighlight(key == NSDownArrowFunctionKey ? 1 : -1);
        [self revealHighlight];
        [self setNeedsDisplay:YES];
        return;
    }

    if (key == NSCarriageReturnCharacter || key == NSEnterCharacter) {
        [self chooseRow:_model->highlight()];
        return;
    }

    [super keyDown:event];
}

- (void)scrollWheel:(NSEvent*)event
{
    _scroll -= event.scrollingDeltaY;
    [self clampScroll];
    [self setNeedsDisplay:YES];
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const NSRect  list  = [self listRect];

    _mouseDownAt    = point;
    _mouseDownOnRow = false;

    if (!NSPointInRect(point, list))
        return;

    const std::size_t row = _model->rowAtY(
        static_cast<double>(point.y - NSMinY(list) + _scroll));

    if (row == app::PluginPickerModel::noRow || _model->entryAtRow(row) == nullptr)
        return;

    _model->setHighlight(row);
    _mouseDownOnRow = true;
    [self setNeedsDisplay:YES];

    // Double-click inserts. A single click selects, so that a drag can start
    // from the row the user just pressed.
    if (event.clickCount >= 2)
        [self chooseRow:row];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (!_mouseDownOnRow)
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    if (std::abs(point.x - _mouseDownAt.x) < dragThreshold
        && std::abs(point.y - _mouseDownAt.y) < dragThreshold)
        return;

    const app::PluginPickerEntry* entry = _model->highlightedEntry();
    if (entry == nullptr)
        return;

    _mouseDownOnRow = false;

    NSPasteboardItem* item = [[NSPasteboardItem alloc] init];
    [item setString:@(entry->plugin.toString().c_str())
            forType:INCDAWPluginPasteboardType];

    NSDraggingItem* dragged = [[NSDraggingItem alloc] initWithPasteboardWriter:item];

    const NSRect row = [self rowRect:_model->highlight()];
    NSImage* image   = [[NSImage alloc] initWithSize:row.size];

    [image lockFocusFlipped:YES];
    theme::fillRounded(NSMakeRect(0, 0, row.size.width, row.size.height),
                       theme::metrics::radiusPad, theme::ink(Ink::accent));
    theme::drawTextCentred(@(entry->name.c_str()),
                           NSMakeRect(6.0, 0, row.size.width - 12.0, row.size.height),
                           theme::ink(Ink::textOnAccent), theme::labelFont(11.0));
    [image unlockFocus];

    [dragged setDraggingFrame:row contents:image];

    [self beginDraggingSessionWithItems:@[ dragged ] event:event source:self];
}

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
    (void)session;

    // Inside the application only: a plugin choice is meaningless anywhere
    // else, and copy is what an insert is — the catalogue keeps its entry.
    return context == NSDraggingContextWithinApplication ? NSDragOperationCopy
                                                         : NSDragOperationNone;
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    const NSRect list = [self listRect];
    theme::drawWell(list, YES);

    [NSGraphicsContext saveGraphicsState];
    [NSBezierPath clipRect:list];

    const auto height = static_cast<CGFloat>(app::PluginPickerModel::rowHeight);

    for (std::size_t row = 0; row < _model->rowCount(); ++row) {
        const NSRect rect = [self rowRect:row];

        // Rows outside the well are not drawn at all: the list is long and the
        // clip alone would still cost a fill and a text layout each.
        if (NSMaxY(rect) < NSMinY(list) || NSMinY(rect) > NSMaxY(list))
            continue;

        const app::PluginPickerModel::Row& entry = _model->rows()[row];

        if (entry.header) {
            theme::drawTextCentred(@(entry.text.c_str()),
                                   NSInsetRect(rect, 8.0, 0.0),
                                   theme::ink(Ink::textDim),
                                   theme::labelFont(9.5, NSFontWeightBold));
            continue;
        }

        const bool highlighted = row == _model->highlight();

        if (highlighted)
            theme::fillRounded(NSInsetRect(rect, 2.0, 1.0), theme::metrics::radiusPad,
                               theme::ink(Ink::accent));

        theme::drawTextCentred(@(entry.text.c_str()),
                               NSMakeRect(NSMinX(rect) + 12.0, NSMinY(rect),
                                          rect.size.width - 18.0, height),
                               highlighted ? theme::ink(Ink::textOnAccent)
                                           : theme::ink(Ink::textPrimary),
                               theme::labelFont(11.0));
    }

    [NSGraphicsContext restoreGraphicsState];

    if (_model->rowCount() == 0)
        theme::drawTextCentred(@"Nothing matches", list, theme::ink(Ink::textDim),
                               theme::labelFont(11.0), theme::Align::centre);
}

@end
