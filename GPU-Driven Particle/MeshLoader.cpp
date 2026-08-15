#include "MeshLoader.h"
#include "ThirdParty/tiny_obj_loader.h"

#include <unordered_map>
#include <map>
#include <filesystem>
#include <cmath>

namespace
{
	// OBJ는 위치/노멀/UV가 각자 인덱스를 가짐
	// GPU는 정점당 인덱스 하나뿐이라 (v, vn, vt) 조합마다 정점을 새로 만듦
	// 노멀/UV 없으면 조합이 v 하나로 접힘
	struct IndexKey
	{
		int v, vn, vt;
		bool operator==(const IndexKey& o) const { return v == o.v && vn == o.vn && vt == o.vt; }
	};

	struct IndexKeyHash
	{
		size_t operator()(const IndexKey& k) const
		{
			size_t h = (size_t)(uint32_t)k.v * 73856093u;
			h ^= (size_t)(uint32_t)(k.vn + 1) * 19349663u;
			h ^= (size_t)(uint32_t)(k.vt + 1) * 83492791u;
			return h;
		}
	};

	// shape 이름에 이게 있으면 최고 해상도 LOD
	constexpr const char* kLODTag = "LOD1";

	void AppendVertex(GP::RawMesh& dst, const GP::RawMesh& src, uint32_t srcVertex)
	{
		dst.positions.push_back(src.positions[srcVertex * 3 + 0]);
		dst.positions.push_back(src.positions[srcVertex * 3 + 1]);
		dst.positions.push_back(src.positions[srcVertex * 3 + 2]);

		if (!src.uvs.empty())
		{
			dst.uvs.push_back(src.uvs[srcVertex * 2 + 0]);
			dst.uvs.push_back(src.uvs[srcVertex * 2 + 1]);
		}
		if (!src.colors.empty())
		{
			dst.colors.push_back(src.colors[srcVertex * 3 + 0]);
			dst.colors.push_back(src.colors[srcVertex * 3 + 1]);
			dst.colors.push_back(src.colors[srcVertex * 3 + 2]);
		}
	}
}

bool GP::LoadOBJ(const char* path, RawMesh& out, std::string* outError, const LoadOptions& options)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	// mtl은 obj와 같은 폴더에 있음. tinyobj는 basedir 끝에 구분자가 있어야 찾음
	std::string baseDir;
	if (options.loadMaterials)
	{
		const std::filesystem::path parent = std::filesystem::path(path).parent_path();
		baseDir = parent.empty() ? "./" : (parent.generic_string() + "/");
	}

	// triangulate = true 라 사각형 이상도 삼각형으로 쪼개져서 나옴
	bool ok = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path,
		baseDir.empty() ? nullptr : baseDir.c_str(), true, false);
	if (!ok)
	{
		if (outError) *outError = err.empty() ? "LoadObj 실패" : err;
		return false;
	}
	if (attrib.vertices.empty())
	{
		if (outError) *outError = "정점이 없음";
		return false;
	}

	const bool hasNormals = !attrib.normals.empty();
	const bool hasUVs = !attrib.texcoords.empty();
	const bool hasColors = attrib.colors.size() == attrib.vertices.size();

	out = RawMesh();

	out.materials.reserve(materials.size());
	for (const tinyobj::material_t& src : materials)
	{
		RawMaterial dst;
		dst.name = src.name;
		dst.baseColor[0] = src.diffuse[0];
		dst.baseColor[1] = src.diffuse[1];
		dst.baseColor[2] = src.diffuse[2];
		dst.baseColor[3] = src.dissolve;
		dst.emissive[0] = src.emission[0];
		dst.emissive[1] = src.emission[1];
		dst.emissive[2] = src.emission[2];
		out.materials.push_back(std::move(dst));
	}

	// LOD1이 있으면 그것만, 없으면 전부
	std::vector<const tinyobj::shape_t*> selected;
	if (options.preferLOD1)
	{
		for (const tinyobj::shape_t& shape : shapes)
			if (shape.name.find(kLODTag) != std::string::npos)
				selected.push_back(&shape);
	}
	if (selected.empty())
	{
		for (const tinyobj::shape_t& shape : shapes)
			selected.push_back(&shape);
	}

	std::unordered_map<IndexKey, uint32_t, IndexKeyHash> unique;
	unique.reserve(attrib.vertices.size() / 3);

	// 정점은 shape을 가로질러 공유하고 인덱스만 머티리얼별로 모음
	// map이라 섹션 순서가 매번 같음 (-1 다음 id 오름차순)
	std::map<int, std::vector<uint32_t>> buckets;

	for (const tinyobj::shape_t* shapePtr : selected)
	{
		const tinyobj::shape_t& shape = *shapePtr;
		const size_t faceCount = shape.mesh.indices.size() / 3;

		// 크기가 어긋나면 인덱싱이 나가므로 통째로 머티리얼 없음 처리
		const bool hasFaceMaterial = shape.mesh.material_ids.size() == faceCount;

		for (size_t f = 0; f < faceCount; ++f)
		{
			int materialId = -1;
			if (hasFaceMaterial)
			{
				const int id = shape.mesh.material_ids[f];
				if (id >= 0 && (size_t)id < out.materials.size())
					materialId = id;
			}
			std::vector<uint32_t>& bucket = buckets[materialId];

			for (size_t k = 0; k < 3; ++k)
			{
				const tinyobj::index_t& idx = shape.mesh.indices[f * 3 + k];
				IndexKey key{ idx.vertex_index,
							  hasNormals ? idx.normal_index : -1,
							  hasUVs ? idx.texcoord_index : -1 };

				auto it = unique.find(key);
				if (it == unique.end())
				{
					uint32_t newIndex = out.VertexCount();

					out.positions.push_back(attrib.vertices[key.v * 3 + 0]);
					out.positions.push_back(attrib.vertices[key.v * 3 + 1]);
					out.positions.push_back(attrib.vertices[key.v * 3 + 2]);

					if (hasNormals && key.vn >= 0)
					{
						out.normals.push_back(attrib.normals[key.vn * 3 + 0]);
						out.normals.push_back(attrib.normals[key.vn * 3 + 1]);
						out.normals.push_back(attrib.normals[key.vn * 3 + 2]);
					}
					if (hasUVs && key.vt >= 0)
					{
						out.uvs.push_back(attrib.texcoords[key.vt * 2 + 0]);
						out.uvs.push_back(attrib.texcoords[key.vt * 2 + 1]);
					}
					if (hasColors)
					{
						out.colors.push_back(attrib.colors[key.v * 3 + 0]);
						out.colors.push_back(attrib.colors[key.v * 3 + 1]);
						out.colors.push_back(attrib.colors[key.v * 3 + 2]);
					}

					it = unique.emplace(key, newIndex).first;
				}
				bucket.push_back(it->second);
			}
		}
	}

	for (const std::pair<const int, std::vector<uint32_t>>& bucket : buckets)
	{
		if (bucket.second.empty())
			continue;

		RawSection section;
		section.indexOffset = (uint32_t)out.indices.size();
		section.indexCount = (uint32_t)bucket.second.size();
		section.materialIndex = bucket.first;
		out.sections.push_back(section);

		out.indices.insert(out.indices.end(), bucket.second.begin(), bucket.second.end());
	}

	// 노멀이 일부 면에만 있으면 개수가 어긋남. 그 경우 통째로 버리고 재생성
	if (out.normals.size() != out.positions.size())
		out.normals.clear();

	if (out.indices.empty())
	{
		if (outError) *outError = "삼각형이 없음";
		return false;
	}

	// 그리는 쪽이 섹션 루프 하나로 끝내도록 항상 1개는 남김
	if (out.sections.empty())
		out.sections.push_back(RawSection{ 0, (uint32_t)out.indices.size(), -1 });

	return true;
}

void GP::EnsureNormals(RawMesh& mesh, ENormalMode mode)
{
	if (mesh.normals.size() == mesh.positions.size() && !mesh.normals.empty())
		return;

	const uint32_t triCount = mesh.TriangleCount();

	if (mode == ENormalMode::Flat)
	{
		// 삼각형마다 정점 3개 새로 만듦 (공유되면 면 노멀이 뭉개짐)
		RawMesh split;
		split.positions.reserve(mesh.positions.size());
		split.normals.reserve(mesh.positions.size());
		split.indices.reserve(mesh.indices.size());

		// 삼각형 순서를 안 바꾸고 인덱스만 0부터 다시 매기므로 섹션 구간은 그대로 유효
		// 여기서 인덱스를 재정렬하면 섹션이 깨짐
		split.sections = mesh.sections;
		split.materials = mesh.materials;

		for (uint32_t t = 0; t < triCount; ++t)
		{
			const uint32_t i0 = mesh.indices[t * 3 + 0];
			const uint32_t i1 = mesh.indices[t * 3 + 1];
			const uint32_t i2 = mesh.indices[t * 3 + 2];

			const float* p0 = &mesh.positions[i0 * 3];
			const float* p1 = &mesh.positions[i1 * 3];
			const float* p2 = &mesh.positions[i2 * 3];

			float e1[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
			float e2[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
			float n[3] = { e1[1] * e2[2] - e1[2] * e2[1],
							e1[2] * e2[0] - e1[0] * e2[2],
							e1[0] * e2[1] - e1[1] * e2[0] };
			float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
			if (len > 1e-20f) { n[0] /= len; n[1] /= len; n[2] /= len; }
			else { n[0] = 0.0f; n[1] = 1.0f; n[2] = 0.0f; }

			const uint32_t src[3] = { i0, i1, i2 };
			for (int k = 0; k < 3; ++k)
			{
				split.indices.push_back(split.VertexCount());
				AppendVertex(split, mesh, src[k]);
				split.normals.push_back(n[0]);
				split.normals.push_back(n[1]);
				split.normals.push_back(n[2]);
			}
		}
		mesh = std::move(split);
		return;
	}

	// Smooth: 면 노멀을 정점에 누적 후 정규화
	// 정규화 안 한 크로스곱을 더하므로 면적 가중이 자연히 걸림
	mesh.normals.assign(mesh.positions.size(), 0.0f);

	for (uint32_t t = 0; t < triCount; ++t)
	{
		const uint32_t i0 = mesh.indices[t * 3 + 0];
		const uint32_t i1 = mesh.indices[t * 3 + 1];
		const uint32_t i2 = mesh.indices[t * 3 + 2];

		const float* p0 = &mesh.positions[i0 * 3];
		const float* p1 = &mesh.positions[i1 * 3];
		const float* p2 = &mesh.positions[i2 * 3];

		float e1[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] };
		float e2[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
		float n[3] = { e1[1] * e2[2] - e1[2] * e2[1],
						e1[2] * e2[0] - e1[0] * e2[2],
						e1[0] * e2[1] - e1[1] * e2[0] };

		const uint32_t tri[3] = { i0, i1, i2 };
		for (int k = 0; k < 3; ++k)
			for (int c = 0; c < 3; ++c)
				mesh.normals[tri[k] * 3 + c] += n[c];
	}

	const uint32_t vertCount = mesh.VertexCount();
	for (uint32_t v = 0; v < vertCount; ++v)
	{
		float* n = &mesh.normals[v * 3];
		float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (len > 1e-20f) { n[0] /= len; n[1] /= len; n[2] /= len; }
		else { n[0] = 0.0f; n[1] = 1.0f; n[2] = 0.0f; } // 고립 정점
	}
}

void GP::PackVertices(const RawMesh& mesh, std::vector<Vertex>& outVerts)
{
	const uint32_t vertCount = mesh.VertexCount();
	const bool hasNormals = mesh.normals.size() == mesh.positions.size();

	outVerts.clear();
	outVerts.resize(vertCount);

	for (uint32_t v = 0; v < vertCount; ++v)
	{
		Vertex& dst = outVerts[v];
		dst.position[0] = mesh.positions[v * 3 + 0];
		dst.position[1] = mesh.positions[v * 3 + 1];
		dst.position[2] = mesh.positions[v * 3 + 2];

		if (hasNormals)
		{
			dst.normal[0] = mesh.normals[v * 3 + 0];
			dst.normal[1] = mesh.normals[v * 3 + 1];
			dst.normal[2] = mesh.normals[v * 3 + 2];
		}
		else
		{
			dst.normal[0] = 0.0f; dst.normal[1] = 1.0f; dst.normal[2] = 0.0f;
		}
		if (mesh.uvs.size() >= (v + 1) * 2)
		{
			dst.uv[0] = mesh.uvs[v * 2 + 0];
			dst.uv[1] = mesh.uvs[v * 2 + 1];
		}
		else
		{
			dst.uv[0] = 0.0f;
			dst.uv[1] = 0.0f;
		}
		// colors는 현재 정점 포맷에 없어서 버림
		// 속성 추가 시 Vertex 선언과 여기 세 줄만 늘리면 됨
	}
}
