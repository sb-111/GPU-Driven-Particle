#pragma once
#include "VectorMath.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "SDFDebugSettings.h"
class GraphicsContext;
namespace GP
{
	struct MeshSDF;

	__declspec(align(16)) struct SDFSliceCB
	{
		Math::Matrix4 localToWorld;
		Math::Matrix4 viewProj;

		float volumeBoundsMin[3]; uint32_t axis;
		float volumeBoundsSize[3]; uint32_t sliceIndex;

		uint32_t resolution[3]; uint32_t padding;
	};
	class SDFDebugRenderer
	{
	public:
		void Init();
		/**
		 * @brief MeshSDF의 선택된 slice를 월드 공간 quad로 렌더링
		 * 내부 = 파랑, 표면 = 검정, 외부 = 빨강
		 * 
		 * @param gfx            현재 프레임의 graphics command context
		 * @param sdf			 표시할 SDF volume과 grid 정보
		 * @param localToWorld   월드 변환 행렬
		 * @param viewProj       클립 변환 행렬
		 * @param axis           Slice 축 (X, Y, Z)
		 * @param sliceIndex     axis 방향에서 몇번 째 slice를 표시할 지
		 */
		void RenderSlice(
			GraphicsContext& gfx,
			MeshSDF& sdf,
			const Math::Matrix4& localToWorld,
			const Math::Matrix4& viewProj,
			ESDFSliceAxis axis,
			uint32_t sliceIndex);
	private:
		RootSignature m_RootSig;
		GraphicsPSO m_PSO;
	};


}
