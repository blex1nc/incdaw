#pragma once

#include "project/Model.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace incdaw::app {

using project::Clip;
using project::EntityId;
using project::Project;
using project::Tick;
using project::Track;

/// The playlist's geometry and hit testing, with no drawing in it.
///
/// The third pane built this way, after app::PianoRollModel and
/// app::ChannelRackModel, and for the same reason: what is on screen and what
/// the mouse is over is arithmetic, and arithmetic is testable without a
/// window.
///
/// Selection is by `EntityId`, not by index — unlike the Piano Roll, where an
/// index into one channel's event list is safe because the undo stack is LIFO.
/// The playlist holds every track's clips in one vector, so an edit on one
/// track moves the indices of another's.
class PlaylistModel {
public:
    struct Viewport {
        Tick   firstTick    = 0;
        Tick   visibleTicks = incdaw::engine::ticksPerQuarterNote * 64;   ///< sixteen 4/4 bars

        /// Vertical scroll, in points from the top of the first track.
        double firstTrackY  = 0.0;

        double width  = 1000.0;   ///< grid area, excluding the track headers
        double height = 400.0;
    };

    struct Rect {
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;

        [[nodiscard]] bool contains(double pointX, double pointY) const noexcept
        {
            return pointX >= x && pointX < x + width && pointY >= y && pointY < y + height;
        }
    };

    /// A clip intersecting the viewport, with its screen rectangle.
    struct VisibleClip {
        EntityId      id;
        std::size_t   index = 0;      ///< index into Project::clips()
        Rect          rect;
        bool          selected = false;
        bool          muted    = false;
        std::uint32_t colour   = 0xFF6699CCu;
    };

    void setViewport(const Viewport& viewport) noexcept { viewport_ = viewport; }
    [[nodiscard]] const Viewport& viewport() const noexcept { return viewport_; }

    [[nodiscard]] double pointsPerTick() const noexcept
    {
        return viewport_.visibleTicks > 0
                   ? viewport_.width / static_cast<double>(viewport_.visibleTicks)
                   : 0.0;
    }

    // ── Coordinate conversion ───────────────────────────────────────────────

    [[nodiscard]] double tickToX(Tick tick) const noexcept
    {
        return static_cast<double>(tick - viewport_.firstTick) * pointsPerTick();
    }

    [[nodiscard]] Tick xToTick(double x) const noexcept
    {
        const double scale = pointsPerTick();
        return scale > 0.0 ? viewport_.firstTick + static_cast<Tick>(x / scale)
                           : viewport_.firstTick;
    }

    /// Row height, from the track itself: heights are per track and persist
    /// with the project, so the playlist has to ask rather than assume.
    [[nodiscard]] static double trackHeight(const Track& track) noexcept;

    /// Row height in the layout, which is zero for a track a collapsed folder
    /// is hiding.
    ///
    /// Every piece of geometry below goes through this rather than through
    /// `trackHeight`, so a hidden row takes no space, catches no click and
    /// draws no clip — one rule, applied once, instead of a `hidden` test
    /// scattered through drawing and hit testing separately.
    [[nodiscard]] static double rowHeight(const std::vector<Track>& tracks,
                                          std::size_t index) noexcept;

    [[nodiscard]] double trackY(const std::vector<Track>& tracks, std::size_t index) const noexcept;
    [[nodiscard]] static double tracksHeight(const std::vector<Track>& tracks) noexcept;

    static constexpr std::size_t noTrack = static_cast<std::size_t>(-1);
    static constexpr std::size_t noClip  = static_cast<std::size_t>(-1);

    [[nodiscard]] std::size_t trackAtY(const std::vector<Track>& tracks, double y) const noexcept;

    /// Screen rectangle for a clip, whether or not it is on screen.
    [[nodiscard]] Rect clipRect(const Project& project, const Clip& clip) const noexcept;

    // ── Culling ─────────────────────────────────────────────────────────────

    /// Fills `out` with the clips intersecting the viewport. `out` is a
    /// caller-owned buffer reused across frames, so a steady state allocates
    /// nothing.
    void collectVisibleClips(const Project& project, std::vector<VisibleClip>& out) const;

    // ── Hit testing ─────────────────────────────────────────────────────────

    /// Topmost clip under a point, as an index into Project::clips(), or
    /// `noClip`. Searched back to front, so the clip drawn last — the one on
    /// top where they overlap — is the one picked up.
    [[nodiscard]] std::size_t clipAtPoint(const Project& project, double x, double y) const;

    [[nodiscard]] bool isOverResizeHandle(const Project& project, std::size_t index,
                                          double x, double y) const;

    void clipsInRectangle(const Project& project, double x, double y, double width, double height,
                          std::vector<EntityId>& out) const;

    // ── Grid ────────────────────────────────────────────────────────────────

    void setSnap(Tick snap) noexcept { snap_ = snap > 0 ? snap : 0; }
    [[nodiscard]] Tick snap() const noexcept { return snap_; }

    [[nodiscard]] Tick snapTick(Tick tick) const noexcept;

    // ── Selection ───────────────────────────────────────────────────────────

    void setSelection(std::vector<EntityId> clips);
    void addToSelection(EntityId clip);
    void toggleSelection(EntityId clip);
    void clearSelection() noexcept { selection_.clear(); }

    [[nodiscard]] bool isSelected(EntityId clip) const noexcept;
    [[nodiscard]] const std::vector<EntityId>& selection() const noexcept { return selection_; }

    /// Drops ids that no longer exist, after clips have been deleted.
    void pruneSelection(const Project& project);

    /// Grab zone at a clip's right edge, in points.
    static constexpr double resizeHandleWidth = 8.0;

private:
    Viewport              viewport_;
    Tick                  snap_ = incdaw::engine::ticksPerQuarterNote;
    std::vector<EntityId> selection_;
};

} // namespace incdaw::app
