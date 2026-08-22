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
#import <QuartzCore/CATextLayer.h>
#import <QuartzCore/CATransaction.h>

#include <memory>
#include <vector>

using namespace incdaw;
using incdaw::engine::Tick;
using incdaw::engine::ticksPerQuarterNote;

namespace {

/// Width of the keyboard strip down the left edge, in points.
constexpr double keyboardWidth = 64.0;

/// Height of the velocity lane, in points. Tall enough that the 1..127 range
/// has resolution a hand can aim at, short enough not to compete with the grid.
constexpr double velocityLaneExtent = 88.0;

/// The grid never shrinks below this to make room for the lane. In a short
/// window the lane gives way instead — losing the notes to keep the lane would
/// be the wrong trade in every case.
constexpr double minimumGridHeight = 140.0;

/// The numbered band across the top, where bars are counted. Every other pane
/// with a time axis has one; the Piano Roll was the last without.
constexpr double rulerExtent = 22.0;

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

enum class DragMode { none, move, resize, boxSelect, velocity };

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
    std::vector<app::PianoRollModel::VisibleNote> _ghosts;
    std::vector<app::PianoRollModel::VelocityBar> _bars;
    std::vector<std::size_t>                     _hits;

    /// Text the renderer cannot draw.
    ///
    /// PianoRollRenderer draws one primitive and it is a rectangle, which is
    /// what makes ten thousand notes cost one draw call. Bar numbers and key
    /// names are the two things in this pane that are not rectangles, so they
    /// ride as CATextLayers over the Metal layer — a pool, repositioned rather
    /// than rebuilt, because allocating layers per frame would undo the point
    /// of the renderer.
    NSMutableArray<CATextLayer*>* _labels;
    NSUInteger                    _labelsUsed;

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

    /// The velocity lane, and the drag that edits it.
    ///
    /// The targets are snapshotted when the drag starts rather than read from
    /// the selection each move: SetVelocityCommand merges only across identical
    /// index lists, and a selection that changed mid-drag would break the merge
    /// and leave one undo entry per mouse move.
    BOOL                     _velocityLaneVisible;
    BOOL                     _ghostsVisible;
    std::vector<std::size_t> _velocityTargets;
    int                      _dragAppliedVelocity;

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

    // Shown by default. A velocity lane that has to be discovered is a velocity
    // lane nobody uses, and the grid is still the larger half of the view.
    _velocityLaneVisible = YES;
    _dragAppliedVelocity = -1;

    // Ghost notes: what the pattern's other channels are playing, behind what
    // this one is. Writing a counter-line against a part you cannot see is
    // guesswork, which is why every piano roll worth using shows them.
    _ghostsVisible = YES;

    _labels     = [NSMutableArray array];
    _labelsUsed = 0;
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

    [self applyViewportGeometry];

    _metalLayer.frame = self.bounds;
    [self updateDrawableSize];

    [self setNeedsDisplay:YES];
}

/// Divides the view's height between the note grid and the velocity lane.
///
/// The lane is taken out of the grid's share rather than added to the view's:
/// opening it must not resize the window, and it must not be able to squeeze
/// the grid down to nothing in a short one.
- (void)applyViewportGeometry
{
    const NSSize size = self.bounds.size;
    const double want = _velocityLaneVisible ? velocityLaneExtent : 0.0;

    auto viewport = _model->viewport();
    viewport.width       = size.width - keyboardWidth;
    viewport.rulerHeight = rulerExtent;

    const double belowRuler = size.height - viewport.rulerHeight;
    viewport.height = std::max(minimumGridHeight, belowRuler - want);

    // Whatever is genuinely left over, which is less than `want` in a window
    // too short to give the grid its minimum.
    viewport.velocityLaneHeight = std::max(0.0, belowRuler - viewport.height);

    _model->setViewport(viewport);
}

- (void)toggleVelocityLane
{
    _velocityLaneVisible = !_velocityLaneVisible;
    [self applyViewportGeometry];

    // Not reportAction: showing a lane is not an edit, and telling the shell
    // it was one would rebuild the render graph for a view change.
    _statusText = _velocityLaneVisible ? @"Velocity lane shown" : @"Velocity lane hidden";

    [self editorStateChanged];
    [self setNeedsDisplay:YES];
}

/// Tells whoever is showing these settings that one of them moved.
///
/// Only for changes the editor makes to itself — a keystroke. The setters below
/// do not call it: whoever set the value already knows.
- (void)editorStateChanged
{
    if (self.onEditorStateChanged != nil)
        self.onEditorStateChanged();
}

// ── Settings the control strip shows ─────────────────────────────────────────
//
// Each reads and writes the state the editor already had. There is deliberately
// no second copy: the strip is a view of the editor's settings, not a place
// they live, or the two would drift the first time a keystroke changed one.

- (long long)snapTicks { return static_cast<long long>(_model->snap()); }

- (void)setSnapTicks:(long long)ticks
{
    _model->setSnap(static_cast<Tick>(ticks > 0 ? ticks : 0));
    [self setNeedsDisplay:YES];
}

- (int)keyRootPitchClass { return _keyRootPc; }

- (void)setKeyRootPitchClass:(int)pitchClass
{
    _keyRootPc = ((pitchClass % 12) + 12) % 12;
    [self setNeedsDisplay:YES];
}

- (int)scaleIndex { return static_cast<int>(_scale); }

- (void)setScaleIndex:(int)index
{
    _scale = index >= 0 && index <= static_cast<int>(app::music::Scale::harmonicMinor)
                 ? static_cast<app::music::Scale>(index)
                 : app::music::Scale::major;

    [self setNeedsDisplay:YES];
}

- (BOOL)ghostNotesVisible { return _ghostsVisible; }

- (void)setGhostNotesVisible:(BOOL)visible
{
    _ghostsVisible = visible;
    [self setNeedsDisplay:YES];
}

- (BOOL)velocityLaneVisible { return _velocityLaneVisible; }

- (void)setVelocityLaneVisible:(BOOL)visible
{
    if (_velocityLaneVisible == visible)
        return;

    [self toggleVelocityLane];
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

    // Grid lines and the playhead run through the velocity lane as well: the
    // lane shares the grid's time axis, and a beat that stops at the boundary
    // makes the two read as unrelated panes.
    const double laneHeight  = viewport.velocityLaneHeight;
    const double totalHeight = _model->gridTop() + viewport.height + laneHeight;

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
    _rectangles.push_back(makeRect(0.0, _model->gridTop(), keyboardWidth, viewport.height,
                                   components(theme::ink(Ink::panelRaised))));

    // Key rows. Black-key rows are darker, which is what makes the pitch axis
    // readable without labelling every line.
    for (int key = viewport.lowestKey; key < viewport.lowestKey + viewport.visibleKeys; ++key) {
        const double y = _model->keyToY(key);
        const bool black = isBlackKey(key);

        _rectangles.push_back(makeRect(keyboardWidth, y, gridWidth, rowHeight,
                                       black ? blackRow : whiteRow));

        // Scale highlighting: rows outside the key signature are dimmed, so
        // the notes that belong are the ones the eye lands on. The key and
        // scale are the ones the nudge tool already works in ([ and ]), so the
        // grid and the tool can never disagree about what "in key" means.
        if (app::music::degreeOf(_keyRootPc, _scale, ((key % 12) + 12) % 12) < 0)
            _rectangles.push_back(makeRect(keyboardWidth, y, gridWidth, rowHeight,
                                           {0.0, 0.0, 0.0}, 0.22));

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

    // The velocity lane's ground, laid before the grid lines so they cross it.
    if (laneHeight > 0.0) {
        const double top = _model->velocityLaneTop();

        _rectangles.push_back(makeRect(0.0, top, keyboardWidth + gridWidth, laneHeight,
                                       components(theme::ink(Ink::panelSunken))));

        // A hard separator: without it the lowest key row and the lane merge
        // into one surface that has mysteriously lost its rows.
        _rectangles.push_back(makeRect(0.0, top, keyboardWidth + gridWidth, 1.0, barLine, 0.9));

        // The renderer draws rectangles and nothing else, so the scale cannot
        // be numbered. Three lines at 32, 64 and 96 give it instead.
        for (const int mark : {32, 64, 96}) {
            const double y = _model->velocityToY(mark);
            _rectangles.push_back(makeRect(keyboardWidth, y, gridWidth, 1.0, beatLine, 0.35));
        }
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
        _rectangles.push_back(makeRect(x, _model->gridTop(), isBar ? 1.5 : 1.0,
                                       totalHeight - _model->gridTop(),
                                       isBar ? barLine : beatLine));
    }

    // Ghost notes: the pattern's other channels, behind this one's. Drawn
    // before them and without a lit edge, so they read as context and are
    // never mistaken for something this editor can move.
    if (_ghostsVisible) {
        [self collectGhostNotes];

        const Rgb ghost = components(theme::ink(Ink::textDim));

        for (const auto& visible : _ghosts) {
            const double x = keyboardWidth + visible.x;
            const double width  = std::max(2.0, visible.width - 1.0);
            const double height = std::max(2.0, visible.height - 2.0);

            _rectangles.push_back(makeRect(x, visible.y + 1.0, width, height, ghost, 0.30,
                                           std::min(3.0, std::min(width, height) / 2.0)));
        }
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

    // Velocity bars: one stem per note that STARTS in view, in the channel's
    // colour, carrying the same velocity-to-brightness rule the notes do — so a
    // bar and its note are recognisably the same object seen twice.
    if (_model->hasVelocityLane()) {
        _model->collectVelocityBars([self currentNotes], _bars);

        for (const auto& stem : _bars) {
            const double intensity = 0.45 + 0.55 * (static_cast<double>(stem.velocity) / 127.0);

            const Rgb    base  = stem.selected ? selectedNote : noteColour;
            const double scale = stem.selected ? 1.0 : intensity;

            const double x     = keyboardWidth + stem.x;
            const double width = std::max(2.0, stem.width - 1.0);

            _rectangles.push_back(makeRect(x, stem.top, width, std::max(1.0, stem.height - 1.0),
                                           {base.red * scale, base.green * scale,
                                            base.blue * scale}, 1.0, 1.5));

            // The cap at full brightness. It is the grab target the eye looks
            // for, and it is what makes a quiet note's bar visible at all when
            // the stem itself is only two or three points tall.
            _rectangles.push_back(makeRect(x, stem.top, width, 2.0, base, 1.0, 1.0));
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

    // The ruler band, over everything the grid drew and under the playhead.
    if (_model->hasRuler()) {
        const double height = _model->gridTop();

        _rectangles.push_back(makeRect(0.0, 0.0, keyboardWidth + gridWidth, height,
                                       components(theme::ink(Ink::panelRaised))));

        _rectangles.push_back(makeRect(0.0, height - 1.0, keyboardWidth + gridWidth, 1.0,
                                       barLine, 0.9));

        // A tick per beat, taller on the bar. The numbers are text and ride
        // above as layers; these are what makes the band readable without them.
        for (Tick tick = firstBeat; tick <= lastTick; tick += beat) {
            const double x = keyboardWidth + _model->tickToX(tick);
            if (x < keyboardWidth)
                continue;

            const bool isBar = (tick % bar) == 0;

            _rectangles.push_back(makeRect(x, isBar ? height * 0.35 : height * 0.6,
                                           isBar ? 1.5 : 1.0,
                                           isBar ? height * 0.65 : height * 0.4,
                                           isBar ? barLine : beatLine));
        }
    }

    // Playhead.
    if (_playheadTick >= 0) {
        const double x = keyboardWidth + _model->tickToX(_playheadTick);
        if (x >= keyboardWidth && x <= keyboardWidth + gridWidth)
            _rectangles.push_back(makeRect(x, 0.0, 2.0, totalHeight, playhead));
    }
}

/// Gathers what the pattern's other channels are playing into `_ghosts`.
- (void)collectGhostNotes
{
    _ghosts.clear();

    const project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr || _project == nullptr)
        return;

    for (const project::Channel& channel : _project->channels()) {
        if (channel.id.value() == _channelIdValue)
            continue;

        if (const std::vector<project::MidiEvent>* events = pattern->events(channel.id))
            _model->collectVisibleNotes(*events, _ghosts, /*append=*/true);
    }
}

// ── Text over the rectangles ─────────────────────────────────────────────────

/// Hands out one CATextLayer per call, recycling the pool rather than growing
/// it. `beginLabels` resets the cursor; `endLabels` hides whatever the frame
/// did not use, which is what keeps a scrolled-away bar number from lingering.
- (void)beginLabels
{
    _labelsUsed = 0;
}

- (CATextLayer*)nextLabelWithText:(NSString*)text
                            frame:(NSRect)frame
                           colour:(NSColor*)colour
                             size:(CGFloat)size
                        alignment:(NSString*)alignment
{
    CATextLayer* label = nil;

    if (_labelsUsed < _labels.count) {
        label = _labels[_labelsUsed];
    } else {
        label = [CATextLayer layer];
        label.contentsScale = _metalLayer.contentsScale;
        [_labels addObject:label];
        [_metalLayer addSublayer:label];
    }

    ++_labelsUsed;

    // Implicit animation would smear every label across the screen as the view
    // scrolls; these are readouts, not objects that move.
    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    label.hidden           = NO;
    label.string           = text;
    label.font             = (__bridge CFTypeRef)ui::theme::labelFont(size, NSFontWeightMedium);
    label.fontSize         = size;
    label.foregroundColor  = colour.CGColor;
    label.alignmentMode    = alignment;
    label.frame            = frame;

    [CATransaction commit];
    return label;
}

- (void)endLabels
{
    [CATransaction begin];
    [CATransaction setDisableActions:YES];

    for (NSUInteger index = _labelsUsed; index < _labels.count; ++index)
        _labels[index].hidden = YES;

    [CATransaction commit];
}

/// Bar numbers in the ruler, and the name of every C on the keyboard.
///
/// Those two, and nothing else: a label per beat would be a wall of digits at
/// this pitch, and a name per key would be one at any pitch. A C every octave
/// is what makes the keyboard countable.
- (void)layoutLabels
{
    namespace theme = incdaw::ui::theme;
    using theme::Ink;

    [self beginLabels];

    const auto&  viewport = _model->viewport();
    const double rowHeight = _model->keyHeight();

    if (_model->hasRuler()) {
        const Tick bar = ticksPerQuarterNote * 4;
        const Tick lastTick = viewport.firstTick + viewport.visibleTicks;
        const Tick firstBar = (viewport.firstTick / bar) * bar;

        for (Tick tick = firstBar; tick <= lastTick; tick += bar) {
            const double x = keyboardWidth + _model->tickToX(tick);
            if (x < keyboardWidth - 1.0)
                continue;

            [self nextLabelWithText:[NSString stringWithFormat:@"%lld", tick / bar + 1]
                              frame:NSMakeRect(x + 4.0, 3.0, 44.0, 13.0)
                             colour:theme::ink(Ink::textSecondary)
                               size:9.5
                          alignment:kCAAlignmentLeft];
        }
    }

    // Key names, only where a row is tall enough to hold one.
    if (rowHeight >= 9.0) {
        for (int key = viewport.lowestKey; key < viewport.lowestKey + viewport.visibleKeys; ++key) {
            if (key % 12 != 0)
                continue;

            [self nextLabelWithText:@(app::music::noteName(key).c_str())
                              frame:NSMakeRect(6.0, _model->keyToY(key) + (rowHeight - 11.0) / 2.0,
                                               keyboardWidth - 18.0, 11.0)
                             colour:theme::ink(Ink::panelSunken)
                               size:9.0
                          alignment:kCAAlignmentLeft];
        }
    }

    [self endLabels];
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
    [self layoutLabels];

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

    // The ruler is a label, not the top of the grid. Without this guard a
    // click on a bar number would draw a note on whatever key sits under it.
    if (_model->isInRuler(grid.y)) {
        _dragMode = DragMode::none;
        return;
    }

    // The lane is checked first: its y range is below the grid's, so nothing
    // here can be a note, and letting the grid's "empty space draws a note"
    // path see it would add a note every time someone aimed at a bar.
    if (_model->isInVelocityLane(grid.y)) {
        [self beginVelocityDragAt:grid];
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

/// Starts a velocity edit from a press in the lane.
- (void)beginVelocityDragAt:(NSPoint)grid
{
    const std::size_t hit = _model->barAtPoint([self currentNotes], grid.x, grid.y);

    if (hit == app::PianoRollModel::noNote) {
        // Missing a bar is not a request to deselect. The lane is a narrow
        // target, and an aimed click that lands between two stems should cost
        // the user nothing.
        _dragMode = DragMode::none;
        return;
    }

    // Editing a note that is already selected edits the whole selection — the
    // rule the grid already follows for move and resize. Editing one that is
    // not selects it first, so what is about to change is visible before it
    // changes.
    if (!_model->isSelected(hit))
        _model->setSelection({hit});

    _velocityTargets     = _model->selection();
    _dragAppliedVelocity = -1;
    _dragMode            = DragMode::velocity;

    [self applyVelocityAtY:grid.y];
    [self setNeedsDisplay:YES];
}

- (void)applyVelocityAtY:(double)y
{
    if (_velocityTargets.empty())
        return;

    const int velocity = _model->yToVelocity(y);
    if (velocity == _dragAppliedVelocity)
        return;

    // Recorded before the attempt, not after: a command that changes nothing
    // because every target already sits at this velocity returns false, and
    // retrying it on every mouse move would be work for no reason.
    _dragAppliedVelocity = velocity;

    // Merging turns the whole drag into one undo entry, exactly as a move does.
    if (_registry->executeMerging(std::make_unique<app::SetVelocityCommand>(
            project::EntityId{_patternIdValue}, project::EntityId{_channelIdValue},
            _velocityTargets, velocity))) {
        _gestureActive = YES;
        [self reportAction:[NSString stringWithFormat:@"Velocity %d", velocity]];
    }
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

    // The lane's drag is not clamped to the lane: dragging past its floor or
    // ceiling pins at 1 and 127 rather than abandoning the gesture, which is
    // what a hand overshooting an 88-point strip actually wants.
    if (_dragMode == DragMode::velocity) {
        [self applyVelocityAtY:[self gridPointFromEvent:event].y];
        [self setNeedsDisplay:YES];
        return;
    }

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

    _velocityTargets.clear();
    _dragAppliedVelocity = -1;

    [self setNeedsDisplay:YES];
}

- (void)rightMouseDown:(NSEvent*)event
{
    project::Pattern* pattern = [self currentPattern];
    if (pattern == nullptr)
        return;

    const NSPoint grid = [self gridPointFromEvent:event];

    // A right-click in the ruler or the lane is not a delete. yToKey would
    // answer with a key outside the visible range and find nothing, but relying
    // on that would make their safety an accident of the key arithmetic.
    if (_model->isInRuler(grid.y) || _model->isInVelocityLane(grid.y))
        return;

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

    // E for event lane, which is what FL Studio calls the strip this is.
    if (!command && (character == 'e' || character == 'E')) {
        [self toggleVelocityLane];
        return;
    }

    if (!command && (character == 'g' || character == 'G')) {
        _ghostsVisible = !_ghostsVisible;
        _statusText    = _ghostsVisible ? @"Ghost notes shown" : @"Ghost notes hidden";
        [self editorStateChanged];
        [self setNeedsDisplay:YES];
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
