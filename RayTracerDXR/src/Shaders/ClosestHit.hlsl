#include "Common.hlsli"
#include "ShadingHelper.hlsli"

static const float4x4 matr =
{
    0.522613466f, 0, 0, 0,
    0, 0.391960055f, 0, 0,
    0, 0, 19.9800129f, -4.995f,
    0, 0, -19.0200138f, 5.00500345f
};

[shader("closesthit")]
void chs(inout Payload payload, in IntersectionAttributes attribs)
{
    if (payload.recursions >= maxTraceRecursionDepth)
        return;
    
    RayDesc scatterRay;
    rayDesc_Initialize(scatterRay);
    if (!scatter(attribs, payload, scatterRay))
        return;
    
    payload.recursions++;
    TraceRay(gRtScene, 0, 0xFF, 0, 0, 0, scatterRay, payload);
}