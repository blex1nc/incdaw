// Where each widget of a DeviceUiSpec goes, in points, with no AppKit.
//
// The renderer (ui/macos/DevicePanel.mm) draws and hit-tests the rectangles
// this produces; it never places anything itself. Keeping the geometry here
// is what lets a spec be checked without a window server: that it fits its
// width, that nothing overlaps, that collapsing a section shortens the
// panel, that every parameter is reachable. Sixty panels will be written
// against the vocabulary, and this is the test surface they share.
//
// Coordinates are top-left, y growing DOWN (a flipped view). Every metric
// below is a constant of the design language, not of a device.

#pragma once

#include "app/devices/DeviceUiSpec.h"

#include <functional>
#include <vector>

namespace incdaw::app {

struct DeviceUiRect {
    double x      = 0.0;
    double y      = 0.0;
    double width  = 0.0;
    double height = 0.0;

    [[nodiscard]] double right() const noexcept { return x + width; }
    [[nodiscard]] double bottom() const noexcept { return y + height; }

    [[nodiscard]] bool contains(double px, double py) const noexcept
    {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    [[nodiscard]] bool overlaps(const DeviceUiRect& other) const noexcept
    {
        return x < other.right() && other.x < right() && y < other.bottom()
            && other.y < bottom();
    }
};

/// One placed widget. Containers are placed too (their `frame` spans their
/// children; a section's `control` is its clickable header), so the
/// renderer can draw headers and backgrounds without walking the tree.
struct DeviceUiPlacement {
    const DeviceUiWidget* widget = nullptr;
    DeviceUiRect          frame;      ///< the whole cell: caption, control, readout
    DeviceUiRect          control;    ///< the part that takes the mouse
    DeviceUiRect          caption;    ///< where the label draws (may be empty)
    DeviceUiRect          readout;    ///< where the value draws (may be empty)
    bool                  visible = true;   ///< false inside a collapsed section
    int                   depth   = 0;     ///< nesting depth, root children = 0
};

struct DeviceUiLayout {
    double width  = 0.0;
    double height = 0.0;
    std::vector<DeviceUiPlacement> items;   ///< depth-first, containers before children
};

struct DeviceUiLayoutOptions {
    /// 0 uses the spec's preferred width, or the default when that is 0 too.
    double width = 0.0;

    /// Which sections are folded right now. Unset applies each section's
    /// own `collapsed` flag — the state a panel opens in.
    std::function<bool(const DeviceUiWidget&)> isCollapsed;
};

/// The metrics, in points. Public so the renderer's drawing and the tests'
/// expectations read the same numbers.
namespace layout {

inline constexpr double defaultWidth   = 420.0;
inline constexpr double margin         = 14.0;
inline constexpr double gap            = 6.0;    ///< between siblings

inline constexpr double knobSize       = 62.0;
inline constexpr double captionHeight  = 14.0;
inline constexpr double readoutHeight  = 16.0;
inline constexpr double knobCellHeight = captionHeight + 4.0 + knobSize + 4.0 + readoutHeight;

inline constexpr double rowControlHeight = 24.0;  ///< slider, toggle, combo, meter rows
inline constexpr double rowCaptionWidth  = 92.0;
inline constexpr double rowReadoutWidth  = 66.0;
inline constexpr double sliderTrackHeight = 12.0;

inline constexpr double labelHeight    = 18.0;
inline constexpr double sectionHeader  = 24.0;
inline constexpr double sectionInset   = 0.0;
inline constexpr double curveHeight    = 156.0;
inline constexpr double faderWallHeight = 132.0;
inline constexpr double placeholderHeight = 96.0;   ///< a widget the renderer does not draw yet

} // namespace layout

/// Lays out `spec`. Never fails: a widget kind the renderer has no drawing
/// for still gets a placeholder cell, so a spec written ahead of the
/// renderer opens as a panel rather than falling back.
[[nodiscard]] DeviceUiLayout layoutDeviceUi(const DeviceUiSpec& spec,
                                            const DeviceUiLayoutOptions& options = {});

/// True for the widget kinds the renderer draws today; the rest are
/// placeholders (docs/plugin-archive/00-CONTRACTS.md §4).
[[nodiscard]] bool isRenderedWidget(DeviceWidget kind) noexcept;

/// True for the layout containers (section, row, grid, tab).
[[nodiscard]] bool isContainerWidget(DeviceWidget kind) noexcept;

} // namespace incdaw::app
