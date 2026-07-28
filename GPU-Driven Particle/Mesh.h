#pragma once

#include "pch.h"
#include "GpuResource.h"
#include "GpuBuffer.h"
#include "VolumeBuffer.h"
namespace GP
{
	// 메시 로컬 공간에서 구운 SDF (SDFBaker가 생성)
	struct MeshSDF
	{
		VolumeBuffer volume;
		Math::Vector3 boundsMin;
		Math::Vector3 boundsSize;
		uint32_t resolution[3] = { 0,0,0 };
	};

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

		void SetSDF(std::unique_ptr<MeshSDF> sdf) { m_SDF = std::move(sdf); }
		MeshSDF* GetSDF() const { return m_SDF.get(); } // 비소유 참조
	private:
		ByteAddressBuffer m_VertexBuffer;
		ByteAddressBuffer m_IndexBuffer;
		uint32_t m_IndexCount = 0;
		Math::Vector3 m_BoundsMin;
		Math::Vector3 m_BoundsMax;

		// 메시가 SDF 데이터 소유
		std::unique_ptr<MeshSDF> m_SDF;
	};
};
