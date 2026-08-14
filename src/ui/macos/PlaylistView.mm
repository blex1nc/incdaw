#include "ui/macos/PlaylistView.h"

#include "app/CommandRegistry.h"
#include "app/PlaylistModel.h"
#include "app/commands/ClipCommands.h"
#include "app/commands/TrackCommands.h"
#include "project/PatternCompiler.h"
#include "project/Model.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

/// Track headers down the left edge, and the ruler across the top. Both are
/// pinned: the grid scrolls under them.
constexpr CGFloat headerWidth = 150.0;
constexpr CGFloat rulerHeight = 22.0;

/// The row below the last track, which creates one.
constexpr CGFloat addRowHeight = 26.0;

constexpr CGFloat buttonWidth = 18.0;
constexpr CGFloat padding     = 6.0;

NSColor* grey(CGFloat white) { return [NSColor colorWithCalibratedWhite:white alpha:1.0]; }

NSColor* colourFrom(std::uint32_t argb, CGFloat brightness = 1.0)
{
    return [NSColor colorWithCalibratedRed:static_cast<CGFloat>((argb >> 16) & 0xFFu) / 255.0 * brightness
                                     green:static_cast<CGFloat>((argb >> 8) & 0xFFu) / 255.0 * brightness
                                      blue:static_cast<CGFloat>(argb & 0xFFu) / 255.0 * brightness
                                     alpha:1.0];
}

void fillRect(NSRect rect, NSColor* colour)
{
    [colour setFill];
    NSRectFill(rect);
}

void drawText(NSString* text, NSRect rect, NSColor* colour, CGFloat size, BOOL centred = NO)
{
    NSMutableParagraphStyle* style = [[NSMutableParagraphStyle alloc] init];
    style.lineBreakMode = NSLineBreakByTruncatingTail;
    style.alignment     = centred ? NSTextAlignmentCenter : NSTextAlignmentLeft;

    [text drawInRect:rect
      withAttributes:@{NSFontAttributeName: [NSFont systemFontOfSize:size],
                       NSForegroundColorAttributeName: colour,
                       NSParagraphStyleAttributeName: style}];
}

enum class PlaylistDrag { none, move, resize, boxSelect };

} // namespace

@implementation INCDAWPlaylistView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::PlaylistModel> _model;

    std::vector<app::PlaylistModel::VisibleClip> _visible;
    std::vector<project::EntityId>               _boxed;

    PlaylistDrag _drag;
    NSPoint      _dragOrigin;
    NSPoint      _dragCurrent;
    Tick         _dragAppliedTicks;
    int          _dragAppliedTracks;
    Tick         _dragAppliedLength;
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
    _model    = std::make_unique<app::PlaylistModel>();

    _drag              = PlaylistDrag::none;
    _dragAppliedTicks  = 0;
    _dragAppliedTracks = 0;
    _dragAppliedLength = 0;
    _playheadTick      = -1;

    app::PlaylistModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 4 * 16;   // sixteen bars
    viewport.width        = frame.size.width - headerWidth;
    viewport.height       = frame.size.height - rulerHeight;
    _model->setViewport(viewport);
    _model->setSnap(ticksPerQuarterNote * 4);               // one bar

    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];

    auto viewport = _model->viewport();
    viewport.width  = size.width - headerWidth;
    viewport.height = size.height - rulerHeight;
    _model->setViewport(viewport);

    [self setNeedsDisplay:YES];
}

/// Point in the grid's own coordinates: the ruler and the track headers are
/// pinned, so everything else works relative to the corner where they meet.
- (NSPoint)gridPointFor:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    return NSMakePoint(point.x - headerWidth, point.y - rulerHeight);
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)drawRect:(NSRect)dirtyRect
{
    (void)dirtyRect;

    fillRect(self.bounds, grey(0.10));

    if (_project == nullptr)
        return;

    [self drawRuler];
    [self drawTracks];
    [self drawClips];
    [self drawPlayhead];

    if (_drag == PlaylistDrag::boxSelect) {
        const NSRect box = NSMakeRect(std::min(_dragOrigin.x, _dragCurrent.x) + headerWidth,
                                      std::min(_dragOrigin.y, _dragCurrent.y) + rulerHeight,
                                      std::abs(_dragCurrent.x - _dragOrigin.x),
                                      std::abs(_dragCurrent.y - _dragOrigin.y));

        [[NSColor colorWithCalibratedWhite:1.0 alpha:0.10] setFill];
        NSRectFillUsingOperation(box, NSCompositingOperationSourceOver);
        [[NSColor colorWithCalibratedWhite:1.0 alpha:0.35] setStroke];
        NSFrameRect(box);
    }
}

- (void)drawRuler
{
    fillRect(NSMakeRect(0, 0, self.bounds.size.width, rulerHeight), grey(0.14));

    const auto& viewport = _model->viewport();
    const Tick barTicks  = ticksPerQuarterNote * 4;

    const Tick first = (viewport.firstTick / barTicks) * barTicks;
    const Tick last  = viewport.firstTick + viewport.visibleTicks;

    for (Tick tick = first; tick <= last; tick += barTicks) {
        const CGFloat x = headerWidth + _model->tickToX(tick);
        if (x < headerWidth)
            continue;

        fillRect(NSMakeRect(x, 0, 1.0, rulerHeight), grey(0.30));

        drawText([NSString stringWithFormat:@"%lld", static_cast<long long>(tick / barTicks) + 1],
                 NSMakeRect(x + 3.0, 4.0, 40.0, 14.0), grey(0.55), 10.0);
    }
}

- (void)drawTracks
{
    const std::vector<project::Track>& tracks = _project->tracks();

    for (std::size_t row = 0; row < tracks.size(); ++row) {
        const project::Track& track = tracks[row];

        const CGFloat y      = rulerHeight + _model->trackY(tracks, row);
        const CGFloat height = app::PlaylistModel::trackHeight(track);

        if (y + height < rulerHeight || y > self.bounds.size.height)
            continue;

        // The lane behind the clips, so an empty track is still a place rather
        // than a gap.
        fillRect(NSMakeRect(headerWidth, y, self.bounds.size.width - headerWidth, height - 1.0),
                 (row % 2) == 0 ? grey(0.135) : grey(0.125));

        [self drawBarLinesInLaneAt:y height:height - 1.0];

        fillRect(NSMakeRect(0, y, headerWidth, height - 1.0), grey(0.17));
        fillRect(NSMakeRect(0, y, 4.0, height - 1.0),
                 colourFrom(track.colour, track.muted ? 0.4 : 1.0));

        drawText(@(track.name.c_str()),
                 NSMakeRect(10.0, y + 5.0, headerWidth - 2.0 * buttonWidth - 3.0 * padding, 16.0),
                 track.muted ? grey(0.45) : grey(0.88), 12.0);

        const NSRect mute = [self muteRectForRow:row];
        fillRect(mute, track.muted ? [NSColor colorWithCalibratedRed:0.75 green:0.30 blue:0.25 alpha:1.0]
                                   : grey(0.24));
        drawText(@"M", NSMakeRect(mute.origin.x, mute.origin.y + 2.0, mute.size.width, mute.size.height),
                 grey(0.9), 10.0, YES);

        const NSRect solo = [self soloRectForRow:row];
        fillRect(solo, track.soloed ? [NSColor colorWithCalibratedRed:0.85 green:0.70 blue:0.25 alpha:1.0]
                                    : grey(0.24));
        drawText(@"S", NSMakeRect(solo.origin.x, solo.origin.y + 2.0, solo.size.width, solo.size.height),
                 track.soloed ? grey(0.1) : grey(0.9), 10.0, YES);
    }

    const NSRect addRow = [self addTrackRect];
    if (addRow.origin.y < self.bounds.size.height) {
        fillRect(addRow, grey(0.13));
        drawText(@"＋  Add track",
                 NSMakeRect(10.0, addRow.origin.y + 6.0, headerWidth, 16.0), grey(0.55), 12.0);
    }
}

- (void)drawBarLinesInLaneAt:(CGFloat)y height:(CGFloat)height
{
    const auto& viewport = _model->viewport();
    const Tick  barTicks = ticksPerQuarterNote * 4;

    const Tick first = (viewport.firstTick / barTicks) * barTicks;
    const Tick last  = viewport.firstTick + viewport.visibleTicks;

    for (Tick tick = first; tick <= last; tick += barTicks) {
        const CGFloat x = headerWidth + _model->tickToX(tick);
        if (x < headerWidth)
            continue;

        const bool fourBar = (tick / barTicks) % 4 == 0;
        fillRect(NSMakeRect(x, y, 1.0, height), fourBar ? grey(0.24) : grey(0.175));
    }
}

- (void)drawClips
{
    _model->collectVisibleClips(*_project, _visible);

    for (const auto& clip : _visible) {
        const NSRect rect = NSMakeRect(clip.rect.x + headerWidth, clip.rect.y + rulerHeight,
                                       std::max(2.0, clip.rect.width), clip.rect.height);

        fillRect(rect, colourFrom(clip.colour, clip.muted ? 0.45 : 0.75));

        // A lighter cap at the top, so overlapping clips stay readable and the
        // name has something to sit on.
        fillRect(NSMakeRect(rect.origin.x, rect.origin.y, rect.size.width, 14.0),
                 colourFrom(clip.colour, clip.muted ? 0.55 : 1.0));

        const project::Clip& model = _project->clips()[clip.index];
        drawText(@(model.name.c_str()),
                 NSMakeRect(rect.origin.x + 4.0, rect.origin.y + 1.0,
                            std::max(0.0, rect.size.width - 8.0), 13.0),
                 grey(0.08), 10.0);

        if (clip.selected) {
            [grey(1.0) setStroke];
            NSFrameRect(rect);
        }
    }
}

- (void)drawPlayhead
{
    if (_playheadTick < 0)
        return;

    const CGFloat x = headerWidth + _model->tickToX(static_cast<Tick>(_playheadTick));
    if (x < headerWidth || x > self.bounds.size.width)
        return;

    fillRect(NSMakeRect(x, 0, 1.5, self.bounds.size.height),
             [NSColor colorWithCalibratedRed:0.95 green:0.85 blue:0.35 alpha:1.0]);
}

// ── Header geometry ──────────────────────────────────────────────────────────

- (NSRect)muteRectForRow:(std::size_t)row
{
    const CGFloat y = rulerHeight + _model->trackY(_project->tracks(), row);
    return NSMakeRect(headerWidth - 2.0 * buttonWidth - 2.0 * padding, y + 5.0, buttonWidth, 16.0);
}

- (NSRect)soloRectForRow:(std::size_t)row
{
    const NSRect mute = [self muteRectForRow:row];
    return NSMakeRect(mute.origin.x + buttonWidth + padding, mute.origin.y, buttonWidth, 16.0);
}

- (NSRect)addTrackRect
{
    const CGFloat y = rulerHeight
                    + app::PlaylistModel::tracksHeight(_project->tracks())
                    - _model->viewport().firstTrackY;

    return NSMakeRect(0, y, self.bounds.size.width, addRowHeight);
}

// ── Input ────────────────────────────────────────────────────────────────────

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint view = [self convertPoint:event.locationInWindow fromView:nil];
    const NSPoint grid = [self gridPointFor:event];

    _drag              = PlaylistDrag::none;
    _dragOrigin        = grid;
    _dragCurrent       = grid;
    _dragAppliedTicks  = 0;
    _dragAppliedTracks = 0;
    _dragAppliedLength = 0;

    if (view.y < rulerHeight) {
        if (self.onSeekTick != nil && view.x >= headerWidth)
            self.onSeekTick(static_cast<long long>(std::max<Tick>(0, _model->xToTick(grid.x))));

        return;
    }

    if (view.x < headerWidth) {
        [self headerClickAt:view event:event];
        return;
    }

    const std::size_t index = _model->clipAtPoint(*_project, grid.x, grid.y);

    if (index == app::PlaylistModel::noClip) {
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
            _drag = PlaylistDrag::boxSelect;
            return;
        }

        _model->clearSelection();
        [self placeClipAt:grid];
        return;
    }

    const project::EntityId clipId = _project->clips()[index].id;

    if ((event.modifierFlags & NSEventModifierFlagShift) != 0)
        _model->toggleSelection(clipId);
    else if (!_model->isSelected(clipId))
        _model->setSelection({clipId});

    _drag = _model->isOverResizeHandle(*_project, index, grid.x, grid.y) ? PlaylistDrag::resize
                                                                        : PlaylistDrag::move;

    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_drag == PlaylistDrag::none)
        return;

    const NSPoint grid = [self gridPointFor:event];
    _dragCurrent = grid;

    if (_drag == PlaylistDrag::boxSelect) {
        [self setNeedsDisplay:YES];
        return;
    }

    if (_model->selection().empty())
        return;

    // Deltas are computed against what has already been applied, so a drag
    // produces one merged command rather than a fight between absolute and
    // relative positions.
    const Tick wanted = _model->snapTick(_model->xToTick(grid.x) - _model->xToTick(_dragOrigin.x));

    if (_drag == PlaylistDrag::resize) {
        const Tick delta = wanted - _dragAppliedLength;
        if (delta == 0)
            return;

        if (_registry->executeMerging(std::make_unique<app::ResizeClipsCommand>(
                _model->selection(), delta))) {
            _dragAppliedLength = wanted;
            [self changed];
        }

        return;
    }

    const std::size_t fromRow = _model->trackAtY(_project->tracks(), _dragOrigin.y);
    const std::size_t toRow   = _model->trackAtY(_project->tracks(), grid.y);

    int trackDelta = 0;
    if (fromRow != app::PlaylistModel::noTrack && toRow != app::PlaylistModel::noTrack)
        trackDelta = static_cast<int>(toRow) - static_cast<int>(fromRow) - _dragAppliedTracks;

    const Tick tickDelta = wanted - _dragAppliedTicks;

    if (tickDelta == 0 && trackDelta == 0)
        return;

    if (_registry->executeMerging(std::make_unique<app::MoveClipsCommand>(
            _model->selection(), tickDelta, trackDelta))) {
        _dragAppliedTicks  = wanted;
        _dragAppliedTracks += trackDelta;
        [self changed];
    }
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;

    if (_drag == PlaylistDrag::boxSelect) {
        _model->clipsInRectangle(*_project,
                                 std::min(_dragOrigin.x, _dragCurrent.x),
                                 std::min(_dragOrigin.y, _dragCurrent.y),
                                 std::abs(_dragCurrent.x - _dragOrigin.x),
                                 std::abs(_dragCurrent.y - _dragOrigin.y),
                                 _boxed);

        _model->setSelection(_boxed);
    }

    _drag = PlaylistDrag::none;
    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint grid = [self gridPointFor:event];
    const NSPoint view = [self convertPoint:event.locationInWindow fromView:nil];

    if (view.x < headerWidth) {
        const std::size_t row = _model->trackAtY(_project->tracks(), grid.y);
        if (row == app::PlaylistModel::noTrack)
            return;

        [self showTrackMenuFor:_project->tracks()[row].id event:event];
        return;
    }

    const std::size_t index = _model->clipAtPoint(*_project, grid.x, grid.y);
    if (index == app::PlaylistModel::noClip)
        return;

    const project::EntityId clipId = _project->clips()[index].id;
    if (!_model->isSelected(clipId))
        _model->setSelection({clipId});

    [self showClipMenuWithEvent:event];
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

        _model->pruneSelection(*_project);
        [self changed];
        return;
    }

    if (character == NSDeleteCharacter || character == NSBackspaceCharacter
        || character == NSDeleteFunctionKey) {
        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::RemoveClipsCommand>(_model->selection()))) {
            _model->clearSelection();
            [self changed];
        }

        return;
    }

    if (character == ' ') {
        if (self.onTransportToggle != nil)
            self.onTransportToggle();

        return;
    }

    if (command && character == 'd' && !_model->selection().empty()) {
        [self duplicateSelection];
        return;
    }

    [super keyDown:event];
}

- (void)scrollWheel:(NSEvent*)event
{
    auto viewport = _model->viewport();

    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        const double factor = event.scrollingDeltaY > 0 ? 0.9 : 1.1;
        const auto   ticks  = static_cast<Tick>(static_cast<double>(viewport.visibleTicks) * factor);

        viewport.visibleTicks = std::clamp<Tick>(ticks, ticksPerQuarterNote * 4,
                                                 ticksPerQuarterNote * 4 * 512);
    } else {
        const double scale = _model->pointsPerTick();
        if (scale > 0.0)
            viewport.firstTick -= static_cast<Tick>(event.scrollingDeltaX / scale);

        viewport.firstTrackY -= event.scrollingDeltaY;
    }

    viewport.firstTick   = std::max<Tick>(0, viewport.firstTick);
    viewport.firstTrackY = std::max(0.0, viewport.firstTrackY);

    _model->setViewport(viewport);
    [self setNeedsDisplay:YES];
}

// ── Edits ────────────────────────────────────────────────────────────────────

- (void)headerClickAt:(NSPoint)view event:(NSEvent*)event
{
    const NSRect addRow = [self addTrackRect];
    if (view.y >= addRow.origin.y && view.y < addRow.origin.y + addRow.size.height) {
        [self addTrack];
        return;
    }

    const NSPoint grid = NSMakePoint(view.x - headerWidth, view.y - rulerHeight);
    const std::size_t row = _model->trackAtY(_project->tracks(), grid.y);
    if (row == app::PlaylistModel::noTrack)
        return;

    const project::Track& track = _project->tracks()[row];
    const project::EntityId trackId = track.id;

    if (NSPointInRect(view, [self muteRectForRow:row])) {
        [self commit:std::make_unique<app::SetTrackMutedCommand>(trackId, !track.muted)];
        return;
    }

    if (NSPointInRect(view, [self soloRectForRow:row])) {
        [self commit:std::make_unique<app::SetTrackSoloedCommand>(trackId, !track.soloed)];
        return;
    }

    if (event.clickCount == 2)
        [self renameTrack:trackId];
}

/// Clicking empty timeline places the current pattern there — the fastest way
/// to build an arrangement, and the reason the pattern list has a selection.
- (void)placeClipAt:(NSPoint)grid
{
    const std::size_t row = _model->trackAtY(_project->tracks(), grid.y);
    if (row == app::PlaylistModel::noTrack)
        return;

    const project::EntityId pattern{_patternIdValue};
    if (_project->findPattern(pattern) == nullptr)
        return;

    const Tick start = _model->snapTick(std::max<Tick>(0, _model->xToTick(grid.x)));

    auto command = std::make_unique<app::AddPatternClipCommand>(_project->tracks()[row].id,
                                                                pattern, start);
    app::AddPatternClipCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    _model->setSelection({raw->clipId()});
    [self changed];
}

- (void)duplicateSelection
{
    // Offset by the width of the widest selected clip, so the copy lands after
    // the original rather than on top of it.
    Tick offset = 0;
    for (const project::EntityId id : _model->selection())
        if (const project::Clip* clip = _project->findClip(id))
            offset = std::max(offset, clip->lengthTicks);

    auto command = std::make_unique<app::DuplicateClipsCommand>(_model->selection(), offset);
    app::DuplicateClipsCommand* raw = command.get();

    if (!_registry->execute(std::move(command)))
        return;

    _model->setSelection(raw->createdClips());
    [self changed];
}

- (void)showClipMenuWithEvent:(NSEvent*)event
{
    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* duplicate = [menu addItemWithTitle:@"Duplicate"
                                            action:@selector(duplicateFromMenu:)
                                     keyEquivalent:@""];
    duplicate.target = self;

    NSMenuItem* mute = [menu addItemWithTitle:@"Mute"
                                       action:@selector(muteFromMenu:)
                                keyEquivalent:@""];
    mute.target = self;

    NSMenuItem* unmute = [menu addItemWithTitle:@"Unmute"
                                         action:@selector(unmuteFromMenu:)
                                  keyEquivalent:@""];
    unmute.target = self;

    [menu addItem:[NSMenuItem separatorItem]];

    NSMenuItem* remove = [menu addItemWithTitle:@"Remove"
                                         action:@selector(removeFromMenu:)
                                  keyEquivalent:@""];
    remove.target = self;

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)showTrackMenuFor:(project::EntityId)trackId event:(NSEvent*)event
{
    NSMenu* menu = [[NSMenu alloc] init];

    NSMenuItem* rename = [menu addItemWithTitle:@"Rename Track…"
                                         action:@selector(renameTrackFromMenu:)
                                  keyEquivalent:@""];
    rename.target = self;
    rename.representedObject = @(trackId.value());

    NSMenuItem* remove = [menu addItemWithTitle:@"Remove Track"
                                         action:@selector(removeTrackFromMenu:)
                                  keyEquivalent:@""];
    remove.target = self;
    remove.representedObject = @(trackId.value());

    [NSMenu popUpContextMenu:menu withEvent:event forView:self];
}

- (void)duplicateFromMenu:(id)sender { (void)sender; [self duplicateSelection]; }

- (void)muteFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipMutedCommand>(_model->selection(), true)];
}

- (void)unmuteFromMenu:(id)sender
{
    (void)sender;
    [self commit:std::make_unique<app::SetClipMutedCommand>(_model->selection(), false)];
}

- (void)removeFromMenu:(id)sender
{
    (void)sender;

    if (_registry->execute(std::make_unique<app::RemoveClipsCommand>(_model->selection()))) {
        _model->clearSelection();
        [self changed];
    }
}

- (void)renameTrackFromMenu:(NSMenuItem*)item
{
    [self renameTrack:project::EntityId{[item.representedObject unsignedLongLongValue]}];
}

- (void)removeTrackFromMenu:(NSMenuItem*)item
{
    const project::EntityId trackId{[item.representedObject unsignedLongLongValue]};

    [self commit:std::make_unique<app::RemoveTrackCommand>(trackId)];
    _model->pruneSelection(*_project);
}

- (void)addTrack
{
    auto command = std::make_unique<app::AddTrackCommand>(
        "Track " + std::to_string(_project->tracks().size() + 1));

    [self commit:std::move(command)];
}

- (void)renameTrack:(project::EntityId)trackId
{
    const project::Track* track = _project->findTrack(trackId);
    if (track == nullptr)
        return;

    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText = @"Rename track";
    [alert addButtonWithTitle:@"Rename"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 24)];
    field.stringValue = @(track->name.c_str());
    alert.accessoryView = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    [self commit:std::make_unique<app::RenameTrackCommand>(trackId, field.stringValue.UTF8String)];
}

- (void)commit:(app::CommandPtr)command
{
    if (_registry->execute(std::move(command)))
        [self changed];
}

- (void)changed
{
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

- (void)setPatternIdValue:(unsigned long long)value
{
    _patternIdValue = value;
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
