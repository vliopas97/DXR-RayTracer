#include "Common.hlsli"

bool hasIntersection(in uint instanceID, inout IntersectionAttributes attribs)
{
    SphereInfo sphere = gSpheres[instanceID];
    
    float3 rayOrigin = WorldRayOrigin();
    float3 rayDirection = WorldRayDirection();
    float3 f = rayOrigin - sphere.Center;
    
    float a = dot(rayDirection, rayDirection);
    float b = 2 * dot(f, rayDirection);
    float c = dot(f, f) - sphere.Radius * sphere.Radius;
    
    float discrimininant = b * b - 4 * a * c;
    
    if (discrimininant < 0.0f)
        return false;
    
    float discSqrt = sqrt(discrimininant);
    float divisor = 1.0f / (2 * a);
    
    float t1 = (-b - discSqrt) * divisor;
    float t2 = (-b + discSqrt) * divisor;
    float t = (t1 >= RayTMin() && (t2 < RayTMin() || t1 < t2)) ? t1 : t2;

    if (t >= RayTMin())
    {
        attribs.hitT = t;
        attribs.instanceID = instanceID;
        return true;
    }
    
    return false;
}

[shader("intersection")]
void intersection()
{
    IntersectionAttributes intAttribs;
    intersectionAttributes_Initialize(intAttribs);
    
    bool isIntersecting = hasIntersection(InstanceID(), intAttribs);

    if (isIntersecting)
    {
        ReportHit(intAttribs.hitT, 0, intAttribs);
    }
}