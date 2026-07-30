#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "RenderTypes.h"

namespace GP
{
	// 포맷 중간 표현 (로더는 이것만 채우고 GPU 정점 레이아웃 모름)
	// 추후 glTF 로더 붙여도 이 구조체만 채우면 됨
	struct RawMesh
	{
		// 속성 추가 시 RenderTypes.h, PackVertices, 셰이더 수정할 것
		std::vector<float>    positions; // xyz
		std::vector<float>    normals;   // xyz, 비어 있거나 정점 수가 positions와 같음
		std::vector<float>    uvs;       // xy
		std::vector<float>    colors;    // rgb (현재는 팩 단계에서 버림)
		std::vector<uint32_t> indices;

		uint32_t VertexCount() const { return (uint32_t)(positions.size() / 3); }
		uint32_t TriangleCount() const { return (uint32_t)(indices.size() / 3); }
	};

	enum class ENormalMode { Smooth, Flat };

	// OBJ -> RawMesh. 전 shape 하나로 합치고 삼각분할까지 함
	bool LoadOBJ(const char* path, RawMesh& out, std::string* outError = nullptr);

	// 노멀이 이미 있으면 그대로 두고, 없을 때만 생성
	// Flat은 삼각형마다 정점을 분리해서 정점 수가 늘어남
	void EnsureNormals(RawMesh& mesh, ENormalMode mode);

	// RawMesh -> GPU 정점 배열
	// 정점 레이아웃을 아는 유일한 함수
	void PackVertices(const RawMesh& mesh, std::vector<Vertex>& outVerts);
}
