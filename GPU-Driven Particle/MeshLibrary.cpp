#include "MeshLibrary.h"
#include "Mesh.h"
#include "SystemTime.h"
#include "Utility.h"

#include <algorithm>
#include <filesystem>
namespace GP
{
	MeshLibrary::MeshLibrary()
	{
		DiscoverMeshAssets();
	}
	MeshLibrary::~MeshLibrary() = default;

	void GP::MeshLibrary::DiscoverMeshAssets()
	{
		namespace fs = std::filesystem;
		std::error_code error;
		const fs::path root("Meshes");
		if (!fs::exists(root, error))
			return;

		for (const fs::directory_entry& entry : fs::recursive_directory_iterator(root, error))
		{
			if (error)
				break;

			if (!entry.is_regular_file(error) || entry.path().extension() != ".obj")
				continue;

			m_AssetPaths.push_back(entry.path().generic_string());
		}

		std::sort(m_AssetPaths.begin(), m_AssetPaths.end());
	}

	Mesh* GP::MeshLibrary::Get(const char* path, ENormalMode mode)
	{
		// 이미 테이블에 있으면 반환
		auto it = m_MeshTable.find(path);
		if (it != m_MeshTable.end())
			return it->second;

		// 테이블에 없으면 로드 후 반환
		// OBJ -> RawMesh -> (노멀 보정) -> 정점 팩 -> GPU 업로드
		RawMesh raw;
		std::string err;
		int64_t start = SystemTime::GetCurrentTick();
		if (!LoadOBJ(path, raw, &err))
		{
			Utility::Printf("[Mesh] %s 로드 실패 : %s\n", path, err.c_str());
			return nullptr;
		}

		EnsureNormals(raw, mode);

		std::vector<Vertex> verts;
		PackVertices(raw, verts);

		auto mesh = std::make_unique<Mesh>();
		mesh->Create(verts, raw.indices);
		mesh->SetSourcePath(path);

		Utility::Printf("[Mesh] %s: 정점 %u, 삼각형 %u, %.1f ms\n",
			path, raw.VertexCount(), raw.TriangleCount(),
			SystemTime::TicksToMillisecs(SystemTime::GetCurrentTick() - start));

		Mesh* result = mesh.get();
		m_Meshes.push_back(std::move(mesh));
		m_MeshTable.emplace(path, result);
		return result; 
	}

	void GP::MeshLibrary::Clear()
	{
		m_MeshTable.clear();
		m_Meshes.clear();
	}

}
