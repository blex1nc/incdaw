#include "app/devices/DeviceUiLayout.h"

#include <algorithm>

namespace incdaw::app {

namespace {

using namespace layout;

struct Pass {
    const DeviceUiLayoutOptions& options;
    DeviceUiLayout&              out;

    [[nodiscard]] bool collapsed(const DeviceUiWidget& widget) const
    {
        if (widget.kind != DeviceWidget::section)
            return false;

        return options.isCollapsed ? options.isCollapsed(widget) : widget.collapsed;
    }

    /// The height a leaf wants at `width`. Containers are measured by
    /// placing them, so this is leaves only.
    [[nodiscard]] static double leafHeight(const DeviceUiWidget& widget)
    {
        switch (widget.kind) {
        case DeviceWidget::knob:          return knobCellHeight;
        case DeviceWidget::slider:
        case DeviceWidget::toggle:
        case DeviceWidget::combo:
        case DeviceWidget::meter:         return rowControlHeight;
        case DeviceWidget::label:         return labelHeight;
        case DeviceWidget::drawableCurve: return curveHeight;
        case DeviceWidget::faderWall:     return faderWallHeight;
        default:                          return placeholderHeight;
        }
    }

    /// Places `widget` with its top-left at (x, y) in `width`; returns the
    /// height used. Appends the placement (and, for containers, the
    /// children's) to `out.items`.
    double place(const DeviceUiWidget& widget, double x, double y, double width, int depth,
                 bool visible)
    {
        if (isContainerWidget(widget.kind))
            return placeContainer(widget, x, y, width, depth, visible);

        DeviceUiPlacement placement;
        placement.widget  = &widget;
        placement.visible = visible;
        placement.depth   = depth;

        const double height = leafHeight(widget);
        placement.frame     = {x, y, width, height};

        switch (widget.kind) {
        case DeviceWidget::knob: {
            const double knobX = x + (width - knobSize) / 2.0;
            placement.caption = {x, y, width, captionHeight};
            placement.control = {knobX, y + captionHeight + 4.0, knobSize, knobSize};
            placement.readout = {x, y + captionHeight + 4.0 + knobSize + 4.0, width,
                                 readoutHeight};
            break;
        }
        case DeviceWidget::slider:
        case DeviceWidget::combo:
        case DeviceWidget::meter: {
            const double trackX = x + rowCaptionWidth;
            const double trackW = std::max(0.0, width - rowCaptionWidth - rowReadoutWidth - 8.0);
            placement.caption = {x, y, rowCaptionWidth - 4.0, height};
            placement.readout = {x + width - rowReadoutWidth, y, rowReadoutWidth, height};

            if (widget.kind == DeviceWidget::slider)
                placement.control = {trackX, y + (height - sliderTrackHeight) / 2.0, trackW,
                                     sliderTrackHeight};
            else
                placement.control = {trackX, y + 2.0, trackW, height - 4.0};
            break;
        }
        case DeviceWidget::toggle: {
            // The toggle's lamp, then its caption; no readout — the lamp is it.
            placement.control = {x, y + 3.0, height - 6.0, height - 6.0};
            placement.caption = {x + height, y, std::max(0.0, width - height), height};
            break;
        }
        case DeviceWidget::label:
            placement.caption = placement.frame;
            break;
        default:
            // Curves, fader walls and every not-yet-rendered widget: the
            // whole cell is the control, the caption sits inside its top.
            placement.control = placement.frame;
            placement.caption = {x + 6.0, y + 4.0, std::max(0.0, width - 12.0), captionHeight};
            break;
        }

        out.items.push_back(placement);
        return height;
    }

    double placeContainer(const DeviceUiWidget& widget, double x, double y, double width,
                          int depth, bool visible)
    {
        const std::size_t index = out.items.size();
        out.items.push_back({});   // filled in once the children are measured

        double cursor = y;

        const bool titled = widget.kind == DeviceWidget::section
                         || widget.kind == DeviceWidget::tab;
        const bool folded = collapsed(widget);

        DeviceUiRect header;
        if (titled) {
            header = {x, cursor, width, sectionHeader};
            cursor += sectionHeader;
        }

        const bool childrenVisible = visible && !folded;
        const double innerX        = x + sectionInset;
        const double innerWidth    = width - sectionInset * 2.0;

        if (widget.kind == DeviceWidget::row || widget.kind == DeviceWidget::grid) {
            const std::size_t count = widget.children.size();
            const std::size_t columns =
                widget.kind == DeviceWidget::row
                    ? std::max<std::size_t>(count, 1)
                    : std::max<std::size_t>(widget.columns, 1);

            const double cellWidth = (innerWidth - gap * static_cast<double>(columns - 1))
                                   / static_cast<double>(columns);

            double rowTop    = cursor;
            double rowHeight = 0.0;

            for (std::size_t child = 0; child < count; ++child) {
                const std::size_t column = child % columns;
                if (column == 0 && child != 0) {
                    rowTop += rowHeight + gap;
                    rowHeight = 0.0;
                }

                const double cellX = innerX + (cellWidth + gap) * static_cast<double>(column);
                const double used  = place(widget.children[child], cellX, rowTop, cellWidth,
                                           depth + 1, childrenVisible);
                rowHeight = std::max(rowHeight, used);
            }

            if (count > 0)
                cursor = rowTop + rowHeight;
        } else {
            // section, tab: a vertical stack
            bool first = true;
            for (const DeviceUiWidget& child : widget.children) {
                if (!first)
                    cursor += gap;
                first = false;
                cursor += place(child, innerX, cursor, innerWidth, depth + 1, childrenVisible);
            }
        }

        // A folded section contributes only its header.
        const double height = folded ? sectionHeader : cursor - y;

        DeviceUiPlacement& placement = out.items[index];
        placement.widget  = &widget;
        placement.visible = visible;
        placement.depth   = depth;
        placement.frame   = {x, y, width, height};
        placement.control = titled ? header : DeviceUiRect{};
        placement.caption = header;

        if (folded) {
            // The children were placed (so the tree stays whole) but hidden;
            // pull the cursor back so nothing below them moves.
            for (std::size_t item = index + 1; item < out.items.size(); ++item)
                out.items[item].visible = false;
        }

        return height;
    }
};

} // namespace

bool isContainerWidget(DeviceWidget kind) noexcept
{
    return kind == DeviceWidget::section || kind == DeviceWidget::row
        || kind == DeviceWidget::grid || kind == DeviceWidget::tab;
}

bool isRenderedWidget(DeviceWidget kind) noexcept
{
    switch (kind) {
    case DeviceWidget::knob:
    case DeviceWidget::slider:
    case DeviceWidget::faderWall:
    case DeviceWidget::toggle:
    case DeviceWidget::combo:
    case DeviceWidget::meter:
    case DeviceWidget::drawableCurve:
    case DeviceWidget::label:
    case DeviceWidget::section:
    case DeviceWidget::row:
    case DeviceWidget::grid:
        return true;
    default:
        return false;
    }
}

DeviceUiLayout layoutDeviceUi(const DeviceUiSpec& spec, const DeviceUiLayoutOptions& options)
{
    DeviceUiLayout out;
    out.width = options.width > 0.0 ? options.width
              : spec.preferredWidth > 0.0 ? spec.preferredWidth
                                          : layout::defaultWidth;

    Pass pass{options, out};

    const double innerWidth = out.width - layout::margin * 2.0;
    double cursor = layout::margin;

    bool first = true;
    for (const DeviceUiWidget& widget : spec.root) {
        if (!first)
            cursor += layout::gap;
        first = false;
        cursor += pass.place(widget, layout::margin, cursor, innerWidth, 0, true);
    }

    out.height = cursor + layout::margin;
    return out;
}

} // namespace incdaw::app
