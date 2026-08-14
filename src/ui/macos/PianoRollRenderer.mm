#include "ui/macos/PianoRollRenderer.h"

#import <Foundation/Foundation.h>

#include <string>

namespace incdaw::ui {
namespace {

/// Compiled at runtime. See docs/DECISIONS.md D-011.
///
/// A single unit quad, expanded per instance into a rectangle. Positions are
/// supplied in points with y increasing downwards — the same convention as
/// AppKit's flipped views and as app::PianoRollModel — and converted to clip
/// space here, so no coordinate flipping is scattered through the UI code.
constexpr const char* shaderSource = R"METAL(
#include <metal_stdlib>
using namespace metal;

struct Instance {
    float4 bounds;   // x, y, width, height in points
    float4 colour;
};

struct VertexOut {
    float4 position [[position]];
    float4 colour;
};

vertex VertexOut vertexMain(uint vertexID                 [[vertex_id]],
                            uint instanceID               [[instance_id]],
                            constant Instance* instances  [[buffer(0)]],
                            constant float2&   viewport   [[buffer(1)]])
{
    // Triangle strip: (0,0) (1,0) (0,1) (1,1)
    const float2 corner = float2(float(vertexID & 1u), float((vertexID >> 1) & 1u));

    const Instance instance = instances[instanceID];
    const float2 point = instance.bounds.xy + corner * instance.bounds.zw;

    VertexOut out;
    out.position = float4((point.x / viewport.x) * 2.0 - 1.0,
                          1.0 - (point.y / viewport.y) * 2.0,
                          0.0, 1.0);
    out.colour = instance.colour;
    return out;
}

fragment float4 fragmentMain(VertexOut in [[stage_in]])
{
    return in.colour;
}
)METAL";

} // namespace

PianoRollRenderer::~PianoRollRenderer() = default;

bool PianoRollRenderer::initialise(CAMetalLayer* layer, std::string& error)
{
    device_ = MTLCreateSystemDefaultDevice();
    if (device_ == nil) {
        error = "no Metal device available";
        return false;
    }

    layer.device = device_;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    layer.framebufferOnly = YES;

    queue_ = [device_ newCommandQueue];
    if (queue_ == nil) {
        error = "could not create a Metal command queue";
        return false;
    }

    NSError* compileError = nil;
    id<MTLLibrary> library = [device_ newLibraryWithSource:@(shaderSource)
                                                   options:nil
                                                     error:&compileError];
    if (library == nil) {
        error = "shader compilation failed: "
              + std::string(compileError != nil ? compileError.localizedDescription.UTF8String
                                                : "unknown error");
        return false;
    }

    MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.vertexFunction   = [library newFunctionWithName:@"vertexMain"];
    descriptor.fragmentFunction = [library newFunctionWithName:@"fragmentMain"];
    descriptor.colorAttachments[0].pixelFormat = layer.pixelFormat;

    // Alpha blending, so selection highlights and the playhead can sit over the
    // grid without hiding it.
    descriptor.colorAttachments[0].blendingEnabled = YES;
    descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
    descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
    descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

    pipeline_ = [device_ newRenderPipelineStateWithDescriptor:descriptor error:&compileError];
    if (pipeline_ == nil) {
        error = "could not create the render pipeline: "
              + std::string(compileError != nil ? compileError.localizedDescription.UTF8String
                                                : "unknown error");
        return false;
    }

    return true;
}

bool PianoRollRenderer::ensureInstanceCapacity(std::size_t count)
{
    if (count <= instanceCapacity_ && instances_ != nil)
        return true;

    // Grown in steps rather than to the exact size, so scrolling through a
    // dense pattern does not reallocate every frame.
    std::size_t capacity = instanceCapacity_ > 0 ? instanceCapacity_ : 1024;
    while (capacity < count)
        capacity *= 2;

    instances_ = [device_ newBufferWithLength:capacity * sizeof(float) * 8
                                      options:MTLResourceStorageModeShared];
    if (instances_ == nil)
        return false;

    instanceCapacity_ = capacity;
    return true;
}

void PianoRollRenderer::draw(CAMetalLayer* layer, const std::vector<Rect>& rectangles,
                             float widthPoints, float heightPoints)
{
    if (pipeline_ == nil || layer == nil || widthPoints <= 0.0f || heightPoints <= 0.0f)
        return;

    id<CAMetalDrawable> drawable = [layer nextDrawable];
    if (drawable == nil)
        return;   // the layer is not on screen, or we are ahead of the display

    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = drawable.texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(0.09, 0.09, 0.10, 1.0);

    id<MTLCommandBuffer> commands = [queue_ commandBuffer];
    id<MTLRenderCommandEncoder> encoder = [commands renderCommandEncoderWithDescriptor:pass];

    if (!rectangles.empty() && ensureInstanceCapacity(rectangles.size())) {
        auto* destination = static_cast<float*>(instances_.contents);

        for (std::size_t index = 0; index < rectangles.size(); ++index) {
            const Rect& rectangle = rectangles[index];
            float* slot = destination + index * 8;

            slot[0] = rectangle.x;
            slot[1] = rectangle.y;
            slot[2] = rectangle.width;
            slot[3] = rectangle.height;
            slot[4] = rectangle.red;
            slot[5] = rectangle.green;
            slot[6] = rectangle.blue;
            slot[7] = rectangle.alpha;
        }

        const float viewport[2] = {widthPoints, heightPoints};

        [encoder setRenderPipelineState:pipeline_];
        [encoder setVertexBuffer:instances_ offset:0 atIndex:0];
        [encoder setVertexBytes:viewport length:sizeof(viewport) atIndex:1];

        // Every rectangle in the view, in one draw call.
        [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip
                    vertexStart:0
                    vertexCount:4
                  instanceCount:rectangles.size()];
    }

    [encoder endEncoding];
    [commands presentDrawable:drawable];
    [commands commit];
}

} // namespace incdaw::ui
