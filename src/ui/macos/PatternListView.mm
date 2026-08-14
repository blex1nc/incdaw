#include "ui/macos/PatternListView.h"

#include "app/CommandRegistry.h"
#include "app/commands/PatternCommands.h"
#include "project/Model.h"

#include <memory>
#include <string>
#include <vector>

using namespace incdaw;

namespace {

constexpr CGFloat rowHeight = 26.0;
constexpr CGFloat swatch    = 5.0;
constexpr CGFloat padding   = 8.0;

NSColor* grey(CGFloat white) { return [NSColor colorWithCalibratedWhite:white alpha:1.0]; }

NSColor* colourFrom(std::uint32_t argb)
{
    return [NSColor colorWithCalibratedRed:static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0
                                     green:static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0
                                      blue:static_cast<CGFloat>(argb & 0xFFu) / 255.0
                                     alpha:1.0];
}

void drawText(NSString* text, NSRect rect, NSColor* colour)
{
    NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
    style.lineBreakMode = NSLineBreakByTruncatingTail;

    [text drawInRect:rect
      withAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:12.0],
                       NSForegroundColorAttributeName: colour,
                       NSParagraphStyleAttributeName: style}];
}

} // namespace

@implementation INCDAWPatternListView {
    project::Project*     _project;
    app::CommandRegistry* _registry;
}

- (instancetype)initWithFrame:(NSRect)frame
                      project:(project::Project*)project
                     registry:(app::CommandRegistry*)registry
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _project  = project;
    _registry = registry;
    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (std::size_t)rowAtPoint:(NSPoint)point
{
    if (point.y < 0.0)
        return static_cast<std::size_t>(-1);

    return static_cast<std::size_t>(point.y / rowHeight);
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    [grey(0.115) setFill];
    NSRectFill(self.bounds);

    if (_project == nullptr)
        return;

    const std::vector<project::Pattern>& patterns = _project->patterns();
    const CGFloat width = self.bounds.size.width;

    for (std::size_t row = 0; row < patterns.size(); ++row) {
        const project::Pattern& pattern = patterns[row];
        const bool selected = pattern.id.value() == _selectedPatternIdValue;

        const NSRect rect = NSMakeRect(0, static_cast<CGFloat>(row) * rowHeight, width, rowHeight - 1.0);

        [(selected ? grey(0.22) : grey(0.16)) setFill];
        NSRectFill(rect);

        [colourFrom(pattern.colour) setFill];
        NSRectFill(NSMakeRect(0, rect.origin.y, swatch, rect.size.height));

        drawText(@(pattern.name.c_str()),
                 NSMakeRect(swatch + padding, rect.origin.y + 5.0,
                            width - swatch - padding * 2.0, rowHeight - 8.0),
                 selected ? grey(0.95) : grey(0.75));
    }

    const NSRect addRow = NSMakeRect(0, static_cast<CGFloat>(patterns.size()) * rowHeight,
                                     width, rowHeight - 1.0);
    [grey(0.135) setFill];
    NSRectFill(addRow);
    drawText(@"＋  New pattern",
             NSMakeRect(swatch + padding, addRow.origin.y + 5.0, width - swatch - padding, rowHeight - 8.0),
             grey(0.55));
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const std::size_t row = [self rowAtPoint:point];
    const std::size_t count = _project->patterns().size();

    if (row == count) {
        [self addPattern];
        return;
    }

    if (row > count)
        return;

    const project::EntityId patternId = _project->patterns()[row].id;

    if (event.clickCount == 2) {
        [self renamePattern:patternId];
        return;
    }

    if (self.onSelectPattern != nil)
        self.onSelectPattern(patternId.value());

    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const std::size_t row = [self rowAtPoint:point];

    if (row >= _project->patterns().size())
        return;

    const project::EntityId patternId = _project->patterns()[row].id;

    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* rename = [menu addItemWithTitle:@"Rename Pattern…"
                                         action:@selector(renameFromMenu:)
                                  keyEquivalent:@""];
    rename.target = self;
    rename.representedObject = @(patternId.value());

    NSMenuItem* duplicate = [menu addItemWithTitle:@"Duplicate Pattern"
                                            action:@selector(duplicateFromMenu:)
                                     keyEquivalent:@""];
    duplicate.target = self;
    duplicate.representedObject = @(patternId.value());

    // The last pattern is not removable: with none, there is nothing to edit
    // and nothing to play, and the editors would have to grow an empty state
    // that means "you deleted everything" rather than "this is empty".
    if (_project->patterns().size() > 1) {
        NSMenuItem* remove = [menu addItemWithTitle:@"Remove Pattern"
                                             action:@selector(removeFromMenu:)
                                      keyEquivalent:@""];
        remove.target = self;
        remove.representedObject = @(patternId.value());
    }

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)keyDown:(NSEvent*)event
{
    const unichar character = event.charactersIgnoringModifiers.length > 0
                                  ? [event.charactersIgnoringModifiers characterAtIndex:0]
                                  : 0;

    const bool command = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
    const bool shift   = (event.modifierFlags & NSEventModifierFlagShift) != 0;

    if (command && (character == 'z' || character == 'Z')) {
        if (shift)
            (void)_registry->redo();
        else
            (void)_registry->undo();

        [self changed];
        return;
    }

    if (character == ' ') {
        if (self.onTransportToggle != nil)
            self.onTransportToggle();

        return;
    }

    [super keyDown:event];
}

- (void)renameFromMenu:(NSMenuItem*)item
{
    [self renamePattern:project::EntityId{[item.representedObject unsignedLongLongValue]}];
}

- (void)duplicateFromMenu:(NSMenuItem*)item
{
    const project::EntityId source{[item.representedObject unsignedLongLongValue]};

    auto command = std::make_unique<app::DuplicatePatternCommand>(source, std::string{});
    app::DuplicatePatternCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    if (self.onSelectPattern != nil)
        self.onSelectPattern(raw->patternId().value());

    [self changed];
}

- (void)removeFromMenu:(NSMenuItem*)item
{
    const project::EntityId patternId{[item.representedObject unsignedLongLongValue]};

    if (!_registry->execute(std::make_unique<app::RemovePatternCommand>(patternId)))
        return;

    if (patternId.value() == _selectedPatternIdValue && !_project->patterns().empty()
        && self.onSelectPattern != nil)
        self.onSelectPattern(_project->patterns().front().id.value());

    [self changed];
}

- (void)addPattern
{
    auto command = std::make_unique<app::AddPatternCommand>(
        "Pattern " + std::to_string(_project->patterns().size() + 1));

    app::AddPatternCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    if (self.onSelectPattern != nil)
        self.onSelectPattern(raw->patternId().value());

    [self changed];
}

- (void)renamePattern:(project::EntityId)patternId
{
    const project::Pattern* pattern = _project->findPattern(patternId);
    if (pattern == nullptr)
        return;

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Rename pattern";
    [alert addButtonWithTitle:@"Rename"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 24)];
    field.stringValue = @(pattern->name.c_str());
    alert.accessoryView = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    if (_registry->execute(std::make_unique<app::RenamePatternCommand>(
            patternId, field.stringValue.UTF8String)))
        [self changed];
}

/// Sizes the document view to the list, so every pattern is reachable.
- (void)updateContentSize
{
    if (_project == nullptr)
        return;

    const CGFloat height = static_cast<CGFloat>(_project->patterns().size() + 1) * rowHeight;
    const CGFloat width  = self.enclosingScrollView != nil
                               ? self.enclosingScrollView.contentSize.width
                               : self.frame.size.width;

    if (self.frame.size.height != height || self.frame.size.width != width)
        [self setFrameSize:NSMakeSize(width, height)];
}

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    [self updateContentSize];
}

- (void)resizeWithOldSuperviewSize:(NSSize)size
{
    [super resizeWithOldSuperviewSize:size];
    [self updateContentSize];
}

- (void)changed
{
    [self updateContentSize];
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

- (void)setSelectedPatternIdValue:(unsigned long long)value
{
    _selectedPatternIdValue = value;
    [self setNeedsDisplay:YES];
}

@end
