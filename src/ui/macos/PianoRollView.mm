#include "ui/macos/PianoRollView.h"

#include "ui/macos/Theme.h"

#include "app/CommandRegistry.h"
#include "app/MusicTheory.h"
#include "app/PianoRollModel.h"
#include "app/commands/ChordCommands.h"
#include "app/commands/NoteCommands.h"
#include "app/commands/NoteToolCommands.h"
#include "ui/macos/PianoRollRenderer.h"

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
                  double red, double green, double blue, double alpha = 1.0,
                  double radius = 0.0)
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
    rectangle.radius = static_cast<float>(radius);
    return rectangle;
}

/// The Piano Roll draws on the GPU and cannot call into the AppKit drawing
/// helpers, so palette entries are unpacked into components once and used as
/// numbers. The colours themselves still come from theme::ink — there is one
/// palette in the application, not two.
struct Rgb {
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
};

Rgb components(NSColor* colour)
{
    NSColor* srgb = [colour colorUsingColorSpace:NSColorSpace.sRGBColorSpace];
    if (srgb == nil)
        return {};

    return {static_cast<double>(srgb.redComponent),
            static_cast<double>(srgb.greenComponent),
            static_cast<double>(srgb.blueComponent)};
}

ui::Rect makeRect(double x, double y, double width, double height, Rgb colour,
                  double alpha = 1.0, double radius = 0.0)
{
    return makeRect(x, y, width, height, colour.red, colour.green, colour.blue, alpha, radius);
}

enum class DragMode { none, move, resize, boxSelect };

/// The stamp palette: which chord an Option-click lays down. Cycled with the
/// number keys; the suffixes address app::music::chordDictionary().
constexpr std::array<const char*, 8> stampSuffixes = {
    "", "m", "7", "maj7", "m7", "sus4", "dim", "add9",
};

} // namespace

@implementation INCDAWPianoRollView {
    project::Project*     _project;
    app::CommandRegistry* _registry;

    std::unique_ptr<app::PianoRollModel>   _model;
    std::unique_ptr<ui::PianoRollRenderer> _renderer;

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

    // Chord tools (docs/FL2026_GAP.md P1/P2).
    std::size_t       _stampChordIndex;
    BOOL              _stampTopDown;
    int               _keyRootPc;      ///< nudge key signature root, 0 = C
    app::music::Scale _scale;
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
    _renderer = std::make_unique<ui::PianoRollRenderer>();

    _dragMode         = DragMode::none;
    _dragAppliedTicks = 0;
    _dragAppliedKeys  = 0;
    _gestureActive    = NO;
    _playheadTick     = -1;
    _statusText       = @"Ready";
    _stampChordIndex  = 0;
    _stampTopDown     = NO;
    _keyRootPc        = 0;
    _scale            = app::music::Scale::major;

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

/// Notes the editor is currently looking at.
///
/// A pattern that has never been programmed on this channel has no content
/// block yet, and that is not an error — it is an empty editor. The add-note
/// command creates the block when the first note arrives.
- (const std::vector<project::MidiEvent>&)currentNotes
{
    static const std::vector<project::MidiEvent> empty;

    const project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr)
        return empty;

    const std::vector<project::MidiEvent>* notes =
        pattern->events(project::EntityId{_channelIdValue});
    return notes != nullptr ? *notes : empty;
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
    namespace theme = incdaw::ui::theme;
    using theme::Ink;

    _rectangles.clear();

    const auto&  viewport = _model->viewport();
    const double rowHeight = _model->keyHeight();
    const double gridWidth = viewport.width;

    const Rgb whiteRow  = components(theme::mix(theme::ink(Ink::panel),
                                                theme::ink(Ink::panelRaised), 0.35));
    const Rgb blackRow  = components(theme::darken(theme::ink(Ink::panel), 0.45));
    const Rgb whiteKey  = components(theme::ink(Ink::textPrimary));
    const Rgb blackKey  = components(theme::darken(theme::ink(Ink::panelSunken), 0.4));
    const Rgb octave    = components(theme::ink(Ink::accent));
    const Rgb beatLine  = components(theme::ink(Ink::gridLine));
    const Rgb barLine   = components(theme::ink(Ink::gridLineStrong));
    const Rgb selection = components(theme::ink(Ink::selectionStroke));
    const Rgb playhead  = components(theme::ink(Ink::playhead));

    // Notes carry the channel's colour, the way a channel is identified
    // everywhere else in the window.
    const project::Channel* channel = _project != nullptr
        ? _project->findChannel(project::EntityId{_channelIdValue})
        : nullptr;

    const Rgb noteColour = components(theme::fromArgb(channel != nullptr ? channel->colour
                                                                        : 0xFF3AA9FFu));
    const Rgb selectedNote = components(theme::lighten(theme::ink(Ink::accent), 0.35));

    // The keyboard's own ground, so that the dark keys read as keys rather than
    // as gaps in the strip.
    _rectangles.push_back(makeRect(0.0, 0.0, keyboardWidth, viewport.height,
                                   components(theme::ink(Ink::panelRaised))));

    // Key rows. Black-key rows are darker, which is what makes the pitch axis
    // readable without labelling every line.
    for (int key = viewport.lowestKey; key < viewport.lowestKey + viewport.visibleKeys; ++key) {
        const double y = _model->keyToY(key);
        const bool black = isBlackKey(key);

        _rectangles.push_back(makeRect(keyboardWidth, y, gridWidth, rowHeight,
                                       black ? blackRow : whiteRow));

        // Octave separators, so twelve rows read as one octave.
        if (key % 12 == 0)
            _rectangles.push_back(makeRect(keyboardWidth, y, gridWidth, 1.0, barLine, 0.55));

        // Keyboard strip: white keys full width, black keys short and dark, the
        // proportions of a keyboard rather than a colour-coded list.
        const double keyWidth = black ? (keyboardWidth - 2.0) * 0.62 : keyboardWidth - 2.0;

        _rectangles.push_back(makeRect(0.0, y + 0.5, keyWidth, rowHeight - 1.0,
                                       black ? blackKey : whiteKey, 1.0,
                                       std::min(2.5, rowHeight / 3.0)));

        // C rows get a marker, so octaves are countable at a glance.
        if (key % 12 == 0)
            _rectangles.push_back(makeRect(keyboardWidth - 7.0, y + 1.5, 4.0,
                                           std::max(1.0, rowHeight - 3.0), octave, 1.0, 1.5));
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
        _rectangles.push_back(makeRect(x, 0.0, isBar ? 1.5 : 1.0, viewport.height,
                                       isBar ? barLine : beatLine));
    }

    // Notes.
    {
        _model->collectVisibleNotes([self currentNotes], _visible);

        for (const auto& visible : _visible) {
            // Velocity drives brightness: the loudest information in a pattern
            // should be visible without opening an editor for it.
            const double intensity = 0.45 + 0.55 * (static_cast<double>(visible.velocity) / 127.0);

            const Rgb base = visible.selected ? selectedNote : noteColour;
            const double scale = visible.selected ? 1.0 : intensity;

            const double x = keyboardWidth + visible.x;
            const double y = visible.y + 1.0;
            const double width  = std::max(2.0, visible.width - 1.0);
            const double height = std::max(2.0, visible.height - 2.0);
            const double radius = std::min(3.0, std::min(width, height) / 2.0);

            _rectangles.push_back(makeRect(x, y, width, height,
                                           {base.red * scale, base.green * scale,
                                            base.blue * scale}, 1.0, radius));

            // A lit top edge: the same treatment a step pad gets, so a note and
            // a step read as the same material.
            if (height >= 6.0)
                _rectangles.push_back(makeRect(x + 1.0, y + 1.0, std::max(1.0, width - 2.0),
                                               height * 0.32, {1.0, 1.0, 1.0}, 0.16, radius));
        }
    }

    // Box selection overlay.
    if (_dragMode == DragMode::boxSelect) {
        const double x = std::min(_dragOrigin.x, _dragCurrent.x);
        const double y = std::min(_dragOrigin.y, _dragCurrent.y);
        const double width  = std::abs(_dragCurrent.x - _dragOrigin.x);
        const double height = std::abs(_dragCurrent.y - _dragOrigin.y);

        _rectangles.push_back(makeRect(x, y, width, height, selection, 0.22, 3.0));
    }

    // Playhead.
    if (_playheadTick >= 0) {
        const double x = keyboardWidth + _model->tickToX(_playheadTick);
        if (x >= keyboardWidth && x <= keyboardWidth + gridWidth)
            _rectangles.push_back(makeRect(x, 0.0, 2.0, viewport.height, playhead));
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

    const std::size_t hit = _model->noteAtPoint([self currentNotes], grid.x, grid.y);

    if (hit != app::PianoRollModel::noNote) {
        if ((event.modifierFlags & NSEventModifierFlagShift) != 0) {
            _model->toggleSelection(hit);
        } else if (!_model->isSelected(hit)) {
            _model->setSelection({hit});
        }

        _dragMode = _model->isOverResizeHandle([self currentNotes], hit, grid.x, grid.y)
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

    // Option-click stamps the current chord, clicked key as root — the Chord
    // Stamp gesture. The stamped notes become the selection so a drag can
    // place them, exactly like a freshly drawn note.
    if ((event.modifierFlags & NSEventModifierFlagOption) != 0) {
        [self stampChordAtTick:_model->snapTick(_model->xToTick(grid.x))
                           key:_model->yToKey(grid.y)];
        _dragMode = DragMode::move;
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

    if (note.tick < 0)
        note.tick = 0;

    auto command = std::make_unique<app::AddNoteCommand>(
        project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue}, note);
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
        _model->notesInRectangle([self currentNotes],
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
                project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                   _model->selection(), deltaTicks, deltaKeys))) {
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
                project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                   _model->selection(), delta))) {
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

    // Finishing a selection names what was selected — the Chord Panel's job.
    if (_dragMode == DragMode::boxSelect && !_model->selection().empty())
        [self reportChordOfSelection];

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
    const std::size_t hit = _model->noteAtPoint([self currentNotes], grid.x, grid.y);

    if (hit == app::PianoRollModel::noNote)
        return;

    if (_registry->execute(std::make_unique<app::DeleteNotesCommand>(
            project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
            std::vector<std::size_t>{hit}))) {
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

    // Cmd+Z normally never reaches here: the Edit menu owns it and routes it
    // through app::CommandRegistry for every pane at once. This remains as the
    // fallback for the case where the menu item is disabled — a field editor
    // has focus — and it must prune exactly like the shell's path does.
    if (command && (character == 'z' || character == 'Z')) {
        if (shift) {
            if (_registry->redo())
                [self reportAction:[NSString stringWithFormat:@"Redo"]];
        } else if (_registry->canUndo()) {
            NSString* name = @(_registry->undoName().c_str());
            if (_registry->undo())
                [self reportAction:[NSString stringWithFormat:@"Undo %@", name]];
        }

        [self pruneSelectionAfterHistoryChange];
        return;
    }

    if (character == NSDeleteCharacter || character == NSBackspaceCharacter
        || character == NSDeleteFunctionKey) {
        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::DeleteNotesCommand>(
                   project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                   _model->selection()))) {
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
                project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                _model->snap() > 0 ? _model->snap() : ticksPerQuarterNote / 4, 1.0)))
            [self reportAction:@"Quantize"];

        [self setNeedsDisplay:YES];
        return;
    }

    if (character == 'a' && command) {
        {
            const std::vector<project::MidiEvent>& notes = [self currentNotes];
            std::vector<std::size_t> all;
            for (std::size_t index = 0; index < notes.size(); ++index)
                all.push_back(index);

            _model->setSelection(std::move(all));
            [self reportAction:@"Select All"];
        }

        [self setNeedsDisplay:YES];
        return;
    }

    // ── Chord and note tools (docs/FL2026_GAP.md P1/P2) ─────────────────────

    if (!command && character >= '1'
        && character < static_cast<unichar>('1' + stampSuffixes.size())) {
        _stampChordIndex = static_cast<std::size_t>(character - '1');
        [self reportStampChoice];
        return;
    }

    if (!command && (character == 'v' || character == 'V')) {
        _stampTopDown = !_stampTopDown;
        [self reportStampChoice];
        return;
    }

    if (!command && (character == 'c' || character == 'C')) {
        [self reportChordOfSelection];
        return;
    }

    const Tick grid = _model->snap() > 0 ? _model->snap() : ticksPerQuarterNote / 4;

    if (!command && (character == 's' || character == 'S')) {
        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::StrumNotesCommand>(
                   project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                   _model->selection(), grid, shift)))
            [self reportAction:shift ? @"Strum Down" : @"Strum"];

        [self setNeedsDisplay:YES];
        return;
    }

    if (!command && (character == 'p' || character == 'P')) {
        const auto direction = shift ? app::ArpeggiateNotesCommand::Direction::upDown
                                     : app::ArpeggiateNotesCommand::Direction::up;

        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::ArpeggiateNotesCommand>(
                   project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                   _model->selection(), grid, direction))) {
            // The rewrite renumbered the event list; the old indices are gone.
            _model->clearSelection();
            [self reportAction:@"Arpeggiate"];
        }

        [self setNeedsDisplay:YES];
        return;
    }

    if (!command && (character == 'l' || character == 'L')) {
        if (!_model->selection().empty()
            && _registry->execute(std::make_unique<app::LegatoNotesCommand>(
                   project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                   _model->selection())))
            [self reportAction:@"Legato"];

        [self setNeedsDisplay:YES];
        return;
    }

    if (!command && (character == '[' || character == ']')) {
        const int steps = character == ']' ? 1 : -1;

        if (!_model->selection().empty()
            && _registry->executeMerging(std::make_unique<app::NudgeChordCommand>(
                   project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
                   _model->selection(), _keyRootPc, _scale, steps))) {
            [self reportChordOfSelection];
        }

        [self setNeedsDisplay:YES];
        return;
    }

    // F2, not R: bare R is the transport's record key, claimed by the menu
    // before any view sees it.
    if (character == NSF2FunctionKey) {
        [self renameSelectedNotes];
        return;
    }

    [super keyDown:event];
}

- (void)pruneSelectionAfterHistoryChange
{
    _model->pruneSelection([self currentNotes].size());
    [self setNeedsDisplay:YES];
}

- (void)requestRedraw
{
    _needsRedraw = YES;
}

// Retargeting clears the selection: indices refer to one channel's event list
// in one pattern, and carrying them across would delete or move notes the user
// never selected.
- (void)setPatternIdValue:(unsigned long long)value
{
    if (_patternIdValue == value)
        return;

    _patternIdValue = value;
    _model->clearSelection();
    [self setNeedsDisplay:YES];
}

- (void)setChannelIdValue:(unsigned long long)value
{
    if (_channelIdValue == value)
        return;

    _channelIdValue = value;
    _model->clearSelection();
    [self setNeedsDisplay:YES];
}

// ── Chord tools ──────────────────────────────────────────────────────────────

/// Keys of the selected note events, for detection and stamping context.
- (std::vector<int>)selectedNoteKeys
{
    const std::vector<project::MidiEvent>& notes = [self currentNotes];

    std::vector<int> keys;
    for (const std::size_t index : _model->selection())
        if (index < notes.size() && notes[index].type == project::MidiEventType::note)
            keys.push_back(notes[index].key);
    return keys;
}

/// The Chord Panel surface: name what is selected, in the status line.
- (void)reportChordOfSelection
{
    const std::vector<int> keys = [self selectedNoteKeys];
    if (keys.empty())
        return;

    const app::music::ChordDetection detection = app::music::detectChord(keys);
    if (!detection.display.empty())
        [self reportAction:[NSString stringWithFormat:@"Chord: %s", detection.display.c_str()]];
}

- (void)reportStampChoice
{
    const char* suffix = stampSuffixes[_stampChordIndex];
    [self reportAction:[NSString stringWithFormat:@"Stamp: C%s shape, %s", suffix,
                                                  _stampTopDown ? "top-down" : "bottom-up"]];
}

- (void)stampChordAtTick:(Tick)tick key:(int)key
{
    const app::music::ChordType* type =
        app::music::findChordType(stampSuffixes[_stampChordIndex]);
    if (type == nullptr)
        return;

    if (tick < 0)
        tick = 0;

    const Tick duration = _model->snap() > 0 ? _model->snap() * 4 : ticksPerQuarterNote;

    std::vector<project::MidiEvent> notes;
    for (const int chordKey :
         app::music::stampChord(key, *type,
                                _stampTopDown ? app::music::StampVoicing::topDown
                                              : app::music::StampVoicing::bottomUp)) {
        project::MidiEvent note;
        note.type     = project::MidiEventType::note;
        note.tick     = tick;
        note.key      = chordKey;
        note.duration = duration;
        note.value    = 100;
        notes.push_back(note);
    }

    auto  command = std::make_unique<app::InsertNotesCommand>(
        project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
        std::move(notes), "Stamp Chord");
    auto* raw = command.get();

    if (_registry->execute(std::move(command))) {
        std::vector<std::size_t> selection;
        for (std::size_t offset = 0; offset < raw->insertedCount(); ++offset)
            selection.push_back(raw->firstInsertedIndex() + offset);
        _model->setSelection(std::move(selection));

        const app::music::ChordDetection detection =
            app::music::detectChord([self selectedNoteKeys]);
        [self reportAction:[NSString stringWithFormat:@"Stamp %s", detection.display.c_str()]];
    }
}

/// One label for the whole selection, through a modal prompt. Prefilled with
/// the first selected note's current label so renaming is also editing.
- (void)renameSelectedNotes
{
    if (_model->selection().empty())
        return;

    const std::vector<project::MidiEvent>& notes = [self currentNotes];

    NSString* initial = @"";
    for (const std::size_t index : _model->selection()) {
        if (index < notes.size() && !notes[index].label.empty()) {
            initial = @(notes[index].label.c_str());
            break;
        }
    }

    NSAlert* alert    = [[NSAlert alloc] init];
    alert.messageText = @"Rename Notes";
    [alert addButtonWithTitle:@"Rename"];
    [alert addButtonWithTitle:@"Cancel"];

    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 24)];
    field.stringValue  = initial;
    alert.accessoryView = field;
    alert.window.initialFirstResponder = field;

    if ([alert runModal] != NSAlertFirstButtonReturn)
        return;

    if (_registry->execute(std::make_unique<app::SetNoteLabelCommand>(
            project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
            _model->selection(), std::string(field.stringValue.UTF8String))))
        [self reportAction:@"Rename Notes"];

    [self setNeedsDisplay:YES];
}

- (void)reportAction:(NSString*)action
{
    _statusText = [action copy];
    if (self.onChange != nil)
        self.onChange();
}

@end
