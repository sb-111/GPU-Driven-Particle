#pragma once

#include "pch.h"
#include "GpuResource.h"
#include "GpuBuffer.h"
#include "VolumeBuffer.h"
#include "RenderTypes.h"
#include <vector>
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
		// 정점 배열 및 인덱스 배열을 받아 메시 생성
		void Create(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
		void CreateSphere(float radius, uint32_t rings, uint32_t segments);

		uint32_t GetVertexCount() const { return m_VertexCount; }
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
		uint32_t m_VertexCount = 0;
		uint32_t m_IndexCount = 0;
		Math::Vector3 m_BoundsMin;
		Math::Vector3 m_BoundsMax;

		// 메시가 SDF 데이터 소유
		std::unique_ptr<MeshSDF> m_SDF;
	};
};
