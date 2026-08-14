#pragma once

#include "project/Model.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace incdaw::app {

using project::EntityId;
using project::FrameCount;
using project::FramePosition;
using project::Project;

/// The Playlist's geometry and hit testing, with no drawing in it.
///
/// The same split as PianoRollModel and for the same reason: what is on screen
/// and what the mouse is over is arithmetic, and arithmetic is testable without
/// a window. The renderer only draws the list this produces.
///
/// Clips are addressed by `EntityId`, not by index. Notes inside a pattern can
/// use indices because the undo stack is LIFO and note commands never reorder;
/// clips are created, deleted and split from several places, and an index would
/// mean something different after every one of those.
class PlaylistModel {
public:
    struct VisibleClip {
        EntityId      id;
        std::size_t   trackRow = 0;
        double        x = 0.0;
        double        y = 0.0;
        double        width = 0.0;
        double        height = 0.0;
        std::uint32_t colour = 0xFF6699CCu;
        bool          selected = false;
        bool          muted    = false;
        bool          locked   = false;
    };

    struct Viewport {
        FramePosition firstFrame    = 0;
        FrameCount    visibleFrames = 48000 * 8;   ///< eight seconds at 48 kHz

        std::size_t   firstTrack    = 0;
        std::size_t   visibleTracks = 8;

        double        width  = 1000.0;   ///< in points
        double        height = 400.0;
    };

    void setViewport(const Viewport& viewport) noexcept { viewport_ = viewport; }
    [[nodiscard]] const Viewport& viewport() const noexcept { return viewport_; }

    /// Points per frame. The playlist zooms in time, not in ticks, because it
    /// also has to place audio — which has frames and no opinion about tempo.
    [[nodiscard]] double pointsPerFrame() const noexcept
    {
        return viewport_.visibleFrames > 0
                   ? viewport_.width / static_cast<double>(viewport_.visibleFrames)
                   : 0.0;
    }

    [[nodiscard]] double trackHeight() const noexcept
    {
        return viewport_.visibleTracks > 0
                   ? viewport_.height / static_cast<double>(viewport_.visibleTracks)
                   : 0.0;
    }

    [[nodiscard]] double frameToX(FramePosition frame) const noexcept
    {
        return static_cast<double>(frame - viewport_.firstFrame) * pointsPerFrame();
    }

    [[nodiscard]] FramePosition xToFrame(double x) const noexcept
    {
        const double scale = pointsPerFrame();
        return scale > 0.0 ? viewport_.firstFrame + static_cast<FramePosition>(x / scale)
                           : viewport_.firstFrame;
    }

    [[nodiscard]] double trackToY(std::size_t row) const noexcept
    {
        return (static_cast<double>(row) - static_cast<double>(viewport_.firstTrack)) * trackHeight();
    }

    /// Row under a y coordinate. May be past the last track, which is how a
    /// click on empty space below the arrangement is recognised.
    [[nodiscard]] std::size_t yToTrack(double y) const noexcept
    {
        const double height = trackHeight();
        if (height <= 0.0 || y < 0.0)
            return viewport_.firstTrack;

        return viewport_.firstTrack + static_cast<std::size_t>(y / height);
    }

    // ── Culling ─────────────────────────────────────────────────────────────

    /// Fills `out` with the clips intersecting the viewport.
    ///
    /// `out` is caller-owned and reused across frames, so a steady state costs
    /// no allocation.
    void collectVisibleClips(const Project& project, std::vector<VisibleClip>& out) const;

    // ── Hit testing ─────────────────────────────────────────────────────────

    /// Topmost clip under a point, or an invalid id.
    ///
    /// Searched back to front, so the clip drawn last — the one on top where
    /// they overlap — is the one picked up. Overlapping clips are legal: the
    /// playlist places freely and does not push clips out of each other's way.
    [[nodiscard]] EntityId clipAtPoint(const Project& project, double x, double y) const;

    [[nodiscard]] bool isOverResizeHandle(const Project& project, EntityId clip,
                                          double x, double y) const;

    void clipsInRectangle(const Project& project, double x, double y, double width, double height,
                          std::vector<EntityId>& out) const;

    // ── Grid ────────────────────────────────────────────────────────────────

    /// Snap resolution in frames. Zero means free placement.
    void setSnap(FrameCount frames) noexcept { snap_ = frames > 0 ? frames : 0; }
    [[nodiscard]] FrameCount snap() const noexcept { return snap_; }

    [[nodiscard]] FramePosition snapFrame(FramePosition frame) const noexcept;

    // ── Selection ───────────────────────────────────────────────────────────

    void setSelection(std::vector<EntityId> clips);
    void addToSelection(EntityId clip);
    void toggleSelection(EntityId clip);
    void clearSelection() noexcept { selection_.clear(); }

    [[nodiscard]] bool isSelected(EntityId clip) const noexcept;
    [[nodiscard]] const std::vector<EntityId>& selection() const noexcept { return selection_; }

    /// Drops ids that no longer exist, after clips have been deleted.
    void pruneSelection(const Project& project);

    static constexpr double resizeHandleWidth = 8.0;

private:
    [[nodiscard]] std::size_t rowOfTrack(const Project& project, EntityId track) const noexcept;

    Viewport              viewport_;
    FrameCount            snap_ = 0;
    std::vector<EntityId> selection_;
};

} // namespace incdaw::app
