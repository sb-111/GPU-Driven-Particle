#pragma once
#include <cstdint>
#include <cstddef>
#include <d3d12.h>

namespace GP
{
	struct Vertex
	{
		float position[3];
		float normal[3];
	};

	constexpr uint32_t kVertexStride = (uint32_t)sizeof(Vertex);
	constexpr uint32_t kPositionOffset = (uint32_t)offsetof(Vertex, position);

	static_assert(sizeof(Vertex) == 24, "정점 포맷을 바꿨다면 kVertexLayout과 셰이더도 같이 갱신할 것");

	// PSO 생성부는 이 배열만 참조 (Input Layout 하드코딩 금지)
	inline const D3D12_INPUT_ELEMENT_DESC kVertexLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex, normal),   D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	constexpr uint32_t kVertexLayoutCount = (uint32_t)(sizeof(kVertexLayout) / sizeof(kVertexLayout[0]));

	// 오브젝트 단위 머티리얼 (b1)
	// 색은 머티리얼 소유
	__declspec(align(16)) struct MaterialCB
	{
		float baseColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		// 향후 확장 가능: roughness, metallic, textureIndex ...
	};
}
