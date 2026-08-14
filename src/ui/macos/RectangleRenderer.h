#pragma once

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstddef>
#include <vector>

namespace incdaw::ui {

/// One axis-aligned rectangle, in points, with a colour.
///
/// A Piano Roll and a Playlist are entirely rectangles — key rows, grid lines,
/// notes, tracks, clips, playhead — so the renderer draws exactly one primitive
/// type. Everything is
/// submitted as instances of a single unit quad in one draw call, which is why
/// ten thousand notes cost the same as ten.
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float red = 1.0f;
    float green = 1.0f;
    float blue = 1.0f;
    float alpha = 1.0f;
};

/// GPU-accelerated rectangle renderer.
///
/// Shared by the Piano Roll and the Playlist. Both editors are entirely
/// rectangles, both need to stay smooth with thousands of them, and a second
/// copy of Metal device, pipeline and buffer management would be a second place
/// for the same bug to live.
///
/// The shader is compiled from source at runtime rather than built offline,
/// because this machine has Command Line Tools without a full Xcode and
/// therefore no `metal` compiler (docs/DECISIONS.md D-011). Compilation happens
/// once, at startup, off any hot path.
class RectangleRenderer {
public:
    RectangleRenderer() = default;
    ~RectangleRenderer();

    RectangleRenderer(const RectangleRenderer&)            = delete;
    RectangleRenderer& operator=(const RectangleRenderer&) = delete;

    /// Returns false and fills `error` if Metal is unavailable or the shader
    /// fails to compile. The caller must then fall back or report — never
    /// silently present an empty view.
    bool initialise(CAMetalLayer* layer, std::string& error);

    [[nodiscard]] bool isReady() const noexcept { return pipeline_ != nil; }

    /// Draws `rectangles` into the layer's next drawable.
    void draw(CAMetalLayer* layer, const std::vector<Rect>& rectangles, float widthPoints, float heightPoints);

private:
    bool ensureInstanceCapacity(std::size_t count);

    id<MTLDevice>              device_   = nil;
    id<MTLCommandQueue>        queue_    = nil;
    id<MTLRenderPipelineState> pipeline_ = nil;
    id<MTLBuffer>              instances_ = nil;
    std::size_t                instanceCapacity_ = 0;
};

} // namespace incdaw::ui
