#include "ui/macos/PianoRollView.h"

#include "app/CommandRegistry.h"
#include "app/PianoRollModel.h"
#include "app/commands/NoteCommands.h"
#include "ui/macos/RectangleRenderer.h"

#import <QuartzCore/CAMetalLayer.h>
#import <QuartzCore/CADisplayLink.h>

#include <memory>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

/// Width of the keyboard strip down the left edge, in points.
constexpr double keyboardWidth = 64.0;

/// Which pitch classes are black keys. Used for both the keyboard strip and the
/// row shading behind the grid, so the two can never disagree.
bool isBlackKey(int key) noexcept
{
    switch (((key % 12) + 12) % 12) {
        case 1: case 3: case 6: case 8: case 10: return true;
        default: return false;
    }
}

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

enum class DragMode { none, move, resize, boxSelect };

} // namespace

@implementation INCDAWPianoRollView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::PianoRollModel>   _model;
    std::unique_ptr<ui::RectangleRenderer> _renderer;

    std::vector<ui::Rect>                        _rectangles;
    std::vector<app::PianoRollModel::VisibleNote> _visible;
    std::vector<std::size_t>                     _hits;

    CAMetalLayer* _metalLayer;
    NSString*     _rendererError;
    CADisplayLink* _displayLink;
    BOOL           _needsRedraw;

    DragMode   _dragMode;
    NSPoint    _dragOrigin;
    NSPoint    _dragCurrent;
    long long  _dragAppliedTicks;
    int        _dragAppliedKeys;
    BOOL       _gestureActive;
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
    _model    = std::make_unique<app::PianoRollModel>();
    _renderer = std::make_unique<ui::RectangleRenderer>();

    _dragMode         = DragMode::none;
    _dragAppliedTicks = 0;
    _dragAppliedKeys  = 0;
    _gestureActive    = NO;
    _playheadTick     = -1;
    _statusText       = @"Ready";

    app::PianoRollModel::Viewport viewport;
    viewport.firstTick    = 0;
    viewport.visibleTicks = ticksPerQuarterNote * 8;   // two bars
    viewport.lowestKey    = 48;
    viewport.visibleKeys  = 30;
    viewport.width        = frame.size.width - keyboardWidth;
    viewport.height       = frame.size.height;
    _model->setViewport(viewport);
    _model->setSnap(ticksPerQuarterNote / 4);          // sixteenths

    self.wantsLayer = YES;

    _metalLayer = [CAMetalLayer layer];
    _metalLayer.frame = frame;
    self.layer = _metalLayer;

    std::string error;
    if (_renderer->initialise(_metalLayer, error)) {
        NSLog(@"INCDAW: Metal renderer ready (%@)", _metalLayer.device.name);
    } else {
        // Reported rather than swallowed: a silently empty editor looks like a
        // hung application, and the cause is invisible.
        _rendererError = @(error.c_str());
        NSLog(@"INCDAW: renderer unavailable: %s", error.c_str());
    }

    return self;
}

// AppKit's default origin is bottom-left; flipping puts y at the top so the
// view agrees with app::PianoRollModel instead of converting at every call.
- (BOOL)isFlipped { return YES; }
- (BOOL)acceptsFirstResponder { return YES; }

// Assigning `self.layer` makes this a layer-HOSTING view, and AppKit never
// sends updateLayer or drawRect to one. Frames are therefore driven from a
// display link, which is what a GPU-rendered editor wants anyway: it paces to
// the actual refresh rate instead of to AppKit's idea of when to redraw.
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

    // Only submit when something changed. An editor that redraws an unchanged
    // view sixty times a second burns battery for nothing.
    if (!_needsRedraw)
        return;

    _needsRedraw = NO;
    [self renderFrame];
}

- (void)setNeedsDisplay:(BOOL)needsDisplay
{
    [super setNeedsDisplay:needsDisplay];
    if (needsDisplay)
        _needsRedraw = YES;
}

- (void)updateDrawableSize
{
    const CGFloat scale = self.window != nil ? self.window.backingScaleFactor : 2.0;
    _metalLayer.contentsScale = scale;
    _metalLayer.drawableSize = CGSizeMake(self.bounds.size.width * scale,
                                          self.bounds.size.height * scale);
}

- (void)setChannelIdValue:(unsigned long long)value
{
    _channelIdValue = value;

    if (_model == nullptr || _project == nullptr)
        return;

    _model->setChannelFilter(project::EntityId{value}, _project->defaultChannel());

    // Selecting a channel whose notes are off-screen — a kick, with the view
    // sitting where a melody was — would look like an empty pattern. Follow the
    // content instead of making the user go and find it.
    [self scrollToEditableContent];
    _model->clearSelection();
}

- (void)scrollToEditableContent
{
    const project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr)
        return;

    int lowest  = 128;
    int highest = -1;

    for (const project::MidiEvent& event : pattern->events) {
        if (event.type != project::MidiEventType::note || !_model->ownsNote(event))
            continue;

        lowest  = std::min(lowest, event.key);
        highest = std::max(highest, event.key);
    }

    if (highest < 0)
        return;   // nothing on this channel; leave the view where the user put it

    auto viewport = _model->viewport();

    if (lowest >= viewport.lowestKey && highest < viewport.lowestKey + viewport.visibleKeys)
        return;   // already on screen

    const int centre = (lowest + highest) / 2;
    viewport.lowestKey = std::clamp(centre - viewport.visibleKeys / 2, 0, 127 - viewport.visibleKeys);
    _model->setViewport(viewport);
}

- (project::Pattern*)currentPattern
{
    if (_project == nullptr)
        return nullptr;

    for (project::Pattern& pattern : _project->patterns())
        if (pattern.id.value() == _patternIdValue)
            return &pattern;

    return nullptr;
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];

    auto viewport = _model->viewport();
    viewport.width  = size.width - keyboardWidth;
    viewport.height = size.height;
    _model->setViewport(viewport);

    _metalLayer.frame = self.bounds;
    [self updateDrawableSize];

    [self setNeedsDisplay:YES];
}

// ── Drawing ──────────────────────────────────────────────────────────────────

- (void)buildRectangles
{
    _rectangles.clear();

    const auto&  viewport = _model->viewport();
    const double rowHeight = _model->keyHeight();
    const double gridWidth = viewport.width;

    // Key rows. Black-key rows are darker, which is what makes the pitch axis
    // readable without labelling every line.
    for (int key = viewport.lowestKey; key < viewport.lowestKey + viewport.visibleKeys; ++key) {
        const double y = _model->keyToY(key);
        const double shade = isBlackKey(key) ? 0.13 : 0.17;

        _rectangles.push_back(makeRect(keyboardWidth, y, gridWidth, rowHeight, shade, shade, shade + 0.01));

        // Keyboard strip.
        const bool black = isBlackKey(key);
        const double keyShade = black ? 0.10 : 0.86;
        _rectangles.push_back(makeRect(0.0, y + 0.5, keyboardWidth - 2.0, rowHeight - 1.0,
                                       keyShade, keyShade, keyShade));

        // C rows get a marker, so octaves are countable at a glance.
        if (key % 12 == 0)
            _rectangles.push_back(makeRect(keyboardWidth - 8.0, y + 1.0, 4.0, rowHeight - 2.0,
                                           0.35, 0.55, 0.80));
    }

    // Grid lines: beats faint, bars stronger.
    const Tick beat = ticksPerQuarterNote;
    const Tick bar  = ticksPerQuarterNote * 4;
    const Tick lastTick = viewport.firstTick + viewport.visibleTicks;

    const Tick firstBeat = (viewport.firstTick / beat) * beat;

    for (Tick tick = firstBeat; tick <= lastTick; tick += beat) {
        const double x = keyboardWidth + _model->tickToX(tick);
        if (x < keyboardWidth)
            continue;

        const bool isBar = (tick % bar) == 0;
        const double shade = isBar ? 0.34 : 0.23;
        _rectangles.push_back(makeRect(x, 0.0, isBar ? 1.5 : 1.0, viewport.height,
                                       shade, shade, shade));
    }

    // Notes.
    if (const project::Pattern* pattern = [self currentPattern]) {
        _model->collectVisibleNotes(*pattern, _visible);

        for (const auto& visible : _visible) {
            // Velocity drives brightness: the loudest information in a pattern
            // should be visible without opening an editor for it.
            const double intensity = 0.45 + 0.55 * (static_cast<double>(visible.velocity) / 127.0);

            const double red   = visible.selected ? 1.00 : 0.30 * intensity;
            const double green = visible.selected ? 0.80 : 0.72 * intensity;
            const double blue  = visible.selected ? 0.35 : 0.95 * intensity;

            // Another channel's note: context, not a target. Dimmed rather than
            // hidden, because what the rest of the pattern is doing is exactly
            // what you need to see while editing one part of it.
            const double alpha = visible.ghost ? 0.28 : 1.0;

            _rectangles.push_back(makeRect(keyboardWidth + visible.x, visible.y + 1.0,
                                           std::max(2.0, visible.width - 1.0),
                                           std::max(2.0, visible.height - 2.0),
                                           red, green, blue, alpha));
        }
    }

    // Box selection overlay.
    if (_dragMode == DragMode::boxSelect) {
        const double x = std::min(_dragOrigin.x, _dragCurrent.x);
        const double y = std::min(_dragOrigin.y, _dragCurrent.y);
        const double width  = std::abs(_dragCurrent.x - _dragOrigin.x);
        const double height = std::abs(_dragCurrent.y - _dragOrigin.y);

        _rectangles.push_back(makeRect(x, y, width, height, 0.4, 0.7, 1.0, 0.20));
    }

    // Playhead.
    if (_playheadTick >= 0) {
        const double x = keyboardWidth + _model->tickToX(_playheadTick);
        if (x >= keyboardWidth && x <= keyboardWidth + gridWidth)
            _rectangles.push_back(makeRect(x, 0.0, 2.0, viewport.height, 1.0, 0.85, 0.25));
    }
}

- (void)renderFrame
{
    if (!_renderer->isReady())
        return;

    [self buildRectangles];

    // One line, once, confirming the first frame actually had content. A blank
    // window and a window drawing nothing look identical from outside.
    static bool reportedFirstFrame = false;
    if (!reportedFirstFrame) {
        reportedFirstFrame = true;
        NSLog(@"INCDAW: first frame — %lu rectangles, %lu notes visible",
              static_cast<unsigned long>(_rectangles.size()),
              static_cast<unsigned long>(_visible.size()));
    }

    [self updateDrawableSize];

    _renderer->draw(_metalLayer, _rectangles,
                    static_cast<float>(self.bounds.size.width),
                    static_cast<float>(self.bounds.size.height));
}

// ── Input ────────────────────────────────────────────────────────────────────

/// Converts a view point into the grid's coordinate space, which is what
/// PianoRollModel expects — it knows nothing about the keyboard strip.
- (NSPoint)gridPointFromEvent:(NSEvent*)event
{
    const NSPoint point = [self convertPoint:event.locationInWindow fromView:nil];
    return NSMakePoint(point.x - keyboardWidth, point.y);
}

- (void)mouseDown:(NSEvent*)event
{
    project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr)
        return;

    const NSPoint viewPoint = [self convertPoint:event.locationInWindow fromView:nil];
    const NSPoint grid      = [self gridPointFromEvent:event];

    _dragOrigin       = viewPoint;
    _dragCurrent      = viewPoint;
    _dragAppliedTicks = 0;
    _dragAppliedKeys  = 0;
    _gestureActive    = NO;

    // Clicking the keyboard strip is not an edit.
    if (viewPoint.x < keyboardWidth) {
        _dragMode = DragMode::none;
        return;
    }

    const std::size_t hit = _model->noteAtPoint(*pattern, grid.x, grid.y);

    if (hit != app::PianoRollModel::noNote) {
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
            _model->toggleSelection(hit);
        } else if (!_model->isSelected(hit)) {
            _model->setSelection({hit});
        }

        _dragMode = _model->isOverResizeHandle(*pattern, hit, grid.x, grid.y)
                        ? DragMode::resize
                        : DragMode::move;

        [self setNeedsDisplay:YES];
        return;
    }

    if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
        _dragMode = DragMode::boxSelect;
        [self setNeedsDisplay:YES];
        return;
    }

    // Empty space: draw a note there. One click, one note — the same gesture
    // every step sequencer and piano roll uses.
    project::MidiEvent note;
    note.type     = project::MidiEventType::note;
    note.tick     = _model->snapTick(_model->xToTick(grid.x));
    note.key      = _model->yToKey(grid.y);
    note.duration = _model->snap() > 0 ? _model->snap() : ticksPerQuarterNote / 4;
    note.value    = 100;
    note.channelId = project::EntityId{_channelIdValue};

    if (note.tick < 0)
        note.tick = 0;

    auto command = std::make_unique<app::AddNoteCommand>(project::EntityId{_patternIdValue}, note);
    auto* raw = command.get();

    if (_registry->execute(std::move(command))) {
        _model->setSelection({raw->insertedIndex()});
        _dragMode = DragMode::move;
        [self reportAction:@"Add Note"];
    }

    [self setNeedsDisplay:YES];
}

- (void)mouseDragged:(NSEvent*)event
{
    if (_dragMode == DragMode::none)
        return;

    const NSPoint viewPoint = [self convertPoint:event.locationInWindow fromView:nil];
    _dragCurrent = viewPoint;

    project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr)
        return;

    if (_dragMode == DragMode::boxSelect) {
        _model->notesInRectangle(*pattern,
                                 _dragOrigin.x - keyboardWidth, _dragOrigin.y,
                                 viewPoint.x - _dragOrigin.x, viewPoint.y - _dragOrigin.y,
                                 _hits);
        _model->setSelection(_hits);
        [self setNeedsDisplay:YES];
        return;
    }

    if (_model->selection().empty())
        return;

    const double scale = _model->pointsPerTick();
    if (scale <= 0.0)
        return;

    const auto totalTicks = static_cast<long long>((viewPoint.x - _dragOrigin.x) / scale);
    const int  totalKeys  = _model->yToKey(viewPoint.y) - _model->yToKey(_dragOrigin.y);

    if (_dragMode == DragMode::move) {
        const long long snapped = _model->snap() > 0
            ? (totalTicks / _model->snap()) * _model->snap()
            : totalTicks;

        const long long deltaTicks = snapped - _dragAppliedTicks;
        const int       deltaKeys  = totalKeys - _dragAppliedKeys;

        if (deltaTicks == 0 && deltaKeys == 0)
            return;

        // Merging turns the whole drag into one undo entry rather than one per
        // mouse move.
        if (_registry->executeMerging(std::make_unique<app::MoveNotesCommand>(
                project::EntityId{_patternIdValue}, _model->selection(), deltaTicks, deltaKeys))) {
            _dragAppliedTicks = snapped;
            _dragAppliedKeys  = totalKeys;
            _gestureActive    = YES;
            [self reportAction:@"Move Notes"];
        }
    } else if (_dragMode == DragMode::resize) {
        const long long snapped = _model->snap() > 0
            ? (totalTicks / _model->snap()) * _model->snap()
            : totalTicks;

        const long long delta = snapped - _dragAppliedTicks;
        if (delta == 0)
            return;

        if (_registry->executeMerging(std::make_unique<app::ResizeNotesCommand>(
                project::EntityId{_patternIdValue}, _model->selection(), delta))) {
            _dragAppliedTicks = snapped;
            _gestureActive    = YES;
            [self reportAction:@"Resize Notes"];
        }
    }

    [self setNeedsDisplay:YES];
}

- (void)mouseUp:(NSEvent*)event
{
    (void)event;
    _dragMode      = DragMode::none;
    _gestureActive = NO;
    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
    project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr)
        return;

    const NSPoint grid = [self gridPointFromEvent:event];
    const std::size_t hit = _model->noteAtPoint(*pattern, grid.x, grid.y);

    if (hit == app::PianoRollModel::noNote)
        return;

    if (_registry->execute(std::make_unique<app::DeleteNotesCommand>(
            project::EntityId{_patternIdValue}, std::vector<std::size_t>{hit}))) {
        _model->clearSelection();
        [self reportAction:@"Delete Note"];
    }

    [self setNeedsDisplay:YES];
}

- (void)scrollWheel:(NSEvent*)event
{
    auto viewport = _model->viewport();

    if ((event.modifierFlags & NSEventModifierFlagCommand) != 0) {
        // Zoom, anchored so the music under the cursor stays put.
        const double factor = event.scrollingDeltaY > 0 ? 0.9 : 1.1;
        const auto   ticks  = static_cast<Tick>(static_cast<double>(viewport.visibleTicks) * factor);

        viewport.visibleTicks = std::clamp<Tick>(ticks, ticksPerQuarterNote / 2,
                                                 ticksPerQuarterNote * 128);
    } else {
        const double scale = _model->pointsPerTick();
        if (scale > 0.0)
            viewport.firstTick -= static_cast<Tick>(event.scrollingDeltaX / scale);

        viewport.lowestKey -= static_cast<int>(event.scrollingDeltaY / 8.0);
        viewport.lowestKey = std::clamp(viewport.lowestKey, 0, 127 - viewport.visibleKeys);
    }

    if (viewport.firstTick < 0)
        viewport.firstTick = 0;

    _model->setViewport(viewport);
    [self setNeedsDisplay:YES];
}

- (void)keyDown:(NSEvent*)event
{
    const unichar character = event.charactersIgnoringModifiers.length > 0
                                  ? [event.charactersIgnoringModifiers characterAtIndex:0]
                                  : 0;

    const bool command = (event.modifierFlags & NSEventModifierFlagCommand) != 0;
    const bool shift   = (event.modifierFlags & NSEventModifierFlagShift) != 0;

    if (command && (character == 'z' || character == 'Z')) {
        if (shift) {
            if (_registry->redo())
                [self reportAction:[NSString stringWithFormat:@"Redo"]];
        } else if (_registry->canUndo()) {
            NSString* name = @(_registry->undoName().c_str());
            if (_registry->undo())
                [self reportAction:[NSString stringWithFormat:@"Undo %@", name]];
        }

        if (const project::Pattern* pattern = [self currentPattern])
            _model->pruneSelection(pattern->events.size());

        [self setNeedsDisplay:YES];
        return;
    }

    if (character == NSDeleteCharacter || character == NSBackspaceCharacter
        || character == NSDeleteFunctionKey) {
        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::DeleteNotesCommand>(
                   project::EntityId{_patternIdValue}, _model->selection()))) {
            _model->clearSelection();
            [self reportAction:@"Delete Notes"];
        }

        [self setNeedsDisplay:YES];
        return;
    }

    if (character == ' ') {
        if (self.onTransportToggle != nil)
            self.onTransportToggle();
        return;
    }

    if (character == 'q' || character == 'Q') {
        if (_registry->execute(std::make_unique<app::QuantizeNotesCommand>(
                project::EntityId{_patternIdValue},
                _model->snap() > 0 ? _model->snap() : ticksPerQuarterNote / 4, 1.0)))
            [self reportAction:@"Quantize"];

        [self setNeedsDisplay:YES];
        return;
    }

    if (character == 'a' && command) {
        if (const project::Pattern* pattern = [self currentPattern]) {
            std::vector<std::size_t> all;
            for (std::size_t index = 0; index < pattern->events.size(); ++index)
                all.push_back(index);

            _model->setSelection(std::move(all));
            [self reportAction:@"Select All"];
        }

        [self setNeedsDisplay:YES];
        return;
    }

    [super keyDown:event];
}

- (void)requestRedraw
{
    _needsRedraw = YES;
}

- (void)reportAction:(NSString*)action
{
    _statusText = [action copy];
    if (self.onChange != nil)
        self.onChange();
}

@end
