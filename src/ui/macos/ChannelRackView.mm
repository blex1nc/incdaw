#include "ui/macos/ChannelRackView.h"

#include "app/ChannelRackModel.h"
#include "app/CommandRegistry.h"
#include "app/commands/ChannelCommands.h"
#include "app/commands/SamplerCommands.h"
#include "app/commands/StepCommands.h"
#include "project/Model.h"
#include "ui/macos/Theme.h"

#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;

namespace theme = incdaw::ui::theme;

namespace {

using theme::Ink;

/// The rack draws in the model's coordinates; this is the only place they turn
/// into AppKit rectangles.
NSRect box(app::ChannelRackModel::Rect rect)
{
    return NSMakeRect(rect.x, rect.y, rect.width, rect.height);
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

    theme::fillRect(self.bounds, theme::ink(Ink::panel));

    if (_project == nullptr)
        return;

    const project::Pattern* pattern = [self currentPattern];
    const auto& layout = _model->layout();

    // The step grid sits in a well that runs the height of the rack, so the
    // pads read as one instrument rather than as loose buttons per row.
    const NSRect gridArea = NSMakeRect(layout.headerWidth - 4.0, 0.0,
                                       self.bounds.size.width - layout.headerWidth + 4.0,
                                       self.bounds.size.height);
    theme::fillRect(gridArea, theme::ink(Ink::panelSunken));

    // Only the steps that fit are drawn; a 512-step pattern must not cost 512
    // rectangles per row per frame.
    const int steps = pattern != nullptr ? _model->stepCount(*pattern) : 0;
    const int visible = _model->visibleStepCount(self.bounds.size.width - layout.headerWidth) + 1;
    const int lastStep = std::min(steps, _model->firstStep() + visible);

    const long long playheadStep = _playheadTick >= 0 && _model->stepTicks() > 0
                                       ? _playheadTick / _model->stepTicks()
                                       : -1;

    const std::vector<project::Channel>& channels = _project->channels();

    // Bar lines behind the pads: four beats of sixteenths, the division a step
    // sequencer is counted in.
    for (int step = _model->firstStep(); step <= lastStep; ++step) {
        if (step % 16 != 0)
            continue;

        const auto cell = _model->stepRect(0, step);
        theme::fillRect(NSMakeRect(cell.x - layout.stepGap / 2.0 - 0.5, 0.0, 1.0,
                                   self.bounds.size.height),
                        theme::ink(Ink::gridLineStrong));
    }

    for (std::size_t row = 0; row < channels.size(); ++row) {
        const project::Channel& channel = channels[row];
        const bool selected = channel.id.value() == _selectedChannelIdValue;

        NSColor* colour = theme::fromArgb(channel.colour);

        // ── Header: the channel's own strip ──────────────────────────────────
        const NSRect header = NSInsetRect(box(_model->rowRect(row)), 2.0, 0.0);
        theme::drawPanel(header, theme::metrics::radiusControl, selected, true);

        if (selected)
            theme::strokeRounded(header, theme::metrics::radiusControl, theme::ink(Ink::accent));

        // The colour tile: a channel is identified by its colour before it is
        // read by its name, which is why it gets a lit tile and not a hairline.
        const NSRect swatch = NSMakeRect(NSMinX(header) + 5.0, NSMinY(header) + 5.0,
                                         layout.swatchWidth,
                                         header.size.height - 10.0);

        theme::fillGradient(swatch, 2.5, theme::lighten(colour, 0.25),
                            theme::darken(colour, channel.muted ? 0.65 : 0.15), true);

        theme::drawTextCentred(@(channel.name.c_str()), box(_model->nameRect(row)),
                               channel.muted ? theme::ink(Ink::textDim)
                                             : theme::ink(Ink::textPrimary),
                               theme::labelFont(12.0, NSFontWeightMedium));

        theme::drawToggle(box(_model->muteRect(row)), @"M", channel.muted,
                          theme::ink(Ink::mute), true);

        theme::drawToggle(box(_model->soloRect(row)), @"S", channel.soloed,
                          theme::ink(Ink::solo), true);

        theme::drawSlider(box(_model->volumeRect(row)), channel.volume,
                          channel.muted ? theme::ink(Ink::textDim) : colour, true);

        if (pattern == nullptr)
            continue;

        // ── Steps ────────────────────────────────────────────────────────────
        const std::vector<project::MidiEvent>* events = pattern->events(channel.id);

        for (int step = _model->firstStep(); step < lastStep; ++step) {
            const bool on = events != nullptr
                         && app::noteAtStep(*events, _model->tickForStep(step), _model->stepTicks(),
                                            channel.stepKey) != app::noStep;

            // Every fourth step is lighter, so the beat is readable without a
            // ruler above the grid.
            theme::drawStepPad(box(_model->stepRect(row, step)), colour, on, (step % 4) == 0,
                               step == playheadStep, true);
        }
    }

    if (playheadStep >= _model->firstStep() && playheadStep < lastStep) {
        const auto cell = _model->stepRect(0, static_cast<int>(playheadStep));
        theme::fillRect(NSMakeRect(cell.x - 1.0, 0.0, 1.0, self.bounds.size.height),
                        theme::withAlpha(theme::ink(Ink::playhead), 0.75));
    }

    // The add row, so a rack with one channel does not look like a rack that
    // cannot have two.
    auto addRow = _model->rowRect(channels.size());
    addRow.width = self.bounds.size.width;

    const NSRect addRect = NSInsetRect(box(addRow), 2.0, 2.0);
    theme::fillRounded(addRect, theme::metrics::radiusControl,
                       theme::withAlpha(theme::ink(Ink::panelRaised), 0.55));
    theme::strokeRounded(addRect, theme::metrics::radiusControl,
                         theme::withAlpha(theme::ink(Ink::textDim), 0.35));

    theme::drawTextCentred(@"＋  Add channel",
                           NSMakeRect(NSMinX(addRect) + layout.padding, NSMinY(addRect),
                                      addRect.size.width, addRect.size.height),
                           theme::ink(Ink::textSecondary), theme::labelFont(12.0));
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

    NSMenuItem* loadSample = [menu addItemWithTitle:@"Load Sample…"
                                             action:@selector(loadSampleFromMenu:)
                                      keyEquivalent:@""];
    loadSample.target = self;
    loadSample.representedObject = @(channelId.value());

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

- (void)loadSampleFromMenu:(NSMenuItem*)item
{
    const project::EntityId channelId{[item.representedObject unsignedLongLongValue]};

    NSOpenPanel* panel            = [NSOpenPanel openPanel];
    panel.canChooseFiles          = YES;
    panel.canChooseDirectories    = NO;
    panel.allowsMultipleSelection = NO;
    panel.allowedContentTypes     = @[ [UTType typeWithFilenameExtension:@"wav"] ];
    panel.prompt                  = @"Load";

    if ([panel runModal] != NSModalResponseOK || panel.URL == nil)
        return;

    [self commit:std::make_unique<app::LoadSampleCommand>(channelId,
                                                          panel.URL.path.UTF8String)];
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
