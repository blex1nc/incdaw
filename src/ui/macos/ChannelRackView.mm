#include "ui/macos/ChannelRackView.h"

#include "app/ChannelRackModel.h"
#include "app/CommandRegistry.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/StepCommands.h"
#include "project/Model.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;

namespace {

NSColor* colourFrom(std::uint32_t argb, CGFloat brightness = 1.0)
{
    const CGFloat red   = static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0;
    const CGFloat green = static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0;
    const CGFloat blue  = static_cast<CGFloat>(argb & 0xFFu) / 255.0;

    return [NSColor colorWithCalibratedRed:red * brightness
                                     green:green * brightness
                                      blue:blue * brightness
                                     alpha:1.0];
}

NSColor* grey(CGFloat white) { return [NSColor colorWithCalibratedWhite:white alpha:1.0]; }

void fill(app::ChannelRackModel::Rect rect, NSColor* colour)
{
    [colour setFill];
    NSRectFill(NSMakeRect(rect.x, rect.y, rect.width, rect.height));
}

void drawText(NSString* text, app::ChannelRackModel::Rect rect, NSColor* colour, CGFloat size,
              BOOL centred = NO)
{
    NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
    style.lineBreakMode = NSLineBreakByTruncatingTail;
    style.alignment     = centred ? NSTextAlignmentCenter : NSTextAlignmentLeft;

    [text drawInRect:NSMakeRect(rect.x, rect.y, rect.width, rect.height)
      withAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:size],
                       NSForegroundColorAttributeName: colour,
                       NSParagraphStyleAttributeName: style}];
}

/// What a drag started on, so that dragging keeps doing the same thing even
/// when the cursor leaves the control it began in.
enum class RackDrag { none, volume, paintSteps };

} // namespace

@implementation INCDAWChannelRackView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::ChannelRackModel> _model;

    RackDrag          _drag;
    project::EntityId _dragChannel;
    BOOL              _paintOn;
    int               _lastPaintedStep;
}

- (instancetype)initWithFrame:(NSRect)frame
                      project:(project::Project*)project
                     registry:(app::CommandRegistry*)registry
{
    self = [super initWithFrame:frame];
    if (self == nil)
        return nil;

    _project      = project;
    _registry     = registry;
    _model        = std::make_unique<app::ChannelRackModel>();
    _drag         = RackDrag::none;
    _playheadTick = -1;
    _lastPaintedStep = -1;

    return self;
}

// Flipped, so the view agrees with app::ChannelRackModel instead of converting
// at every call — the same choice the Piano Roll makes.
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (const project::Pattern*)currentPattern
{
    return _project != nullptr ? _project->findPattern(project::EntityId{_patternIdValue}) : nullptr;
}

- (std::size_t)channelCount
{
    return _project != nullptr ? _project->channels().size() : 0;
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    [grey(0.10) setFill];
    NSRectFill(self.bounds);

    if (_project == nullptr)
        return;

    const project::Pattern* pattern = [self currentPattern];
    const auto& layout = _model->layout();

    // Only the steps that fit are drawn; a 512-step pattern must not cost 512
    // rectangles per row per frame.
    const int steps = pattern != nullptr ? _model->stepCount(*pattern) : 0;
    const int visible = _model->visibleStepCount(self.bounds.size.width - layout.headerWidth) + 1;
    const int lastStep = std::min(steps, _model->firstStep() + visible);

    const long long playheadStep = _playheadTick >= 0 && _model->stepTicks() > 0
                                       ? _playheadTick / _model->stepTicks()
                                       : -1;

    const std::vector<project::Channel>& channels = _project->channels();

    for (std::size_t row = 0; row < channels.size(); ++row) {
        const project::Channel& channel = channels[row];
        const bool selected = channel.id.value() == _selectedChannelIdValue;

        fill(_model->rowRect(row), selected ? grey(0.20) : grey(0.145));
        fill(_model->swatchRect(row), colourFrom(channel.colour, channel.muted ? 0.4 : 1.0));

        drawText(@(channel.name.c_str()), _model->nameRect(row),
                 channel.muted ? grey(0.45) : grey(0.88), 12.0);

        const auto mute = _model->muteRect(row);
        fill(mute, channel.muted ? [NSColor colorWithCalibratedRed:0.75 green:0.30 blue:0.25 alpha:1.0]
                                 : grey(0.24));
        drawText(@"M", {mute.x, mute.y + 2.0, mute.width, mute.height}, grey(0.9), 10.0, YES);

        const auto solo = _model->soloRect(row);
        fill(solo, channel.soloed ? [NSColor colorWithCalibratedRed:0.85 green:0.70 blue:0.25 alpha:1.0]
                                  : grey(0.24));
        drawText(@"S", {solo.x, solo.y + 2.0, solo.width, solo.height},
                 channel.soloed ? grey(0.1) : grey(0.9), 10.0, YES);

        const auto volume = _model->volumeRect(row);
        fill(volume, grey(0.22));
        fill({volume.x, volume.y, volume.width * channel.volume, volume.height}, grey(0.55));

        if (pattern == nullptr)
            continue;

        const std::vector<project::MidiEvent>* events = pattern->events(channel.id);

        for (int step = _model->firstStep(); step < lastStep; ++step) {
            const auto cell = _model->stepRect(row, step);

            const bool on = events != nullptr
                         && app::noteAtStep(*events, _model->tickForStep(step), _model->stepTicks(),
                                            channel.stepKey) != app::noStep;

            // Every fourth step is lighter, so the beat is readable without a
            // ruler above the grid.
            const bool downbeat = (step % 4) == 0;
            NSColor* off = downbeat ? grey(0.235) : grey(0.175);

            fill(cell, on ? colourFrom(channel.colour, channel.muted ? 0.5 : 1.0) : off);

            if (step == playheadStep) {
                [[NSColor colorWithCalibratedWhite:1.0 alpha:0.18] setFill];
                NSRectFillUsingOperation(NSMakeRect(cell.x, cell.y, cell.width, cell.height),
                                         NSCompositingOperationPlusLighter);
            }
        }
    }

    // The add row, so a rack with one channel does not look like a rack that
    // cannot have two.
    auto addRow = _model->rowRect(channels.size());
    addRow.width = self.bounds.size.width;
    fill(addRow, grey(0.13));
    drawText(@"＋  Add channel", {addRow.x + layout.padding, addRow.y + 6.0,
                                  addRow.width, addRow.height}, grey(0.55), 12.0);
}

// ── Input ────────────────────────────────────────────────────────────────────

- (app::ChannelRackModel::Hit)hitForEvent:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    return _model->hitTest([self channelCount], [self currentPattern], point.x, point.y);
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    const auto hit = _model->hitTest([self channelCount], [self currentPattern], point.x, point.y);

    _drag = RackDrag::none;
    _lastPaintedStep = -1;

    if (hit.row == app::ChannelRackModel::noRow) {
        // Below the last channel is the add row.
        const auto addRow = _model->rowRect([self channelCount]);
        if (point.y >= addRow.y && point.y < addRow.y + addRow.height)
            [self addChannel];

        return;
    }

    const project::Channel& channel = _project->channels()[hit.row];
    const project::EntityId channelId = channel.id;

    switch (hit.zone) {
        case app::ChannelRackModel::Zone::name:
            if (event.clickCount == 2)
                [self renameChannel:channelId];
            else if (self.onSelectChannel != nil)
                self.onSelectChannel(channelId.value());

            [self setNeedsDisplay:YES];
            return;

        case app::ChannelRackModel::Zone::mute:
            [self commit:std::make_unique<app::SetChannelMutedCommand>(channelId, !channel.muted)];
            return;

        case app::ChannelRackModel::Zone::solo:
            [self commit:std::make_unique<app::SetChannelSoloedCommand>(channelId, !channel.soloed)];
            return;

        case app::ChannelRackModel::Zone::volume:
            _drag        = RackDrag::volume;
            _dragChannel = channelId;
            [self applyVolumeAt:point row:hit.row];
            return;

        case app::ChannelRackModel::Zone::step:
            _drag        = RackDrag::paintSteps;
            _dragChannel = channelId;
            [self toggleStep:hit.step channel:channelId];
            _paintOn         = [self stepIsOn:hit.step channel:channelId];
            _lastPaintedStep = hit.step;
            return;

        case app::ChannelRackModel::Zone::none:
            return;
    }
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_drag == RackDrag::none)
        return;

    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];

    if (_drag == RackDrag::volume) {
        const std::size_t row = _project->indexOfChannel(_dragChannel);
        if (row != project::Project::notFound)
            [self applyVolumeAt:point row:row];

        return;
    }

    const auto hit = _model->hitTest([self channelCount], [self currentPattern], point.x, point.y);
    if (hit.zone != app::ChannelRackModel::Zone::step || hit.step == _lastPaintedStep)
        return;

    // Painting sets cells to match the one the gesture started on, rather than
    // toggling each: dragging across a programmed step would otherwise erase it
    // on the way past.
    if ([self stepIsOn:hit.step channel:_dragChannel] != _paintOn) {
        [self toggleStep:hit.step channel:_dragChannel];
        _lastPaintedStep = hit.step;
    }
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _drag = RackDrag::none;
}

- (void)rightMouseDown:(NSEvent*)event
{
    const auto hit = [self hitForEvent:event];
    if (hit.row == app::ChannelRackModel::noRow)
        return;

    const project::EntityId channelId = _project->channels()[hit.row].id;

    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* rename = [menu addItemWithTitle:@"Rename Channel…"
                                         action:@selector(renameFromMenu:)
                                  keyEquivalent:@""];
    rename.target = self;
    rename.representedObject = @(channelId.value());

    NSMenuItem* remove = [menu addItemWithTitle:@"Remove Channel"
                                         action:@selector(removeFromMenu:)
                                  keyEquivalent:@""];
    remove.target = self;
    remove.representedObject = @(channelId.value());

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)renameFromMenu:(NSMenuItem*)item
{
    [self renameChannel:project::EntityId{[item.representedObject unsignedLongLongValue]}];
}

- (void)removeFromMenu:(NSMenuItem*)item
{
    const project::EntityId channelId{[item.representedObject unsignedLongLongValue]};

    [self commit:std::make_unique<app::RemoveChannelCommand>(channelId)];

    if (channelId.value() == _selectedChannelIdValue && !_project->channels().empty()
        && self.onSelectChannel != nil)
        self.onSelectChannel(_project->channels().front().id.value());
}

- (void)keyDown:(NSEvent*)event
{
    const unichar character = event.charactersIgnoringModifiers.length > 0
                                  ? [event.charactersIgnoringModifiers characterAtIndex:0]
                                  : 0;

    const bool command = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
    const bool shift   = (event.modifierFlags & NSEventModifierFlagShift) != 0;

    // Undo has to work from whichever editor has focus. Both call the same
    // registry, so the two panes share one history rather than keeping two.
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

- (void)scrollWheel:(NSEvent*)event
{
    // Horizontal scrolling moves the step window rather than the view, so the
    // channel names stay pinned at the left edge where they are useful.
    // Vertical scrolling is the enclosing scroll view's job.
    if (std::abs(event.scrollingDeltaX) <= std::abs(event.scrollingDeltaY)) {
        [super scrollWheel:event];
        return;
    }

    const double pitch = _model->layout().stepWidth + _model->layout().stepGap;
    if (pitch <= 0.0)
        return;

    _model->setFirstStep(_model->firstStep() - static_cast<int>(event.scrollingDeltaX / pitch));
    [self setNeedsDisplay:YES];
}

/// Sizes the document view to the rack's contents, so the enclosing scroll view
/// can reach every channel.
- (void)updateContentSize
{
    if (_project == nullptr)
        return;

    const double height = _model->contentHeight(_project->channels().size() + 1);
    const CGFloat width = self.enclosingScrollView != nil
                              ? self.enclosingScrollView.contentSize.width
                              : self.frame.size.width;

    if (self.frame.size.height != height || self.frame.size.width != width)
        [self setFrameSize:NSMakeSize(width, static_cast<CGFloat>(height))];
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

// ── Edits ────────────────────────────────────────────────────────────────────

- (BOOL)stepIsOn:(int)step channel:(project::EntityId)channelId
{
    const project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr)
        return NO;

    const std::vector<project::MidiEvent>* events = pattern->events(channelId);
    if (events == nullptr)
        return NO;

    const project::Channel* channel = _project->findChannel(channelId);
    if (channel == nullptr)
        return NO;

    return app::noteAtStep(*events, _model->tickForStep(step), _model->stepTicks(),
                           channel->stepKey) != app::noStep;
}

- (void)toggleStep:(int)step channel:(project::EntityId)channelId
{
    const project::Channel* channel = _project->findChannel(channelId);
    if (channel == nullptr)
        return;

    app::ToggleStepCommand::Step cell;
    cell.pattern  = project::EntityId{_patternIdValue};
    cell.channel  = channelId;
    cell.start    = _model->tickForStep(step);
    cell.length   = _model->stepTicks();
    cell.key      = channel->stepKey;
    cell.velocity = 100;

    [self commit:std::make_unique<app::ToggleStepCommand>(cell)];
}

- (void)applyVolumeAt:(NSPoint)point row:(std::size_t)row
{
    const double volume = _model->volumeForX(row, point.x);

    if (_registry->executeMerging(std::make_unique<app::SetChannelVolumeCommand>(_dragChannel, volume)))
        [self changed];
}

- (void)addChannel
{
    auto command = std::make_unique<app::AddChannelCommand>(
        "Channel " + std::to_string(_project->channels().size() + 1));

    app::AddChannelCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    if (self.onSelectChannel != nil)
        self.onSelectChannel(raw->channelId().value());

    [self changed];
}

- (void)renameChannel:(project::EntityId)channelId
{
    const project::Channel* channel = _project->findChannel(channelId);
    if (channel == nullptr)
        return;

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Rename channel";
    [alert addButtonWithTitle:@"Rename"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 24)];
    field.stringValue = @(channel->name.c_str());
    alert.accessoryView = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    [self commit:std::make_unique<app::RenameChannelCommand>(channelId,
                                                             field.stringValue.UTF8String)];
}

- (void)commit:(app::CommandPtr)command
{
    if (_registry->execute(std::move(command)))
        [self changed];
}

- (void)changed
{
    [self updateContentSize];
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

- (void)setPatternIdValue:(unsigned long long)value
{
    _patternIdValue = value;
    [self setNeedsDisplay:YES];
}

- (void)setSelectedChannelIdValue:(unsigned long long)value
{
    _selectedChannelIdValue = value;
    [self setNeedsDisplay:YES];
}

- (void)setPlayheadTick:(long long)tick
{
    if (_playheadTick == tick)
        return;

    _playheadTick = tick;
    [self setNeedsDisplay:YES];
}

@end
