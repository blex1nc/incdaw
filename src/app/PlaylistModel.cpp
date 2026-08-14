#include "app/PlaylistModel.h"

#include <algorithm>

namespace incdaw::app {

double PlaylistModel::trackHeight(const Track& track) noexcept
{
    return static_cast<double>(track.height > 0 ? track.height : 64);
}

double PlaylistModel::trackY(const std::vector<Track>& tracks, std::size_t index) const noexcept
{
    double y = -viewport_.firstTrackY;

    for (std::size_t row = 0; row < index && row < tracks.size(); ++row)
        y += trackHeight(tracks[row]);

    return y;
}

double PlaylistModel::tracksHeight(const std::vector<Track>& tracks) noexcept
{
    double height = 0.0;
    for (const Track& track : tracks)
        height += trackHeight(track);

    return height;
}

std::size_t PlaylistModel::trackAtY(const std::vector<Track>& tracks, double y) const noexcept
{
    double top = -viewport_.firstTrackY;

    for (std::size_t row = 0; row < tracks.size(); ++row) {
        const double height = trackHeight(tracks[row]);
        if (y >= top && y < top + height)
            return row;

        top += height;
    }

    return noTrack;
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
    return {x, y + 1.0, width, trackHeight(project.tracks()[row]) - 2.0};
}

void PlaylistModel::collectVisibleClips(const Project& project, std::vector<VisibleClip>& out) const
{
    out.clear();

    const std::vector<Clip>& clips = project.clips();

    for (std::size_t index = 0; index < clips.size(); ++index) {
        const Clip& clip = clips[index];
        if (clip.type == project::ClipType::automation)
            continue;   // automation clips arrive with Phase 11b

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

    for (std::size_t index = clips.size(); index > 0; --index) {
        const std::size_t position = index - 1;
        const Clip& clip = clips[position];

        if (clip.type == project::ClipType::automation)
            continue;

        if (clipRect(project, clip).contains(x, y))
            return position;
    }

    return noClip;
}

bool PlaylistModel::isOverResizeHandle(const Project& project, std::size_t index,
                                       double x, double y) const
{
    if (index >= project.clips().size())
        return false;

    // Audio clips are frame-anchored and not yet resizable from the grid;
    // offering the handle would start a drag that cannot apply (9b work).
    if (project.clips()[index].type == project::ClipType::audio)
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
        if (clip.type == project::ClipType::automation)
            continue;

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
