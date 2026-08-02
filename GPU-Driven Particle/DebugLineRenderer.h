#pragma once

#include "VectorMath.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include <vector>

namespace GP
{
	struct alignas(16) DebugLineVertex
	{
		float position[3];
		float padding;
		float color[4];
	};
	static_assert(sizeof(DebugLineVertex) == 32);

	__declspec(align(16)) struct DebugLineCB
	{
		Math::Matrix4 viewProj;
	};

	class DebugLineRenderer
	{
	public:
		void Init();

		void AddLine(
			const Math::Vector3& start,
			const Math::Vector3& end,
			const Math::Vector4& color);

		void AddAABB(
			const Math::Vector3& boundsMin,
			const Math::Vector3& boundsMax,
			const Math::Vector4& color);

		// local AABB를 월드 공간 vertex로 변환
		void AddAABB(
			const Math::Vector3& boundsMin,
			const Math::Vector3& boundsMax,
			const Math::Matrix4& localToWorld,
			const Math::Vector4& color);

		void Render(GraphicsContext& gfx, const Math::Matrix4& viewProj);

		void Clear() { m_Vertices.clear(); }

	private:
		RootSignature m_RootSig;
		GraphicsPSO m_PSO;
		std::vector<DebugLineVertex> m_Vertices;
	};
}
