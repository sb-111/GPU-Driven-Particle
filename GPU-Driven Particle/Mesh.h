#pragma once

#include "pch.h"
#include "GpuResource.h"
#include "GpuBuffer.h"
#include "VolumeBuffer.h"
#include "RenderTypes.h"
#include "SDFGrid.h"
#include <vector>
#include <string>
namespace GP
{
	// Mesh에 대해 구워진 SDF 결과
	struct MeshSDF
	{
		VolumeBuffer volume; // GPU 리소스
		SDFGrid grid;		 // 텍스처 해석 데이터

		double bakeMs = 0.0;			// 베이크 계측용	
		uint64_t allocatedBytes = 0;    // 드라이버가 할당한 크기 
	};

	// 메시가 배열로 소유. 텍스처를 붙이면 여기에
	struct MeshMaterial
	{
		std::string name;
		MaterialCB params;
	};

	// 같은 머티리얼을 쓰는 인덱스 구간
	struct SubMesh
	{
		uint32_t indexOffset = 0;
		uint32_t indexCount = 0;
		int materialSlot = -1; // GetMaterials() 인덱스, -1이면 머티리얼 없음
	};

	class Mesh
	{
	public:
		Mesh() {}
		// 정점 배열 및 인덱스 배열을 받아 메시 생성
		void Create(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices);
		void Create(const std::vector<Vertex>& verts, const std::vector<uint32_t>& indices,
			const std::vector<SubMesh>& sections, const std::vector<MeshMaterial>& materials);
		void CreateSphere(float radius, uint32_t rings, uint32_t segments);

		uint32_t GetVertexCount() const { return m_VertexCount; }
		ByteAddressBuffer& GetVertexBuffer()  { return m_VertexBuffer; }
		ByteAddressBuffer& GetIndexBuffer()  { return m_IndexBuffer; }
		uint32_t GetIndexCount() const { return m_IndexCount; }

		// 항상 1개 이상 (머티리얼 없으면 전체를 덮는 1개)
		const std::vector<SubMesh>& GetSections() const { return m_Sections; }
		const std::vector<MeshMaterial>& GetMaterials() const { return m_Materials; }
		bool HasMaterials() const { return !m_Materials.empty(); }
		const MaterialCB* FindMaterial(int slot) const; // 슬롯이 비면 nullptr
		Math::Vector3 GetBoundsMin() const { return m_BoundsMin; }
		Math::Vector3 GetBoundsMax() const { return m_BoundsMax; }

		const std::vector<Vertex>& GetCPUVertices() const { return m_CPUVertices; }
		const std::vector<uint32_t>& GetCPUIndices() const { return m_CPUIndices; }

		void SetSDF(std::unique_ptr<MeshSDF> sdf) { m_SDF = std::move(sdf); }
		MeshSDF* GetSDF() const { return m_SDF.get(); } // 비소유 참조

		void SetSourcePath(const char* path) { m_SourcePath = path; }
		const std::string& GetSourcePath() const { return m_SourcePath; }
	private:
		ByteAddressBuffer m_VertexBuffer;
		ByteAddressBuffer m_IndexBuffer;
		uint32_t m_VertexCount = 0;
		uint32_t m_IndexCount = 0;
		Math::Vector3 m_BoundsMin;
		Math::Vector3 m_BoundsMax;
		std::string m_SourcePath; // 로드된 파일 경로 (스탯 표시용)

		std::vector<SubMesh> m_Sections;
		std::vector<MeshMaterial> m_Materials;

		std::vector<Vertex> m_CPUVertices;
		std::vector<uint32_t> m_CPUIndices;

		// 메시가 SDF 데이터 소유
		std::unique_ptr<MeshSDF> m_SDF;
	};
};
