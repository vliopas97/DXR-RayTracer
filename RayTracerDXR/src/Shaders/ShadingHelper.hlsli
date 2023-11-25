#include "Common.hlsli"

struct HitInfo
{
    float3 Point;
    float3 Normal;
};

HitInfo getHitInfo(in IntersectionAttributes attribs)
{
    HitInfo output;
    output.Point = WorldRayOrigin() + attribs.hitT * WorldRayDirection();
    output.Normal = normalize(output.Point - gSpheres[attribs.instanceID].Center);
    return output;
}

float3 reflect(in float3 v, in float3 n)
{
    return v - 2.0f * dot(v, n) * n;
}

// Bec's refraction formula
bool refract(in float eta, in float3 incidentRay, in float3 n, inout float3 refractedRay)
{
    // eta is the relative index of refraction (n1/n2)
    float c1 = -dot(incidentRay, n); // cos(theta1)
    float w = eta * c1;
    float c2m = (w - eta) * (w + eta); // cos^2(theta2) - 1;

    if (c2m < -1.0f)
        return false;
    
    refractedRay = eta * incidentRay + (w - sqrt(1.0f + c2m)) * n;
    return true;
}

// Schlick's approximation
float schlick(float cosine, float refIdx)
{
    float R0 = (1 - refIdx) / (1 + refIdx);
    R0 = R0 * R0;
    return lerp(R0, 1.0f, pow((1.0f - cosine), 5.0f));
}

bool scatterDiffuse(in IntersectionAttributes attribs, inout Payload payload, inout RayDesc scatterRay)
{
    uint instanceID = attribs.instanceID;
    HitInfo hit = getHitInfo(attribs);
    
    float3 randomPoint = getRandInUnitSphere(payload);
    float3 scatterDir = normalize((hit.Point + hit.Normal + randomPoint) - hit.Point);
    
    payload.color *= gSpheres[instanceID].Albedo;
    scatterRay.Origin = offsetRay(hit.Point, hit.Normal);
    scatterRay.Direction = scatterDir;

    return true;
}

bool scatterMetal(in IntersectionAttributes attribs, inout Payload payload, inout RayDesc scatterRay)
{
    uint instanceID = attribs.instanceID;
    SphereInfo sphere = gSpheres[instanceID];
    HitInfo hit = getHitInfo(attribs);
    float3 reflectionVector = reflect(normalize(WorldRayDirection()), hit.Normal);
    
    float roughness = gMaterials[sphere.Type].Roughness;

    payload.color *= gSpheres[instanceID].Albedo;
    scatterRay.Origin = offsetRay(hit.Point, hit.Normal);
    scatterRay.Direction = reflectionVector + pow(roughness, 2.0f) * getRandInUnitSphere(payload);
    return dot(scatterRay.Direction, hit.Normal) > 0.0f;
}

bool scatterDielectric(in IntersectionAttributes attribs, inout Payload payload, inout RayDesc scatterRay)
{
    uint instanceID = attribs.instanceID;
    SphereInfo sphere = gSpheres[instanceID];
    float3 indidentRay = WorldRayDirection();
    HitInfo hit = getHitInfo(attribs);
    
    payload.color *= sphere.Albedo;
    float refIdx = gMaterials[sphere.Type].Eta;
    
    float3 outwardNormal = float3(0, 0, 0);
    float3 reflected = reflect(indidentRay, hit.Normal);
    float3 refracted = float3(0, 0, 0);
    float n1Byn2 = 0.0f;
    float reflectProb = 1.0f;
    float cosine = 0.0f;
    
    bool cond = dot(indidentRay, hit.Normal) > 0.0f;
    outwardNormal = select(cond, -1, 1) * hit.Normal;
    n1Byn2 = select(cond, refIdx, 1.0f / refIdx);
    
    cosine = -dot(indidentRay, outwardNormal);
    
    if (refract(n1Byn2, indidentRay, outwardNormal, refracted))
        reflectProb = schlick(cosine, refIdx);
   
    if (reflectProb < 0.5f * getRandInUnitSphere(payload).x + 0.5f)
    {
        scatterRay.Origin = offsetRay(hit.Point, -outwardNormal);
        scatterRay.Direction = normalize(refracted);
    }
    else
    {
        scatterRay.Origin = offsetRay(hit.Point, hit.Normal);
        scatterRay.Direction = normalize(reflected);
    }
    return true;
}

bool scatter(in IntersectionAttributes attribs, inout Payload payload, inout RayDesc scatterRay)
{
    MaterialType type = gSpheres[attribs.instanceID].Type;
    switch (type)
    {
        case MaterialType::Diffuse:
            return scatterDiffuse(attribs, payload, scatterRay);
        case MaterialType::Metal:
            return scatterMetal(attribs, payload, scatterRay);
        case MaterialType::Dielectric:
            return scatterDielectric(attribs, payload, scatterRay);
        default:
            return false;
    }
}
