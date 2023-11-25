#include "Common.hlsli"

float3 GenerateRayDirection(float2 launchIdx, float2 launchDim, float aspectRatio)
{
    float2 ndc = (launchIdx / launchDim) * 2.0f - 1.0f;
    ndc.y *= -1.0f;

    float4 screenSpaceFar = float4(ndc, 1.0f, 1.0f);
    float4 screenSpaceNear = float4(ndc, 0.0f, 1.0f);
    
    float4 far = mul(screenSpaceFar, RayTraceCB.ViewProjectionInv);
    far /= far.w;
    float4 near = mul(screenSpaceNear, RayTraceCB.ViewProjectionInv);
    near /= near.w;
 
    return normalize(far.xyz - near.xyz);
}

float3 TraceRayPerPixel(float2 launchIndex, float2 launchDim)
{
    float aspectRatio = float(launchDim.x) / float(launchDim.y);

    float3 color = float3(0.0f, 0.0f, 0.0f);
    float2 ndc = launchIndex.xy + float2(0.5f, 0.5f);
    float2 ndcInLoop = ndc;

    for (uint i = 0; i < RaysPerPixel; i++)
    {
        ndcInLoop = ndc + (rand(frac(ndcInLoop)) * 2.0f - 1.0f);

        RayDesc ray;
        ray.Origin = RayTraceCB.CameraPosition;
        ray.Direction = GenerateRayDirection(ndcInLoop, launchDim, aspectRatio);
        ray.TMin = 0;
        ray.TMax = TMAX;
        
        Payload payload;
        payload.color = float3(1, 1, 1);
        payload.recursions = 1;
        payload.AAIndex = i;
        TraceRay(gRtScene, 0, 0xFF, 0, 0, 0, ray, payload);
        color += payload.color;
    }

    return color / float(RaysPerPixel);
}

[shader("raygeneration")]
void rayGen()
{
    uint3 launchIndex = DispatchRaysIndex();
    uint3 launchDim = DispatchRaysDimensions();

    float3 col = linearToSrgb(TraceRayPerPixel(launchIndex.xy, launchDim.xy));
    gOutput[launchIndex.xy] = float4(col, 1.0f);

}