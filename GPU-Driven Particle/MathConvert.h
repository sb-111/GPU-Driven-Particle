#pragma once
#include "VectorMath.h"
#include "ParticleShared.h"

namespace GP
{
	// Vector3(XMVECTOR) -> float3(12B, CB용)
	inline float3 ToF3(const Math::Vector3& v)
	{
		float3 r;
		DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)&r, v);
		return r;
	}
	// Matrix4(XMMATRIX) -> float4x4(float 16개, CB용)
	inline float4x4 ToF4x4(const Math::Matrix4& m)
	{
		float4x4 r;
		DirectX::XMStoreFloat4x4(reinterpret_cast<DirectX::XMFLOAT4X4*>(&r), m);
		return r;
	}
}
