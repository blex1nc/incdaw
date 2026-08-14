#include "ui/macos/PlaylistView.h"

#include "app/CommandRegistry.h"
#include "app/PlaylistModel.h"
#include "app/commands/ArrangementCommands.h"
#include "ui/macos/RectangleRenderer.h"

#import <QuartzCore/CADisplayLink.h>
#import <QuartzCore/CAMetalLayer.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

using namespace incdaw;
using incdaw::project::FrameCount;
using incdaw::project::FramePosition;

namespace {

/// Width of the track-header strip down the left edge, in points.
constexpr double headerWidth = 150.0;

ui::Rect makeRect(double x, double y, double width, double height,
                  double red, double green, double blue, double alpha = 1.0)
{
    ui::Rect rectangle;
    rectangle.x      = static_cast<float>(x);
    rectangle.y      = static_cast<float>(y);
    rectangle.width  = static_cast<float>(width);
    rectangle.height = static_cast<float>(height);
    rectangle.red    = static_cast<float>(red);
    rectangle.green  = static_cast<float>(green);
    rectangle.blue   = static_cast<float>(blue);
    rectangle.alpha  = static_cast<float>(alpha);
    return rectangle;
}

void unpackColour(std::uint32_t argb, double& red, double& green, double& blue)
{
    red   = static_cast<double>((argb >> 16) & 0xFFu) / 255.0;
    green = static_cast<double>((argb >> 8) & 0xFFu) / 255.0;
    blue  = static_cast<double>(argb & 0xFFu) / 255.0;
}

enum class DragMode { none, move, resize, boxSelect };

} // namespace

@implementation INCDAWPlaylistView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::PlaylistModel>     _model;
    std::unique_ptr<ui::RectangleRenderer>  _renderer;

    std::vector<app::PlaylistModel::VisibleClip> _visible;
    std::vector<ui::Rect>                        _rectangles;

    CAMetalLayer*   _metalLayer;
    CADisplayLink*  _displayLink;
    BOOL            _needsRedraw;

    DragMode        _dragMode;
    NSPoint         _dragOrigin;
    NSPoint         _dragCurrent;
    project::EntityId _dragClip;
    FramePosition   _dragStartFrame;
    FrameCount      _dragStartLength;
}

- (instancetype)initWithFrame:(NSRect)frame
                      project:(project::Project*)project
                     registry:(app::CommandRegistry*)registry
{
    if ((self = [super initWithFrame:frame]) == nil)
        return nil;

    _project  = project;
    _registry = registry;
    _model    = std::make_unique<app::PlaylistModel>();
    _renderer = std::make_unique<ui::RectangleRenderer>();

    _dragMode      = DragMode::none;
    _needsRedraw   = YES;
    _playheadFrame = -1;

    app::PlaylistModel::Viewport viewport;
    viewport.firstFrame    = 0;
    viewport.visibleFrames = static_cast<FrameCount>(project->tempoMap().frameForTick(
        engine::ticksPerQuarterNote * 32));                 // eight bars
    viewport.firstTrack    = 0;
    viewport.visibleTracks = 8;
    viewport.width         = frame.size.width - headerWidth;
    viewport.height        = frame.size.height;
    _model->setViewport(viewport);

    // Snap to the bar. The playlist's unit is the bar in a way the Piano Roll's
    // is not: clips are arrangement-sized, and a clip half a beat out of place
    // is a mistake rather than an expressive choice.
    _model->setSnap(static_cast<FrameCount>(
        project->tempoMap().frameForTick(engine::ticksPerQuarterNote * 4)));

    self.wantsLayer = YES;

    _metalLayer = [CAMetalLayer layer];
    _metalLayer.frame = frame;
    self.layer = _metalLayer;

    std::string error;
    if (!_renderer->initialise(_metalLayer, error))
        NSLog(@"INCDAW: playlist renderer unavailable: %s", error.c_str());

    return self;
}

- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];

    if (self.window == nil) {
        [_displayLink invalidate];
        _displayLink = nil;
        return;
    }

    if (_displayLink == nil) {
        _displayLink = [self displayLinkWithTarget:self selector:@selector(displayLinkFired:)];
        [_displayLink addToRunLoop:NSRunLoop.currentRunLoop forMode:NSRunLoopCommonModes];
    }

    _needsRedraw = YES;
    [self updateDrawableSize];
}

- (void)displayLinkFired:(CADisplayLink*)link
{
    (void)link;

    if (!_needsRedraw)
        return;

    _needsRedraw = NO;
    [self renderFrame];
}

- (void)requestRedraw { _needsRedraw = YES; }

- (void)setNeedsDisplay:(BOOL)needsDisplay
{
    [super setNeedsDisplay:needsDisplay];
    if (needsDisplay)
        _needsRedraw = YES;
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];

    auto viewport = _model->viewport();
    viewport.width  = size.width - headerWidth;
    viewport.height = size.height;
    _model->setViewport(viewport);

    _needsRedraw = YES;
    [self updateDrawableSize];
}

- (void)updateDrawableSize
{
    const CGFloat scale = self.window != nil ? self.window.backingScaleFactor : 2.0;
    _metalLayer.contentsScale = scale;
    _metalLayer.frame = self.bounds;
    _metalLayer.drawableSize = CGSizeMake(self.bounds.size.width * scale,
                                          self.bounds.size.height * scale);
}

// ── Drawing ───────────────────────────────────────────────────────────────────

- (void)buildRectangles
{
    _rectangles.clear();

    const auto&  viewport  = _model->viewport();
    const double rowHeight = _model->trackHeight();
    const double gridWidth = viewport.width;

    _rectangles.push_back(makeRect(0.0, 0.0, self.bounds.size.width, self.bounds.size.height,
                                   0.10, 0.10, 0.11));

    // Track rows and headers.
    const auto& tracks = _project->tracks();

    for (std::size_t row = viewport.firstTrack;
         row < tracks.size() && row < viewport.firstTrack + viewport.visibleTracks; ++row) {
        const double y = _model->trackToY(row);
        const bool   audible = _project->trackIsAudible(tracks[row].id);
        const double shade   = (row % 2 == 0) ? 0.145 : 0.135;

        _rectangles.push_back(makeRect(headerWidth, y, gridWidth, rowHeight - 1.0,
                                       shade, shade, shade + 0.01));

        double red = 0.0;
        double green = 0.0;
        double blue = 0.0;
        unpackColour(tracks[row].colour, red, green, blue);

        _rectangles.push_back(makeRect(0.0, y, headerWidth - 2.0, rowHeight - 1.0,
                                       0.17, 0.17, 0.18));
        _rectangles.push_back(makeRect(0.0, y, 4.0, rowHeight - 1.0,
                                       red, green, blue, audible ? 1.0 : 0.25));
    }

    // Bar lines. Drawn from the tempo map rather than from a frame count, so
    // they stay where the music is when the tempo changes.
    const auto barFrames = static_cast<FrameCount>(
        _project->tempoMap().frameForTick(engine::ticksPerQuarterNote * 4));

    if (barFrames > 0) {
        const FramePosition firstBar = (viewport.firstFrame / barFrames) * barFrames;
        const FramePosition lastBar  = viewport.firstFrame + static_cast<FramePosition>(viewport.visibleFrames);

        for (FramePosition bar = firstBar; bar <= lastBar; bar += barFrames) {
            const double x = headerWidth + _model->frameToX(bar);
            if (x < headerWidth)
                continue;

            const bool fourth = ((bar / barFrames) % 4) == 0;
            const double tone = fourth ? 0.30 : 0.20;
            _rectangles.push_back(makeRect(x, 0.0, 1.0, viewport.height, tone, tone, tone));
        }
    }

    // Clips.
    _model->collectVisibleClips(*_project, _visible);

    for (const auto& clip : _visible) {
        double red = 0.0;
        double green = 0.0;
        double blue = 0.0;
        unpackColour(clip.colour, red, green, blue);

        const double alpha = clip.muted ? 0.30 : 1.0;
        const double x     = headerWidth + clip.x;
        const double width = std::max(2.0, clip.width - 1.0);

        _rectangles.push_back(makeRect(x, clip.y + 2.0, width, clip.height - 5.0,
                                       red * 0.55, green * 0.55, blue * 0.55, alpha));

        // A title bar across the top of the clip, brighter when selected: the
        // clip body carries the colour, the bar carries the state.
        _rectangles.push_back(makeRect(x, clip.y + 2.0, width, 5.0,
                                       clip.selected ? 1.0 : red,
                                       clip.selected ? 0.85 : green,
                                       clip.selected ? 0.35 : blue, alpha));

        if (clip.locked)
            _rectangles.push_back(makeRect(x + width - 5.0, clip.y + 8.0, 3.0, 3.0,
                                           0.95, 0.95, 0.95, 0.8));
    }

    if (_dragMode == DragMode::boxSelect) {
        const double x = std::min(_dragOrigin.x, _dragCurrent.x);
        const double y = std::min(_dragOrigin.y, _dragCurrent.y);
        _rectangles.push_back(makeRect(headerWidth + x, y,
                                       std::abs(_dragCurrent.x - _dragOrigin.x),
                                       std::abs(_dragCurrent.y - _dragOrigin.y),
                                       0.4, 0.7, 1.0, 0.20));
    }

    if (_playheadFrame >= 0) {
        const double x = headerWidth + _model->frameToX(_playheadFrame);
        if (x >= headerWidth && x <= headerWidth + gridWidth)
            _rectangles.push_back(makeRect(x, 0.0, 2.0, viewport.height, 1.0, 0.85, 0.25));
    }
}

- (void)renderFrame
{
    if (!_renderer->isReady())
        return;

    [self buildRectangles];

    // One line, once, confirming the first frame had content. A blank surface
    // and a surface drawing nothing look identical from outside.
    static bool reportedFirstFrame = false;
    if (!reportedFirstFrame) {
        reportedFirstFrame = true;
        NSLog(@"INCDAW: playlist first frame — %lu rectangles, %lu clips visible",
              static_cast<unsigned long>(_rectangles.size()),
              static_cast<unsigned long>(_visible.size()));
    }

    [self updateDrawableSize];

    _renderer->draw(_metalLayer, _rectangles,
                    static_cast<float>(self.bounds.size.width),
                    static_cast<float>(self.bounds.size.height));
}

// ── Input ─────────────────────────────────────────────────────────────────────

- (NSPoint)gridPointFromEvent:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    return NSMakePoint(point.x - headerWidth, point.y);
}

- (void)mouseDown:(NSEvent*)event
{
    const NSPoint grid = [self gridPointFromEvent:event];

    _dragOrigin  = grid;
    _dragCurrent = grid;

    if (grid.x < 0.0) {
        [self toggleTrackAt:grid alternate:(event.modifierFlags & NSEventModifierFlagShift) != 0];
        return;
    }

    const project::EntityId hit = _model->clipAtPoint(*_project, grid.x, grid.y);

    if (!hit.isValid()) {
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
            _dragMode = DragMode::boxSelect;
            return;
        }

        [self placePatternAt:grid];
        return;
    }

    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        _model->toggleSelection(hit);
        [self setNeedsDisplay:YES];
        return;
    }

    if (!_model->isSelected(hit))
        _model->setSelection({hit});

    const project::Clip* clip = _project->findClip(hit);
    if (clip == nullptr)
        return;

    _dragClip        = hit;
    _dragStartFrame  = clip->start;
    _dragStartLength = clip->length;
    _dragMode        = _model->isOverResizeHandle(*_project, hit, grid.x, grid.y)
                           ? DragMode::resize
                           : DragMode::move;

    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    const NSPoint grid = [self gridPointFromEvent:event];
    _dragCurrent = grid;

    if (_dragMode == DragMode::boxSelect) {
        std::vector<project::EntityId> selected;
        _model->clipsInRectangle(*_project,
                                 std::min(_dragOrigin.x, grid.x), std::min(_dragOrigin.y, grid.y),
                                 std::abs(grid.x - _dragOrigin.x), std::abs(grid.y - _dragOrigin.y),
                                 selected);
        _model->setSelection(std::move(selected));
        [self setNeedsDisplay:YES];
        return;
    }

    if (!_dragClip.isValid())
        return;

    const FramePosition delta = _model->xToFrame(grid.x) - _model->xToFrame(_dragOrigin.x);

    if (_dragMode == DragMode::move) {
        const FramePosition target = _model->snapFrame(std::max<FramePosition>(0, _dragStartFrame + delta));

        // The track under the pointer, so a clip can be dragged between tracks
        // in the same gesture rather than needing a second one.
        const std::size_t row = _model->yToTrack(grid.y);
        project::EntityId track;

        if (row < _project->tracks().size())
            track = _project->tracks()[row].id;

        if (_registry->executeMerging(std::make_unique<app::MoveClipCommand>(_dragClip, target, track)))
            [self notifyChanged];

        return;
    }

    if (_dragMode == DragMode::resize) {
        const auto length = static_cast<FrameCount>(
            std::max<FramePosition>(1, static_cast<FramePosition>(_dragStartLength) + delta));

        const FramePosition snapped = _model->snapFrame(_dragStartFrame + static_cast<FramePosition>(length));
        const auto snappedLength    = static_cast<FrameCount>(std::max<FramePosition>(1, snapped - _dragStartFrame));

        if (_registry->executeMerging(std::make_unique<app::ResizeClipCommand>(_dragClip, snappedLength)))
            [self notifyChanged];
    }
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _dragMode = DragMode::none;
    _dragClip = {};
    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
    const NSPoint grid = [self gridPointFromEvent:event];
    const project::EntityId hit = _model->clipAtPoint(*_project, grid.x, grid.y);

    if (!hit.isValid())
        return;

    if (_registry->execute(std::make_unique<app::DeleteClipCommand>(hit))) {
        _model->pruneSelection(*_project);
        [self notifyChanged];
    }
}

- (void)placePatternAt:(NSPoint)grid
{
    const std::size_t row = _model->yToTrack(grid.y);
    if (row >= _project->tracks().size() || _patternIdValue == 0)
        return;

    const FramePosition start = _model->snapFrame(std::max<FramePosition>(0, _model->xToFrame(grid.x)));

    auto command = std::make_unique<app::AddPatternClipCommand>(
        _project->tracks()[row].id, project::EntityId{_patternIdValue}, start);

    app::AddPatternClipCommand* pointer = command.get();

    if (_registry->execute(std::move(command))) {
        _model->setSelection({pointer->createdClip()});
        [self notifyChanged];
    }
}

/// Mute a track, or solo it with shift.
- (void)toggleTrackAt:(NSPoint)grid alternate:(bool)alternate
{
    const std::size_t row = _model->yToTrack(grid.y);
    if (row >= _project->tracks().size())
        return;

    const project::Track& track = _project->tracks()[row];

    using Flag = app::SetTrackFlagCommand::Flag;
    const Flag flag  = alternate ? Flag::soloed : Flag::muted;
    const bool value = alternate ? !track.soloed : !track.muted;

    if (_registry->execute(std::make_unique<app::SetTrackFlagCommand>(track.id, flag, value)))
        [self notifyChanged];
}

- (void)scrollWheel:(NSEvent*)event
{
    auto viewport = _model->viewport();

    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        const double factor = event.scrollingDeltaY > 0 ? 0.9 : 1.1;
        const auto   frames = static_cast<FrameCount>(static_cast<double>(viewport.visibleFrames) * factor);

        viewport.visibleFrames = std::clamp<FrameCount>(frames, 4800, 48000LL * 600);
    } else {
        const double scale = _model->pointsPerFrame();
        if (scale > 0.0)
            viewport.firstFrame = std::max<FramePosition>(
                0, viewport.firstFrame - static_cast<FramePosition>(event.scrollingDeltaX / scale));

        const double rows = event.scrollingDeltaY / std::max(1.0, _model->trackHeight());
        const auto   step = static_cast<std::ptrdiff_t>(rows);

        if (step != 0) {
            const auto current = static_cast<std::ptrdiff_t>(viewport.firstTrack);
            viewport.firstTrack = static_cast<std::size_t>(std::max<std::ptrdiff_t>(0, current - step));
        }
    }

    _model->setViewport(viewport);
    [self setNeedsDisplay:YES];
}

- (void)keyDown:(NSEvent*)event
{
    const NSString* characters = event.charactersIgnoringModifiers;

    if (characters.length == 0)
        return;

    const unichar key = [characters characterAtIndex:0];

    if (key == ' ') {
        if (self.onTransportToggle != nil)
            self.onTransportToggle();

        return;
    }

    if ((key == 's' || key == 'S') && _playheadFrame >= 0) {
        [self splitSelectionAtPlayhead];
        return;
    }

    if ((key == 'd' || key == 'D')) {
        [self duplicateSelection];
        return;
    }

    if (key == NSDeleteCharacter || key == NSBackspaceCharacter) {
        [self deleteSelection];
        return;
    }

    [super keyDown:event];
}

- (void)splitSelectionAtPlayhead
{
    bool changed = false;

    for (const project::EntityId clip : _model->selection())
        changed |= _registry->execute(std::make_unique<app::SplitClipCommand>(clip, _playheadFrame));

    if (changed)
        [self notifyChanged];
}

- (void)duplicateSelection
{
    std::vector<project::EntityId> created;

    for (const project::EntityId clip : _model->selection()) {
        auto command = std::make_unique<app::DuplicateClipCommand>(clip);
        app::DuplicateClipCommand* pointer = command.get();

        if (_registry->execute(std::move(command)))
            created.push_back(pointer->createdClip());
    }

    if (created.empty())
        return;

    // The copies become the selection, so a second D duplicates those rather
    // than stacking another copy on the originals.
    _model->setSelection(std::move(created));
    [self notifyChanged];
}

- (void)deleteSelection
{
    bool changed = false;

    for (const project::EntityId clip : _model->selection())
        changed |= _registry->execute(std::make_unique<app::DeleteClipCommand>(clip));

    if (!changed)
        return;

    _model->clearSelection();
    [self notifyChanged];
}

- (void)notifyChanged
{
    [self setNeedsDisplay:YES];

    if (self.onChange != nil)
        self.onChange();
}

@end
