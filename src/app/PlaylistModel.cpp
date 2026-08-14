#include "app/PlaylistModel.h"

#include <algorithm>

namespace incdaw::app {
namespace {

/// A clip with no length of its own still occupies the timeline: an empty
/// rectangle is not something the user can grab, and a zero-width clip reads as
/// a clip that failed to appear.
[[nodiscard]] FrameCount lengthOf(const project::Clip& clip) noexcept
{
    return clip.length > 0 ? clip.length : 1;
}

} // namespace

std::size_t PlaylistModel::rowOfTrack(const Project& project, EntityId track) const noexcept
{
    const auto& tracks = project.tracks();

    for (std::size_t row = 0; row < tracks.size(); ++row)
        if (tracks[row].id == track)
            return row;

    return tracks.size();
}

void PlaylistModel::collectVisibleClips(const Project& project, std::vector<VisibleClip>& out) const
{
    out.clear();

    const double scale  = pointsPerFrame();
    const double height = trackHeight();

    if (scale <= 0.0 || height <= 0.0)
        return;

    const FramePosition lastFrame = viewport_.firstFrame
                                  + static_cast<FramePosition>(viewport_.visibleFrames);
    const std::size_t   lastTrack = viewport_.firstTrack + viewport_.visibleTracks;

    for (const project::Clip& clip : project.clips()) {
        const std::size_t row = rowOfTrack(project, clip.track);

        if (row >= project.tracks().size() || row < viewport_.firstTrack || row >= lastTrack)
            continue;

        const FramePosition end = clip.start + static_cast<FramePosition>(lengthOf(clip));

        // A clip starting before the viewport is still visible if it reaches
        // into it; dropping it would make long clips vanish as you scroll into
        // them.
        if (end <= viewport_.firstFrame || clip.start >= lastFrame)
            continue;

        VisibleClip visible;
        visible.id       = clip.id;
        visible.trackRow = row;
        visible.x        = frameToX(clip.start);
        visible.y        = trackToY(row);
        visible.width    = static_cast<double>(lengthOf(clip)) * scale;
        visible.height   = height;
        visible.colour   = clip.colour;
        visible.selected = isSelected(clip.id);
        visible.muted    = clip.muted;
        visible.locked   = clip.locked;

        out.push_back(visible);
    }
}

EntityId PlaylistModel::clipAtPoint(const Project& project, double x, double y) const
{
    const double scale  = pointsPerFrame();
    const double height = trackHeight();

    if (scale <= 0.0 || height <= 0.0)
        return {};

    const std::size_t   row   = yToTrack(y);
    const FramePosition frame = xToFrame(x);

    const auto& clips = project.clips();

    for (std::size_t position = clips.size(); position > 0; --position) {
        const project::Clip& clip = clips[position - 1];

        if (rowOfTrack(project, clip.track) != row)
            continue;

        if (frame >= clip.start && frame < clip.start + static_cast<FramePosition>(lengthOf(clip)))
            return clip.id;
    }

    return {};
}

bool PlaylistModel::isOverResizeHandle(const Project& project, EntityId clip,
                                       double x, double y) const
{
    const project::Clip* entry = project.findClip(clip);
    if (entry == nullptr)
        return false;

    const std::size_t row = rowOfTrack(project, entry->track);
    if (row >= project.tracks().size() || yToTrack(y) != row)
        return false;

    const double right = frameToX(entry->start + static_cast<FramePosition>(lengthOf(*entry)));
    return x >= right - resizeHandleWidth && x <= right;
}

void PlaylistModel::clipsInRectangle(const Project& project, double x, double y,
                                     double width, double height,
                                     std::vector<EntityId>& out) const
{
    out.clear();

    // A rectangle dragged up or to the left arrives with negative extents.
    if (width < 0.0) {
        x += width;
        width = -width;
    }

    if (height < 0.0) {
        y += height;
        height = -height;
    }

    const double scale    = pointsPerFrame();
    const double rowSize  = trackHeight();

    if (scale <= 0.0 || rowSize <= 0.0)
        return;

    for (const project::Clip& clip : project.clips()) {
        const std::size_t row = rowOfTrack(project, clip.track);
        if (row >= project.tracks().size())
            continue;

        const double clipLeft   = frameToX(clip.start);
        const double clipRight  = frameToX(clip.start + static_cast<FramePosition>(lengthOf(clip)));
        const double clipTop    = trackToY(row);
        const double clipBottom = clipTop + rowSize;

        const bool overlapsX = clipRight > x && clipLeft < x + width;
        const bool overlapsY = clipBottom > y && clipTop < y + height;

        if (overlapsX && overlapsY)
            out.push_back(clip.id);
    }
}

FramePosition PlaylistModel::snapFrame(FramePosition frame) const noexcept
{
    if (snap_ <= 0)
        return frame;

    const auto grid = static_cast<FramePosition>(snap_);
    const FramePosition remainder = frame % grid;

    // Rounds to the nearest line, including for negative positions, where a
    // truncating division would round towards zero and drift.
    if (remainder == 0)
        return frame;

    if (frame < 0)
        return remainder <= -grid / 2 ? frame - (grid + remainder) : frame - remainder;

    return remainder >= grid / 2 ? frame + (grid - remainder) : frame - remainder;
}

void PlaylistModel::setSelection(std::vector<EntityId> clips)
{
    std::sort(clips.begin(), clips.end());
    clips.erase(std::unique(clips.begin(), clips.end()), clips.end());
    selection_ = std::move(clips);
}

void PlaylistModel::addToSelection(EntityId clip)
{
    if (!isSelected(clip))
        selection_.push_back(clip);
}

void PlaylistModel::toggleSelection(EntityId clip)
{
    const auto position = std::find(selection_.begin(), selection_.end(), clip);

    if (position == selection_.end())
        selection_.push_back(clip);
    else
        selection_.erase(position);
}

bool PlaylistModel::isSelected(EntityId clip) const noexcept
{
    return std::find(selection_.begin(), selection_.end(), clip) != selection_.end();
}

void PlaylistModel::pruneSelection(const Project& project)
{
    selection_.erase(std::remove_if(selection_.begin(), selection_.end(),
                                    [&project](EntityId id) { return project.findClip(id) == nullptr; }),
                     selection_.end());
}

} // namespace incdaw::app
