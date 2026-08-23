#include "ui/macos/PatternListView.h"

#include "app/CommandRegistry.h"
#include "app/commands/PatternCommands.h"
#include "project/Model.h"
#include "ui/macos/Theme.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;

constexpr CGFloat rowHeight = 30.0;
constexpr CGFloat swatch    = 4.0;
constexpr CGFloat padding   = 8.0;

/// Bars a new length is offered in, from the pattern menu.
constexpr int lengthChoices[] = {1, 2, 4, 8, 16};

} // namespace

@implementation INCDAWPatternListView {
    project::Project*     _project;
    app::CommandRegistry* _registry;
}

/// How many times a pattern is placed in the arrangement.
///
/// The picker's job is to show which patterns are IN the song: a list where a
/// used pattern and an abandoned sketch look the same is a list that has to be
/// checked against the playlist by hand.
- (std::size_t)placementsOf:(project::EntityId)pattern
{
    std::size_t count = 0;

    for (const project::Clip& clip : _project->clips())
        if (clip.type == project::ClipType::pattern && clip.source == pattern)
            ++count;

    return count;
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
    // 26 points of section heading sit above the first row.
    const CGFloat y = point.y - 26.0;
    if (y < 0.0)
        return static_cast<std::size_t>(-1);

    return static_cast<std::size_t>(y / rowHeight);
}

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    if (_project == nullptr)
        return;

    const std::vector<project::Pattern>& patterns = _project->patterns();
    const CGFloat width = self.bounds.size.width;

    theme::drawText(@"PATTERNS", NSMakeRect(padding, 8.0, width - padding * 2.0, 14.0),
                    theme::ink(Ink::textDim), theme::labelFont(9.5, NSFontWeightBold));

    const CGFloat top = 26.0;

    for (std::size_t row = 0; row < patterns.size(); ++row) {
        const project::Pattern& pattern = patterns[row];
        const bool selected = pattern.id.value() == _selectedPatternIdValue;

        const NSRect rect = NSMakeRect(4.0, top + static_cast<CGFloat>(row) * rowHeight,
                                       width - 8.0, rowHeight - 3.0);

        NSColor* colour = theme::fromArgb(pattern.colour);

        if (selected) {
            theme::fillGradient(rect, theme::metrics::radiusControl,
                                theme::mix(theme::ink(Ink::rowSelected), colour, 0.20),
                                theme::ink(Ink::rowSelected), true);
            theme::strokeRounded(rect, theme::metrics::radiusControl,
                                 theme::withAlpha(theme::ink(Ink::accent), 0.8));
        } else {
            theme::fillRounded(rect, theme::metrics::radiusControl, theme::ink(Ink::rowOdd));
        }

        // The colour spine: enough to identify the pattern in the playlist by
        // eye, not enough to become the row itself.
        theme::fillRounded(NSMakeRect(NSMinX(rect) + 4.0, NSMinY(rect) + 5.0, swatch,
                                      rect.size.height - 10.0),
                           swatch / 2.0, colour);

        const std::size_t placements = [self placementsOf:pattern.id];

        // The count sits at the trailing edge, so the name keeps the room it
        // had and an unused pattern is legible by what is missing.
        const CGFloat countWidth = 34.0;

        theme::drawTextCentred(@(pattern.name.c_str()),
                               NSMakeRect(NSMinX(rect) + swatch + padding + 2.0, NSMinY(rect),
                                          rect.size.width - swatch - padding * 2.0 - countWidth,
                                          rect.size.height),
                               selected ? theme::ink(Ink::textPrimary)
                                        : theme::ink(Ink::textSecondary),
                               theme::labelFont(12.0, selected ? NSFontWeightSemibold
                                                               : NSFontWeightRegular));

        if (placements > 0)
            theme::drawTextCentred([NSString stringWithFormat:@"×%lu",
                                                              static_cast<unsigned long>(placements)],
                                   NSMakeRect(NSMaxX(rect) - countWidth, NSMinY(rect),
                                              countWidth - 4.0, rect.size.height),
                                   theme::ink(Ink::accent), theme::labelFont(10.0,
                                                                             NSFontWeightSemibold));
        else
            theme::drawTextCentred(@"—",
                                   NSMakeRect(NSMaxX(rect) - countWidth, NSMinY(rect),
                                              countWidth - 4.0, rect.size.height),
                                   theme::withAlpha(theme::ink(Ink::textDim), 0.6),
                                   theme::labelFont(10.0));
    }

    const NSRect addRow = NSMakeRect(4.0, top + static_cast<CGFloat>(patterns.size()) * rowHeight,
                                     width - 8.0, rowHeight - 3.0);

    theme::strokeRounded(addRow, theme::metrics::radiusControl,
                         theme::withAlpha(theme::ink(Ink::textDim), 0.35));

    theme::drawTextCentred(@"＋  New pattern",
                           NSMakeRect(NSMinX(addRow) + swatch + padding + 2.0, NSMinY(addRow),
                                      addRow.size.width - padding, addRow.size.height),
                           theme::ink(Ink::textSecondary), theme::labelFont(12.0));
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

    NSMenuItem* colour = [menu addItemWithTitle:@"Colour" action:nil keyEquivalent:@""];
    colour.submenu = [self colourMenuFor:patternId];

    NSMenuItem* length = [menu addItemWithTitle:@"Length" action:nil keyEquivalent:@""];
    length.submenu = [self lengthMenuFor:patternId];

    [menu addItem:[NSMenuItem separatorItem]];

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

/// The same nine hues the playlist offers a track, so a pattern and the track
/// it is placed on can be given matching colours without a colour panel.
- (NSMenu*)colourMenuFor:(project::EntityId)patternId
{
    static const struct { const char* name; unsigned int argb; } swatches[] = {
        {"Blue",   0xFF6699CCu}, {"Teal",   0xFF3E9E96u}, {"Green",  0xFF5C9E4Au},
        {"Amber",  0xFFC8963Cu}, {"Orange", 0xFFC86E3Cu}, {"Red",    0xFFBE4A4Au},
        {"Pink",   0xFFB4569Eu}, {"Violet", 0xFF7E5CBEu}, {"Grey",   0xFF6E6E78u},
    };

    NSMenu* menu = [[NSMenu alloc] init];

    for (const auto& entry : swatches) {
        NSMenuItem* item = [menu addItemWithTitle:@(entry.name)
                                           action:@selector(setColourFromMenu:)
                                    keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @[@(patternId.value()), @(entry.argb)];
    }

    return menu;
}

- (NSMenu*)lengthMenuFor:(project::EntityId)patternId
{
    NSMenu* menu = [[NSMenu alloc] init];

    const project::Pattern* pattern = _project->findPattern(patternId);

    for (const int bars : lengthChoices) {
        NSMenuItem* item =
            [menu addItemWithTitle:[NSString stringWithFormat:@"%d bar%s", bars,
                                                              bars == 1 ? "" : "s"]
                            action:@selector(setLengthFromMenu:)
                     keyEquivalent:@""];
        item.target            = self;
        item.representedObject = @[@(patternId.value()), @(bars)];

        if (pattern != nullptr
            && pattern->length == engine::ticksPerQuarterNote * 4 * bars)
            item.state = NSControlStateValueOn;
    }

    return menu;
}

- (void)setColourFromMenu:(NSMenuItem*)item
{
    NSArray* pair = item.representedObject;
    if (pair.count != 2)
        return;

    const project::EntityId pattern{[pair[0] unsignedLongLongValue]};
    const auto colour = static_cast<std::uint32_t>([pair[1] unsignedIntValue]);

    if (_registry->execute(std::make_unique<app::SetPatternColourCommand>(pattern, colour)))
        [self changed];
}

- (void)setLengthFromMenu:(NSMenuItem*)item
{
    NSArray* pair = item.representedObject;
    if (pair.count != 2)
        return;

    const project::EntityId pattern{[pair[0] unsignedLongLongValue]};
    const auto bars = static_cast<engine::Tick>([pair[1] intValue]);

    if (_registry->execute(std::make_unique<app::SetPatternLengthCommand>(
            pattern, engine::ticksPerQuarterNote * 4 * bars)))
        [self changed];
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
