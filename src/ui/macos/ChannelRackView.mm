#include "ui/macos/ChannelRackView.h"

#include "app/ChannelRackModel.h"
#include "app/CommandRegistry.h"
#include "app/commands/ChannelCommands.h"
#include "app/Browser.h"
#include "app/commands/ImportCommands.h"
#include "app/commands/SamplerCommands.h"
#include "app/commands/StepCommands.h"
#include "engine/instrument/BuiltinInstruments.h"
#include "plugins/PluginIdentifier.h"
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
enum class RackDrag { none, volume, pan, paintSteps };

} // namespace

@implementation INCDAWChannelRackView {
    project::Project*     _project;

    /// Row a drag is hovering, or noRow. Drawn so the drop's destination is
    /// visible before the mouse is released.
    std::size_t           _dropRow;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::ChannelRackModel> _model;

    RackDrag          _drag;
    project::EntityId _dragChannel;

    /// Where a knob drag began, and the value it began at. Shared by both
    /// knobs, because only one of them can be turning at a time.
    double            _knobDragStartY;
    double            _knobDragStart;
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
    _dropRow      = app::ChannelRackModel::noRow;
    [self registerForDraggedTypes:@[ NSPasteboardTypeFileURL ]];
    _registry     = registry;
    _model        = std::make_unique<app::ChannelRackModel>();

    // The numbered band over the grid. The model defaults it to nothing so its
    // geometry tests describe rows at the top; the rack is the view that wants
    // one.
    app::ChannelRackModel::Layout layout = _model->layout();
    layout.rulerHeight = 20.0;
    _model->setLayout(layout);

    _drag         = RackDrag::none;
    _playheadTick = -1;
    _lastPaintedStep = -1;
    _knobDragStartY = 0.0;
    _knobDragStart  = 0.0;

    return self;
}

// Flipped, so the view agrees with app::ChannelRackModel instead of converting
// at every call — the same choice the Piano Roll makes.
- (BOOL)isFlipped { return YES; }

/// The file being dragged, if it is one INCDAW can actually load.
///
/// Answered from the pasteboard rather than from the drag source, so a drop
/// from Finder works exactly like a drop from the Browser pane — the rack does
/// not care where a sample came from.
static NSString* droppedSamplePath(id<NSDraggingInfo> info)
{
    NSArray<NSURL*>* urls = [info.draggingPasteboard
        readObjectsForClasses:@[ [NSURL class] ]
                      options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];

    if (urls.count != 1)
        return nil;

    NSString* path = urls.firstObject.path;

    return path != nil && incdaw::app::Browser::canDecodeAudio(std::filesystem::path{path.UTF8String})
               ? path
               : nil;
}
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

    [self drawRuler:lastStep playheadStep:playheadStep];

    [self drawChannels:channels pattern:pattern lastStep:lastStep playheadStep:playheadStep];
}

/// The band over the grid: where a beat starts, which bar it belongs to, and
/// where the playhead is. Without it a step sequencer is a field of identical
/// squares and counting to sixteen is the user's problem.
- (void)drawRuler:(int)lastStep playheadStep:(long long)playheadStep
{
    const auto& layout = _model->layout();
    if (layout.rulerHeight <= 0.0)
        return;

    const NSRect band = box(_model->rulerRect(self.bounds.size.width));

    theme::fillRect(band, theme::ink(Ink::panelRaised));
    theme::drawSeparator(NSMakeRect(0.0, NSMaxY(band) - 1.0, band.size.width, 1.0));

    theme::drawTextCentred(@"STEPS",
                           NSMakeRect(layout.padding, NSMinY(band),
                                      layout.headerWidth - layout.padding * 2.0,
                                      band.size.height),
                           theme::ink(Ink::textDim), theme::labelFont(9.0, NSFontWeightBold));

    for (int step = _model->firstStep(); step <= lastStep; ++step) {
        const NSRect cell = box(_model->rulerStepRect(step));

        if (NSMinX(cell) > self.bounds.size.width)
            break;

        const bool beat    = step % 4 == 0;
        const bool barLine = step % 16 == 0;

        if (step == playheadStep)
            theme::fillRounded(NSInsetRect(cell, 0.0, 4.0), theme::metrics::radiusPad,
                               theme::withAlpha(theme::ink(Ink::playhead), 0.55));

        // A number on every beat, a tick on every other step: numbering all
        // sixteen would be a wall of digits at this pitch.
        if (beat)
            theme::drawTextCentred([NSString stringWithFormat:@"%d", step / 4 + 1], cell,
                                   barLine ? theme::ink(Ink::textPrimary)
                                           : theme::ink(Ink::textSecondary),
                                   theme::labelFont(barLine ? 9.5 : 9.0,
                                                    barLine ? NSFontWeightBold
                                                            : NSFontWeightMedium),
                                   theme::Align::centre);
        else
            theme::fillRect(NSMakeRect(NSMidX(cell) - 0.5, NSMaxY(cell) - 7.0, 1.0, 3.0),
                            theme::ink(Ink::textDim));
    }
}

/// What the row's second line says about where its sound comes from.
///
/// Read from the channel rather than from any engine handle: the rack draws
/// from the project model, and a name that needed a live graph would be blank
/// exactly when the graph failed to build.
- (NSString*)sourceNameForChannel:(const project::Channel&)channel
{
    NSString* instrument = @"No instrument";

    if (channel.instrument.uid.empty()) {
        // An empty identifier is not "silent": the compiler falls back to the
        // builtin synth, and the row must say what will actually be heard.
        if (const auto* info = engine::findBuiltinInstrument(plugins::builtinSimpleSynth().uid))
            instrument = @(info->displayName);
    } else if (const auto* info = engine::findBuiltinInstrument(channel.instrument.uid)) {
        instrument = @(info->displayName);
    } else {
        // A hosted plugin whose catalogue entry is not reachable from here.
        // Its uid is worse than a name and better than nothing.
        instrument = @(channel.instrument.uid.c_str());
    }

    if (channel.samplerZones.empty())
        return instrument;

    return [NSString stringWithFormat:@"%@ · %lu %s", instrument,
                                      static_cast<unsigned long>(channel.samplerZones.size()),
                                      channel.samplerZones.size() == 1 ? "zone" : "zones"];
}

- (void)drawChannels:(const std::vector<project::Channel>&)channels
             pattern:(const project::Pattern*)pattern
            lastStep:(int)lastStep
        playheadStep:(long long)playheadStep
{
    const auto& layout = _model->layout();

    for (std::size_t row = 0; row < channels.size(); ++row) {
        const project::Channel& channel = channels[row];
        const bool selected = channel.id.value() == _selectedChannelIdValue;

        NSColor* colour = theme::fromArgb(channel.colour);

        // ── The row: lamp, switch, button, two knobs ─────────────────────────
        //
        // The order a step sequencer's row has had since the hardware ones, and
        // it survives because it matches how the row is used: the lamp is hit
        // constantly while writing a part, the button opens the sound, and the
        // knobs are set once and left.

        // The lamp is the mute switch AND the colour swatch — lit in the
        // channel's own colour when the channel is heard, dark when it is not.
        // A separate swatch beside it would say the same thing twice and cost
        // the row points it needs for steps.
        const NSRect lamp = box(_model->muteRect(row));

        theme::fillRounded(NSInsetRect(lamp, -1.5, -1.5), lamp.size.height / 2.0 + 1.5,
                           theme::ink(Ink::panelSunken));

        theme::fillRounded(lamp, lamp.size.height / 2.0,
                           channel.muted ? theme::darken(colour, 0.7) : colour);

        if (!channel.muted)
            theme::fillRounded(NSInsetRect(lamp, 3.0, 3.0), lamp.size.height / 5.0,
                               theme::withAlpha(theme::lighten(colour, 0.65), 0.9));

        theme::drawToggle(box(_model->soloRect(row)), @"S", channel.soloed,
                          theme::ink(Ink::solo), true);

        // The channel button: the widest thing in the row, because it is the
        // one that opens the instrument. Tinted with the channel's colour, so
        // the rack stays scannable by colour when the names all read alike.
        const NSRect button = box(_model->nameRect(row));

        theme::fillGradient(button, theme::metrics::radiusControl,
                            theme::mix(theme::ink(Ink::panelRaised), colour,
                                       channel.muted ? 0.08 : 0.24),
                            theme::mix(theme::ink(Ink::panel), colour,
                                       channel.muted ? 0.04 : 0.12), true);

        theme::strokeRounded(button, theme::metrics::radiusControl,
                             selected ? theme::ink(Ink::accent)
                                      : theme::withAlpha(theme::ink(Ink::textDim), 0.35),
                             selected ? 1.5 : 1.0);

        const NSRect label = NSInsetRect(button, 8.0, 0.0);

        theme::drawText(@(channel.name.c_str()), label,
                        channel.muted ? theme::ink(Ink::textDim)
                                      : theme::ink(Ink::textPrimary),
                        theme::labelFont(12.0, NSFontWeightMedium));

        // What makes the sound, set right inside the same button. The rack used
        // to name every channel and tell you nothing about any of them — a
        // sampler and a synth were indistinguishable until you opened one — and
        // this says it without costing the row a second line.
        theme::drawText([self sourceNameForChannel:channel], label,
                        theme::ink(Ink::textDim), theme::labelFont(9.0),
                        theme::Align::right);

        // Two knobs rather than a fader: both fit where one fader would, and
        // the width they save goes to the steps. Pan is bipolar, because centre
        // is a value and not the absence of one.
        theme::drawKnob(box(_model->volumeRect(row)), channel.volume,
                        channel.muted ? theme::ink(Ink::textDim) : colour);

        theme::drawKnob(box(_model->panRect(row)), (channel.pan + 1.0) / 2.0,
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

    // The row a dragged sample would land on. Drawn last so it reads over the
    // row it marks: dropping ON a channel replaces its sound, and the user
    // must be able to see which one before letting go.
    if (_dropRow != app::ChannelRackModel::noRow && _dropRow < channels.size()) {
        auto target  = _model->rowRect(_dropRow);
        target.width = self.bounds.size.width;

        theme::strokeRounded(NSInsetRect(box(target), 1.0, 1.0),
                             theme::metrics::radiusControl, theme::ink(Ink::accent), 2.0);
    }
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
            // Double-click restores unity, the way double-clicking pan
            // recentres it: a knob needs a way back to its default that does
            // not depend on landing exactly on it by hand.
            if (event.clickCount == 2) {
                [self commit:std::make_unique<app::SetChannelVolumeCommand>(channelId, 1.0)];
                return;
            }

            _drag           = RackDrag::volume;
            _dragChannel    = channelId;
            _knobDragStartY = point.y;
            _knobDragStart  = channel.volume;
            return;

        case app::ChannelRackModel::Zone::pan:
            // Double-click recentres. A bipolar control needs a way back to
            // zero that does not depend on landing exactly on it by hand.
            if (event.clickCount == 2) {
                [self commit:std::make_unique<app::SetChannelPanCommand>(channelId, 0.0)];
                return;
            }

            _drag           = RackDrag::pan;
            _dragChannel    = channelId;
            _knobDragStartY = point.y;
            _knobDragStart  = channel.pan;
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

    // Both knobs turn from where the drag started, not from where the knob is
    // now: reading the current value on each move compounds rounding, and the
    // knob creeps away from the cursor over a long drag.
    if (_drag == RackDrag::volume) {
        const double volume = app::ChannelRackModel::volumeForDrag(_knobDragStart,
                                                                   _knobDragStartY, point.y);

        [self merge:std::make_unique<app::SetChannelVolumeCommand>(_dragChannel, volume)];
        return;
    }

    if (_drag == RackDrag::pan) {
        const double pan = app::ChannelRackModel::panForDrag(_knobDragStart,
                                                             _knobDragStartY, point.y);

        [self merge:std::make_unique<app::SetChannelPanCommand>(_dragChannel, pan)];
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

    NSMenuItem* editInstrument = [menu addItemWithTitle:@"Edit Instrument…"
                                                 action:@selector(editInstrumentFromMenu:)
                                          keyEquivalent:@""];
    editInstrument.target = self;
    editInstrument.representedObject = @(channelId.value());

    NSMenuItem* editZones = [menu addItemWithTitle:@"Sampler Zones…"
                                            action:@selector(editZonesFromMenu:)
                                     keyEquivalent:@""];
    editZones.target = self;
    editZones.representedObject = @(channelId.value());

    [menu addItem:[NSMenuItem separatorItem]];

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

- (void)editInstrumentFromMenu:(NSMenuItem*)item
{
    if (self.onEditInstrument != nil)
        self.onEditInstrument([item.representedObject unsignedLongLongValue]);
}

- (void)editZonesFromMenu:(NSMenuItem*)item
{
    if (self.onEditSamplerZones != nil)
        self.onEditSamplerZones([item.representedObject unsignedLongLongValue]);
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

// ── Dropping a sample ───────────────────────────────────────────────────────

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info
{
    return [self draggingUpdated:info];
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)info
{
    if (droppedSamplePath(info) == nil) {
        _dropRow = app::ChannelRackModel::noRow;
        return NSDragOperationNone;
    }

    const NSPoint point = [self convertPoint:info.draggingLocation fromView:nil];
    const auto    hit   = _model->hitTest([self channelCount], [self currentPattern],
                                          point.x, point.y);

    _dropRow = hit.row;
    [self setNeedsDisplay:YES];

    return NSDragOperationCopy;
}

- (void)draggingExited:(id<NSDraggingInfo>)info
{
    (void)info;
    _dropRow = app::ChannelRackModel::noRow;
    [self setNeedsDisplay:YES];
}

/// A sample dropped ON a channel replaces that channel's sound; a sample
/// dropped anywhere else becomes a new channel. Both are one undo, and both go
/// through the same commands the menus use — a drop is a gesture, not a
/// second way of editing the project.
- (BOOL)performDragOperation:(id<NSDraggingInfo>)info
{
    NSString* path = droppedSamplePath(info);

    const std::size_t row = _dropRow;
    _dropRow              = app::ChannelRackModel::noRow;
    [self setNeedsDisplay:YES];

    if (path == nil || _project == nullptr)
        return NO;

    if (row != app::ChannelRackModel::noRow && row < _project->channels().size()) {
        const project::EntityId channelId = _project->channels()[row].id;
        [self commit:std::make_unique<app::LoadSampleCommand>(channelId, path.UTF8String)];

        if (self.onSelectChannel != nil)
            self.onSelectChannel(channelId.value());

        return YES;
    }

    auto command = std::make_unique<app::ImportSampleAsChannelCommand>(path.UTF8String);
    app::ImportSampleAsChannelCommand* imported = command.get();

    [self commit:std::move(command)];

    if (imported->channelId().isValid() && self.onSelectChannel != nil)
        self.onSelectChannel(imported->channelId().value());

    return YES;
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

/// A step of a continuous gesture: merges into the entry the gesture opened,
/// so a knob drag is one undo and not one per mouse move.
- (void)merge:(app::CommandPtr)command
{
    if (_registry->executeMerging(std::move(command)))
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
