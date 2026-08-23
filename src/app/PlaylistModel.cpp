#include "app/PlaylistModel.h"

#include <algorithm>
#include <cmath>

namespace incdaw::app {

double PlaylistModel::trackHeight(const Track& track) noexcept
{
    return static_cast<double>(track.height > 0 ? track.height : 64);
}

double PlaylistModel::rowHeight(const std::vector<Track>& tracks, std::size_t index) noexcept
{
    if (index >= tracks.size())
        return 0.0;

    return project::trackHidden(tracks, tracks[index]) ? 0.0 : trackHeight(tracks[index]);
}

double PlaylistModel::trackY(const std::vector<Track>& tracks, std::size_t index) const noexcept
{
    double y = -viewport_.firstTrackY;

    for (std::size_t row = 0; row < index && row < tracks.size(); ++row)
        y += rowHeight(tracks, row);

    return y;
}

double PlaylistModel::tracksHeight(const std::vector<Track>& tracks) noexcept
{
    double height = 0.0;
    for (std::size_t row = 0; row < tracks.size(); ++row)
        height += rowHeight(tracks, row);

    return height;
}

std::size_t PlaylistModel::trackAtY(const std::vector<Track>& tracks, double y) const noexcept
{
    double top = -viewport_.firstTrackY;

    for (std::size_t row = 0; row < tracks.size(); ++row) {
        const double height = rowHeight(tracks, row);

        // A hidden row occupies no range, so `y >= top && y < top` is false
        // for it and the search walks straight past.
        if (y >= top && y < top + height)
            return row;

        top += height;
    }

    return noTrack;
}

double PlaylistModel::laneHeight(const Project& project, std::size_t index) const noexcept
{
    const std::vector<Track>& tracks = project.tracks();
    if (index >= tracks.size())
        return 0.0;

    const double height = rowHeight(tracks, index);
    if (height <= 0.0)
        return 0.0;   // hidden by a collapsed folder: no lanes to divide

    const int lanes = project::trackLaneCount(project, tracks[index].id);
    return height / static_cast<double>(std::max(1, lanes));
}

int PlaylistModel::laneAtY(const Project& project, std::size_t index, double y) const noexcept
{
    const double band = laneHeight(project, index);
    if (band <= 0.0)
        return 0;

    const double top = trackY(project.tracks(), index);
    const int    lane = static_cast<int>(std::floor((y - top) / band));
    const int    last = project::trackLaneCount(project, project.tracks()[index].id) - 1;

    return std::clamp(lane, 0, last);
}

PlaylistModel::Rect PlaylistModel::clipRect(const Project& project, const Clip& clip) const noexcept
{
    const std::size_t row = project.indexOfTrack(clip.track);
    if (row == Project::notFound)
        return {};

    // Placement resolved by clip type (D-013): audio clips live in frames and
    // are converted through the tempo map; everything else is already ticks.
    const Tick startTick   = project::clipStartTicks(clip, project.tempoMap());
    const Tick lengthTicks = project::clipLengthTicks(clip, project.tempoMap());

    const Tick length = lengthTicks > 0 ? lengthTicks : 0;

    const double x = tickToX(startTick);
    const double width = static_cast<double>(length) * pointsPerTick();
    const double y = trackY(project.tracks(), row);

    // One point of inset top and bottom, so adjacent tracks' clips do not touch
    // and a row boundary stays visible where the timeline is dense.
    const double band = laneHeight(project, row);
    if (band <= 0.0)
        return {};   // a collapsed folder is hiding this clip's track

    // A lane is a band within the row, so a clip that shares its span with
    // another sits beside it rather than under it.
    const int lanes = project::trackLaneCount(project, project.tracks()[row].id);
    const double laneTop = y + band * static_cast<double>(std::clamp(clip.lane, 0, lanes - 1));

    return {x, laneTop + 1.0, width, band - 2.0};
}

void PlaylistModel::collectVisibleClips(const Project& project, std::vector<VisibleClip>& out) const
{
    out.clear();

    const std::vector<Clip>& clips = project.clips();

    for (std::size_t index = 0; index < clips.size(); ++index) {
        const Clip& clip = clips[index];

        const Rect rect = clipRect(project, clip);
        if (rect.width <= 0.0)
            continue;

        // Culled against the viewport in both axes. A song with a thousand
        // clips must cost only the ones on screen.
        // Half-open in both axes: a clip whose right edge lands exactly on the
        // left of the viewport covers no pixel and is not visible.
        if (rect.x + rect.width <= 0.0 || rect.x >= viewport_.width)
            continue;

        if (rect.y + rect.height <= 0.0 || rect.y >= viewport_.height)
            continue;

        VisibleClip visible;
        visible.id       = clip.id;
        visible.index    = index;
        visible.rect     = rect;
        visible.selected = isSelected(clip.id);
        visible.muted    = clip.muted;
        visible.colour   = clip.colour;
        out.push_back(visible);
    }
}

std::size_t PlaylistModel::clipAtPoint(const Project& project, double x, double y) const
{
    const std::vector<Clip>& clips = project.clips();

    // The point names a lane, and only that lane's clips are candidates: with
    // lanes, "the one drawn last wins" is exactly the behaviour the lane is
    // there to replace.
    const std::size_t row = trackAtY(project.tracks(), y);
    const int         lane = row == noTrack ? 0 : laneAtY(project, row, y);

    for (std::size_t index = clips.size(); index > 0; --index) {
        const std::size_t position = index - 1;

        if (row != noTrack && clips[position].lane != lane)
            continue;

        if (clipRect(project, clips[position]).contains(x, y))
            return position;
    }

    return noClip;
}

bool PlaylistModel::isOverResizeHandle(const Project& project, std::size_t index,
                                       double x, double y) const
{
    if (index >= project.clips().size())
        return false;

    const Rect rect = clipRect(project, project.clips()[index]);
    if (!rect.contains(x, y))
        return false;

    // A narrow clip is all handle otherwise, and could never be dragged.
    const double handle = std::min(resizeHandleWidth, rect.width / 3.0);
    return x >= rect.x + rect.width - handle;
}

void PlaylistModel::clipsInRectangle(const Project& project, double x, double y,
                                     double width, double height,
                                     std::vector<EntityId>& out) const
{
    out.clear();

    const double left   = std::min(x, x + width);
    const double right  = std::max(x, x + width);
    const double top    = std::min(y, y + height);
    const double bottom = std::max(y, y + height);

    for (const Clip& clip : project.clips()) {
        const Rect rect = clipRect(project, clip);
        if (rect.width <= 0.0)
            continue;

        const bool intersects = rect.x < right && rect.x + rect.width > left
                             && rect.y < bottom && rect.y + rect.height > top;

        if (intersects)
            out.push_back(clip.id);
    }
}

Tick PlaylistModel::snapTick(Tick tick) const noexcept
{
    if (snap_ <= 0)
        return tick;

    const Tick remainder = tick % snap_;
    const Tick base      = tick - remainder;

    return remainder * 2 >= snap_ ? base + snap_ : base;
}

void PlaylistModel::setSelection(std::vector<EntityId> clips)
{
    selection_ = std::move(clips);

    std::sort(selection_.begin(), selection_.end());
    selection_.erase(std::unique(selection_.begin(), selection_.end()), selection_.end());
}

void PlaylistModel::addToSelection(EntityId clip)
{
    if (!isSelected(clip))
        selection_.push_back(clip);
}

void PlaylistModel::toggleSelection(EntityId clip)
{
    const auto found = std::find(selection_.begin(), selection_.end(), clip);

    if (found != selection_.end())
        selection_.erase(found);
    else
        selection_.push_back(clip);
}

bool PlaylistModel::isSelected(EntityId clip) const noexcept
{
    return std::find(selection_.begin(), selection_.end(), clip) != selection_.end();
}

void PlaylistModel::pruneSelection(const Project& project)
{
    selection_.erase(std::remove_if(selection_.begin(), selection_.end(),
                                    [&project](EntityId id) {
                                        return project.findClip(id) == nullptr;
                                    }),
                     selection_.end());
}

} // namespace incdaw::app
