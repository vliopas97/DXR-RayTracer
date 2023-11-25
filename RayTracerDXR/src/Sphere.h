#pragma once

#include "Core.h"
#include "Shaders/HLSLCompat.h"

struct Sphere
{
	Sphere(glm::vec3 center = glm::vec3(0.0f), 
		   float radius = 1.0f, 
		   glm::vec3 albedo = glm::vec3(1.0f),
		   MaterialType type = MaterialType::Diffuse);
	Sphere(const Sphere& other);
	D3D12_RAYTRACING_AABB GetAABB() const;
	glm::mat4x4 GetInstanceTransform() const;

	operator SphereInfo() const;

	static ID3D12ResourcePtr CreateSphereAABB(ID3D12Device5Ptr pDevice);

	union
	{
		struct
		{
			glm::vec3 Center;
			float Radius;
			glm::vec3 Albedo;
			MaterialType Type;
		};
		SphereInfo Data;
	};
};

struct SphereComposite
{
	using ValueType = Sphere;
	
	SphereComposite() = default;

	void AddSphere(const Sphere& sphere);

	inline void SetDevice(ID3D12Device5Ptr device) { Device = device; }
	
	inline uint32_t Size() const { return static_cast<uint32_t>(Spheres.size()); }
	inline const ValueType* Data() const { return Spheres.data(); }
	inline ID3D12Resource* GetBufferPtr() { return Buffer.GetInterfacePtr(); }

	inline ValueType& operator[](size_t i) { return Spheres[i]; }
	inline const ValueType& operator[](size_t i) const { return Spheres[i]; }

private:
	void UpdateBuffer();

private:
	std::vector<ValueType> Spheres;
	ID3D12ResourcePtr Buffer;
	ID3D12Device5Ptr Device;
};