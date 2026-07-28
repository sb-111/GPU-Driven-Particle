#pragma once

#include "pch.h"
#include "GpuResource.h"
#include "GpuBuffer.h"
namespace GP
{
	class Mesh
	{
	public:
		Mesh() {}
		void CreateSphere(float radius, uint32_t rings, uint32_t segments, const float color[4]);
		// TODO: Create(정점배열, 인덱스배열)
		ByteAddressBuffer& GetVertexBuffer()  { return m_VertexBuffer; }
		ByteAddressBuffer& GetIndexBuffer()  { return m_IndexBuffer; }
		uint32_t GetIndexCount() const { return m_IndexCount; }
		Math::Vector3 GetBoundsMin() const { return m_BoundsMin; }
		Math::Vector3 GetBoundsMax() const { return m_BoundsMax; }
	private:
		ByteAddressBuffer m_VertexBuffer;
		ByteAddressBuffer m_IndexBuffer;
		uint32_t m_IndexCount = 0;
		Math::Vector3 m_BoundsMin;
		Math::Vector3 m_BoundsMax;

	};
};
