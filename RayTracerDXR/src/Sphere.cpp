#include "Sphere.h"
#include "Utils.h"

Sphere::Sphere(glm::vec3 center,
			   float radius,
			   glm::vec3 albedo,
			   MaterialType type)
	:Center(center), Radius(radius), Albedo(albedo), Type(type)
{}

Sphere::Sphere(const Sphere & other)
{
	Center = other.Center;
	Radius = other.Radius;
	Albedo = other.Albedo;
	Type = other.Type;
}

D3D12_RAYTRACING_AABB Sphere::GetAABB() const
{
	auto lowerEdge = Center - Radius;
	auto higherEdge = Center + Radius;
	return { lowerEdge.x, lowerEdge.y, lowerEdge.z, higherEdge.x, higherEdge.y, higherEdge.z };
}

glm::mat4x4 Sphere::GetInstanceTransform() const
{
	float scalingFactor = Radius / 1.0f;
	return glm::transpose(glm::translate(glm::mat4(1.0f), glm::vec3(Center)) * glm::scale(glm::vec3(scalingFactor)) * glm::mat4(1.0f));
}

Sphere::operator SphereInfo() const
{
	return Data;
}

ID3D12ResourcePtr Sphere::CreateSphereAABB(ID3D12Device5Ptr pDevice)
{
	auto aabb = Sphere{}.GetAABB();
	ID3D12ResourcePtr pBuffer = D3D::CreateBuffer(pDevice, sizeof(aabb), D3D12_RESOURCE_FLAG_NONE,
												  D3D12_RESOURCE_STATE_GENERIC_READ, D3D::UploadHeapProps);

	uint8_t* pData;
	pBuffer->Map(0, nullptr, (void**)&pData);
	std::memcpy(pData, &aabb, sizeof(aabb));
	pBuffer->Unmap(0, nullptr);
	return pBuffer;
}

void SphereComposite::AddSphere(const Sphere& sphere)
{
	Spheres.push_back(sphere);
	UpdateBuffer();
}

void SphereComposite::UpdateBuffer()
{
	if (Buffer != nullptr)
		Buffer.Release();
	
	Buffer = D3D::CreateBuffer(Device, sizeof(SphereInfo) * Spheres.size(), D3D12_RESOURCE_FLAG_NONE,
							   D3D12_RESOURCE_STATE_GENERIC_READ, D3D::UploadHeapProps);

	uint8_t* pData;
	Buffer->Map(0, nullptr, (void**)&pData);
	std::memcpy(pData, Spheres.data(), sizeof(SphereInfo) * Spheres.size());
	Buffer->Unmap(0, nullptr);
}