#import "ui/macos/ChannelRackView.h"

#include "app/CommandRegistry.h"
#include "app/StepSequencerModel.h"
#include "app/commands/PatternCommands.h"
#include "project/Model.h"

#include <memory>
#include <string>

using namespace incdaw;

namespace {

constexpr double rowHeight     = 30.0;
constexpr double patternHeight = 24.0;
constexpr double headerHeight  = 22.0 + patternHeight;

/// Width of one pattern tab, in points.
constexpr double patternTabWidth = 104.0;
constexpr double nameWidth     = 168.0;
constexpr double buttonSize    = 16.0;
constexpr double stepSpacing   = 2.0;

/// Steps drawn per row. A pattern longer than this scrolls in the Piano Roll;
/// the rack shows the first bar, which is what a step sequencer is for.
constexpr int maximumSteps = 16;

NSColor* colourFromArgb(std::uint32_t argb, double alpha)
{
    const double red   = static_cast<double>((argb >> 16) & 0xFFu) / 255.0;
    const double green = static_cast<double>((argb >> 8) & 0xFFu) / 255.0;
    const double blue  = static_cast<double>(argb & 0xFFu) / 255.0;
    return [NSColor colorWithCalibratedRed:red green:green blue:blue alpha:alpha];
}

void fillRect(NSRect rect, NSColor* colour)
{
    [colour setFill];
    NSRectFill(rect);
}

void drawText(NSString* text, NSPoint origin, double size, NSColor* colour)
{
    NSDictionary* attributes = @{
        NSFontAttributeName            : [NSFont monospacedSystemFontOfSize:size weight:NSFontWeightRegular],
        NSForegroundColorAttributeName : colour,
    };
    [text drawAtPoint:origin withAttributes:attributes];
}

} // namespace

@implementation INCDAWChannelRackView {
    project::Project*     _project;
    app::CommandRegistry* _registry;
}

- (instancetype)initWithFrame:(NSRect)frame
                      project:(project::Project*)project
                     registry:(app::CommandRegistry*)registry
{
    if ((self = [super initWithFrame:frame]) != nil) {
        _project      = project;
        _registry     = registry;
        _playheadTick = -1;
    }

    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return NO; }

// ── Geometry ──────────────────────────────────────────────────────────────────
//
// One place decides where a row and a step live, and both drawing and hit
// testing read it. Two copies of this arithmetic is how a UI ends up drawing a
// button somewhere the click does not land.

- (double)stepWidth
{
    const double available = self.bounds.size.width - nameWidth - 8.0;
    return (available - stepSpacing * (maximumSteps - 1)) / maximumSteps;
}

- (NSRect)patternTabRect:(std::size_t)index
{
    return NSMakeRect(8.0 + static_cast<double>(index) * (patternTabWidth + 4.0),
                      22.0 + 2.0, patternTabWidth, patternHeight - 5.0);
}

/// The tab past the last pattern adds one, the same way the row past the last
/// channel adds a channel.
- (NSRect)addPatternRect
{
    return [self patternTabRect:_project->patterns().size()];
}

- (NSRect)rowRect:(std::size_t)row
{
    return NSMakeRect(0.0, headerHeight + static_cast<double>(row) * rowHeight,
                      self.bounds.size.width, rowHeight);
}

- (NSRect)stepRect:(int)step inRow:(std::size_t)row
{
    const NSRect bounds = [self rowRect:row];
    const double width  = [self stepWidth];

    return NSMakeRect(nameWidth + static_cast<double>(step) * (width + stepSpacing),
                      bounds.origin.y + 5.0, width, rowHeight - 10.0);
}

- (NSRect)muteRectInRow:(std::size_t)row
{
    const NSRect bounds = [self rowRect:row];
    return NSMakeRect(8.0, bounds.origin.y + (rowHeight - buttonSize) * 0.5, buttonSize, buttonSize);
}

- (NSRect)soloRectInRow:(std::size_t)row
{
    NSRect rect = [self muteRectInRow:row];
    rect.origin.x += buttonSize + 4.0;
    return rect;
}

- (std::size_t)rowAtPoint:(NSPoint)point
{
    if (point.y < headerHeight)
        return static_cast<std::size_t>(-1);

    const auto row = static_cast<std::size_t>((point.y - headerHeight) / rowHeight);
    return row < _project->channels().size() ? row : static_cast<std::size_t>(-1);
}

/// Height the rack wants, so the window can lay it out without guessing.
+ (double)heightForChannelCount:(std::size_t)count
{
    return headerHeight + static_cast<double>(count + 1) * rowHeight;
}

- (void)selectPattern:(project::EntityId)pattern
{
    _patternIdValue = pattern.value();
    [self setNeedsDisplay:YES];

    if (self.onPatternSelected != nil)
        self.onPatternSelected();
}

// ── Drawing ───────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    fillRect(self.bounds, [NSColor colorWithCalibratedWhite:0.13 alpha:1.0]);

    const app::StepSequencerModel grid{*_project, project::EntityId{_patternIdValue}};
    const auto& channels = _project->channels();

    [self drawHeaderWithGrid:grid];

    for (std::size_t row = 0; row < channels.size(); ++row)
        [self drawRow:row grid:grid];

    [self drawAddRowAt:channels.size()];
}

- (void)drawHeaderWithGrid:(const app::StepSequencerModel&)grid
{
    const NSRect header = NSMakeRect(0.0, 0.0, self.bounds.size.width, headerHeight);
    fillRect(header, [NSColor colorWithCalibratedWhite:0.17 alpha:1.0]);

    drawText(@"CHANNEL RACK", NSMakePoint(8.0, 5.0), 10.0,
             [NSColor colorWithCalibratedWhite:0.55 alpha:1.0]);

    // Pattern tabs. A pattern-based DAW that can only ever edit one pattern is
    // a pattern editor, so this is not decoration.
    const auto& patterns = _project->patterns();

    for (std::size_t index = 0; index < patterns.size(); ++index) {
        const NSRect rect     = [self patternTabRect:index];
        const bool   selected = patterns[index].id.value() == _patternIdValue;

        fillRect(rect, selected ? colourFromArgb(patterns[index].colour, 0.85)
                                : [NSColor colorWithCalibratedWhite:0.20 alpha:1.0]);

        drawText([NSString stringWithUTF8String:patterns[index].name.c_str()],
                 NSMakePoint(rect.origin.x + 6.0, rect.origin.y + 4.0), 10.0,
                 selected ? [NSColor colorWithCalibratedWhite:0.05 alpha:1.0]
                          : [NSColor colorWithCalibratedWhite:0.70 alpha:1.0]);
    }

    const NSRect addRect = [self addPatternRect];
    fillRect(addRect, [NSColor colorWithCalibratedWhite:0.16 alpha:1.0]);
    drawText(@"+ pattern", NSMakePoint(addRect.origin.x + 6.0, addRect.origin.y + 4.0), 10.0,
             [NSColor colorWithCalibratedWhite:0.42 alpha:1.0]);

    // Beat markers over the step columns, so the grid reads as bars rather than
    // sixteen identical boxes.
    const double width = [self stepWidth];

    for (int step = 0; step < maximumSteps; step += 4) {
        const double x = nameWidth + static_cast<double>(step) * (width + stepSpacing);
        drawText([NSString stringWithFormat:@"%d", step / 4 + 1], NSMakePoint(x + 2.0, 5.0), 9.0,
                 [NSColor colorWithCalibratedWhite:0.45 alpha:1.0]);
    }
}

- (void)drawRow:(std::size_t)row grid:(const app::StepSequencerModel&)grid
{
    const project::Channel& channel = _project->channels()[row];
    const bool selected = channel.id.value() == _selectedChannelIdValue;

    const NSRect bounds = [self rowRect:row];
    fillRect(bounds, [NSColor colorWithCalibratedWhite:(row % 2 == 0 ? 0.15 : 0.14) alpha:1.0]);

    if (selected)
        fillRect(NSMakeRect(0.0, bounds.origin.y, 3.0, rowHeight), colourFromArgb(channel.colour, 1.0));

    // Mute and solo. Solo wins visually when it is on, because that is what is
    // actually deciding whether this channel is heard.
    const bool anySoloed = _project->anyChannelSoloed();
    const bool audible   = !channel.muted && (!anySoloed || channel.soloed);

    fillRect([self muteRectInRow:row],
             audible ? [NSColor colorWithCalibratedRed:0.35 green:0.75 blue:0.45 alpha:1.0]
                     : [NSColor colorWithCalibratedWhite:0.28 alpha:1.0]);

    fillRect([self soloRectInRow:row],
             channel.soloed ? [NSColor colorWithCalibratedRed:0.90 green:0.75 blue:0.30 alpha:1.0]
                            : [NSColor colorWithCalibratedWhite:0.24 alpha:1.0]);

    drawText([NSString stringWithUTF8String:channel.name.c_str()],
             NSMakePoint(52.0, bounds.origin.y + 8.0), 11.0,
             selected ? [NSColor colorWithCalibratedWhite:0.95 alpha:1.0]
                      : [NSColor colorWithCalibratedWhite:0.70 alpha:1.0]);

    const int steps       = grid.isValid() ? grid.stepCount(channel.id) : 0;
    const int cursorStep  = _playheadTick >= 0 && grid.isValid()
                                ? grid.stepForTick(static_cast<engine::Tick>(_playheadTick))
                                : -1;

    for (int step = 0; step < maximumSteps; ++step) {
        const NSRect rect = [self stepRect:step inRow:row];

        // A channel that loops shorter than the pattern greys out the steps it
        // does not reach — that is what polymetry looks like.
        const bool inRange = step < steps;
        const bool on      = inRange && grid.isStepOn(channel.id, step);

        NSColor* colour = nil;

        if (on) {
            const double velocity = static_cast<double>(grid.velocityAt(channel.id, step)) / 127.0;
            colour = colourFromArgb(channel.colour, 0.35 + velocity * 0.65);
        } else if (!inRange) {
            colour = [NSColor colorWithCalibratedWhite:0.11 alpha:1.0];
        } else {
            colour = [NSColor colorWithCalibratedWhite:(step % 4 == 0 ? 0.24 : 0.19) alpha:1.0];
        }

        fillRect(rect, colour);

        if (cursorStep >= 0 && (steps > 0 && cursorStep % steps == step))
            fillRect(NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, 2.0),
                     [NSColor colorWithCalibratedWhite:0.95 alpha:0.8]);
    }

    // Notes the grid cannot represent must be admitted, not hidden.
    if (grid.isValid()) {
        const std::size_t offGrid = grid.offGridNoteCount(channel.id);

        if (offGrid > 0)
            drawText([NSString stringWithFormat:@"+%lu", static_cast<unsigned long>(offGrid)],
                     NSMakePoint(nameWidth - 26.0, bounds.origin.y + 9.0), 9.0,
                     [NSColor colorWithCalibratedWhite:0.45 alpha:1.0]);
    }
}

- (void)drawAddRowAt:(std::size_t)row
{
    const NSRect bounds = [self rowRect:row];
    fillRect(bounds, [NSColor colorWithCalibratedWhite:0.12 alpha:1.0]);
    drawText(@"+ add channel", NSMakePoint(52.0, bounds.origin.y + 8.0), 11.0,
             [NSColor colorWithCalibratedWhite:0.42 alpha:1.0]);
}

// ── Input ─────────────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const auto&   channels = _project->channels();

    if (point.y < headerHeight) {
        [self handlePatternClickAt:point
                         duplicate:(event.modifierFlags & NSEventModifierFlagShift) != 0];
        return;
    }

    // The row past the last channel adds one.
    if (point.y >= headerHeight) {
        const auto row = static_cast<std::size_t>((point.y - headerHeight) / rowHeight);

        if (row == channels.size()) {
            [self addChannel];
            return;
        }
    }

    const std::size_t row = [self rowAtPoint:point];
    if (row == static_cast<std::size_t>(-1))
        return;

    const project::EntityId channel = channels[row].id;

    if (NSPointInRect(point, [self muteRectInRow:row])) {
        [self setFlag:app::SetChannelFlagCommand::Flag::muted
           onChannel:channel
                  to:!channels[row].muted];
        return;
    }

    if (NSPointInRect(point, [self soloRectInRow:row])) {
        [self setFlag:app::SetChannelFlagCommand::Flag::soloed
           onChannel:channel
                  to:!channels[row].soloed];
        return;
    }

    if (point.x < nameWidth) {
        _selectedChannelIdValue = channel.value();
        [self setNeedsDisplay:YES];

        if (self.onChannelSelected != nil)
            self.onChannelSelected();

        return;
    }

    [self toggleStepAt:point inRow:row channel:channel];
}

- (void)handlePatternClickAt:(NSPoint)point duplicate:(bool)duplicate
{
    const auto& patterns = _project->patterns();

    for (std::size_t index = 0; index < patterns.size(); ++index) {
        if (!NSPointInRect(point, [self patternTabRect:index]))
            continue;

        if (!duplicate) {
            [self selectPattern:patterns[index].id];
            return;
        }

        auto command = std::make_unique<app::DuplicatePatternCommand>(patterns[index].id);
        app::DuplicatePatternCommand* pointer = command.get();

        if (_registry->execute(std::move(command)))
            [self selectPattern:pointer->createdPattern()];

        return;
    }

    if (!NSPointInRect(point, [self addPatternRect]))
        return;

    const std::string name = "Pattern " + std::to_string(patterns.size() + 1);

    auto command = std::make_unique<app::AddPatternCommand>(name);
    app::AddPatternCommand* pointer = command.get();

    if (_registry->execute(std::move(command)))
        [self selectPattern:pointer->createdPattern()];
}

- (void)rightMouseDown:(NSEvent*)event
{
    // Right-click on a step clears it; on a name, deletes the channel. Both go
    // through commands, so both are undoable.
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    if (point.y < headerHeight) {
        [self deletePatternAt:point];
        return;
    }

    const std::size_t row = [self rowAtPoint:point];

    if (row == static_cast<std::size_t>(-1))
        return;

    const project::EntityId channel = _project->channels()[row].id;

    if (point.x < nameWidth) {
        if (_registry->execute(std::make_unique<app::DeleteChannelCommand>(channel)))
            [self notifyChanged];

        return;
    }

    [self toggleStepAt:point inRow:row channel:channel];
}

- (void)deletePatternAt:(NSPoint)point
{
    const auto& patterns = _project->patterns();

    // The last pattern stays. A project with no patterns has nothing for the
    // Piano Roll to show and no obvious way back.
    if (patterns.size() <= 1)
        return;

    for (std::size_t index = 0; index < patterns.size(); ++index) {
        if (!NSPointInRect(point, [self patternTabRect:index]))
            continue;

        const project::EntityId doomed = patterns[index].id;

        if (!_registry->execute(std::make_unique<app::DeletePatternCommand>(doomed)))
            return;

        if (_patternIdValue == doomed.value() && !_project->patterns().empty())
            [self selectPattern:_project->patterns().front().id];
        else
            [self notifyChanged];

        return;
    }
}

- (void)toggleStepAt:(NSPoint)point inRow:(std::size_t)row channel:(project::EntityId)channel
{
    const app::StepSequencerModel grid{*_project, project::EntityId{_patternIdValue}};
    if (!grid.isValid())
        return;

    for (int step = 0; step < maximumSteps; ++step) {
        if (!NSPointInRect(point, [self stepRect:step inRow:row]))
            continue;

        if (step >= grid.stepCount(channel))
            return;   // outside this channel's loop; the click means nothing

        // The key follows the channel's row so that a drum-style rack lands each
        // channel on its own note rather than stacking every channel on C4.
        const int key = 36 + static_cast<int>(row);

        if (_registry->execute(std::make_unique<app::ToggleStepCommand>(
                project::EntityId{_patternIdValue}, channel, step, 100, key)))
            [self notifyChanged];

        return;
    }
}

- (void)setFlag:(app::SetChannelFlagCommand::Flag)flag
      onChannel:(project::EntityId)channel
             to:(bool)value
{
    if (_registry->execute(std::make_unique<app::SetChannelFlagCommand>(channel, flag, value)))
        [self notifyChanged];
}

- (void)addChannel
{
    const std::string name = "Channel " + std::to_string(_project->channels().size() + 1);

    if (_registry->execute(std::make_unique<app::AddChannelCommand>(name)))
        [self notifyChanged];
}

- (void)notifyChanged
{
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

@end
