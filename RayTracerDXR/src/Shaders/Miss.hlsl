#include "Common.hlsli"

float3 skyColorCalc(float3 rayOrigin, float3 rayDirection)
{
    float weight = 0.5f * (rayDirection.y + 1.0f);
    return (1.0f - weight) * float3(1.0f, 1.0f, 1.0f) + weight * float3(0.5f, 0.7f, 1.0f);
}

[shader("miss")]
void miss(inout Payload payload)
{
    payload.color *= skyColorCalc(WorldRayOrigin(), WorldRayDirection());
}
