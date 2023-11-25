#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#define HLSL
#include "HLSLCompat.h"

#define TMAX 100
#define _intersection_bias 0.0001f;

Texture2D<float3> globalRandomNumbers[] : register(t0, space0);
StructuredBuffer<SphereInfo> globalSpheres[] : register(t0, space100);
StructuredBuffer<Material> globalMaterials[] : register(t0, space101);
RaytracingAccelerationStructure gRtScene : register(t0, space200);

RWTexture2D<float4> gOutput : register(u0);

ConstantBuffer<RayTracingConstants> RayTraceCB : register(b0);

static const uint RaysPerPixel = 32;

static StructuredBuffer<SphereInfo> gSpheres = globalSpheres[RayTraceCB.SpheresOfsset];
static Texture2D<float3> gRandomNumbers = globalRandomNumbers[RayTraceCB.TexturesOffset + RayTraceCB.RandomNumbersIndex];
static StructuredBuffer<Material> gMaterials = globalMaterials[RayTraceCB.MaterialsOffset];

struct Payload
{
    float3 color;
    uint recursions;
    uint AAIndex;
};

struct IntersectionAttributes
{
    uint instanceID;
    float hitT;
};

void intersectionAttributes_Initialize(inout IntersectionAttributes attribs)
{
    attribs.hitT = 0xFFFFFFFF;
    attribs.instanceID = 0xFFFFFFFF;
}

void rayDesc_Initialize(inout RayDesc rayDesc)
{
    rayDesc.Origin = float3(0, 0, 0);
    rayDesc.Direction = float3(0, 0, 0);
    rayDesc.TMin = 0;
    rayDesc.TMax = TMAX;
}

float3 linearToSrgb(float3 c)
{
    float3 sq1 = sqrt(c);
    float3 sq2 = sqrt(sq1);
    float3 sq3 = sqrt(sq2);
    float3 srgb = 0.662002687 * sq1 + 0.684122060 * sq2 - 0.323583601 * sq3 - 0.0225411470 * c;
    return srgb;
}

float2 rand(float2 uv)
{
    float noiseX = (frac(sin(dot(uv, float2(12.9898, 78.233) * 2.0)) * 43758.5453));
    float noiseY = sqrt(1 - noiseX * noiseX);
    return float2(noiseX, noiseY);
}

float3 getRandInUnitSphere(in Payload payload)
{
    uint2 launchIdx = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    uint index = payload.AAIndex;
    
    uint2 coords = launchIdx;
    coords.x += (index * 29);
    coords.y += (index * 53);
    coords = abs(coords);
    coords = fmod(coords, launchDim);
    float3 randomPoint = gRandomNumbers[coords];
    
    return normalize(randomPoint);
}

float3 offsetRay(const float3 p, const float3 n)
{
    return p + n * _intersection_bias;
}
#endif // RAYTRACING_HLSL