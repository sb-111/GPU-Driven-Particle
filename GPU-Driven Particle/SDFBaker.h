#pragma once

#include "pch.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "ParticleShared.h"
#include "SDFGrid.h"
namespace GP
{
	class Mesh;

	// vertexStride/positionOffset 을 넘기면
	// Vertex 포맷이 바뀌어도 MeshSDFBakeCS는 손댈 필요가 없음
	__declspec(align(16)) struct BakeConstants
	{
		float3 boundsMin; uint32_t triangleCount;
		float3 boundsSize; uint32_t vertexStride;
		uint32_t resolution[3]; uint32_t positionOffset;
	};
	class SDFBaker
	{
	public:
		void Init();
		/*
		* 1회 Dispatch하여 Bake
		* @param Mesh 해당 Mesh에 구워진 MeshSDF를 이동
		* @param baseResolution padding을 포함한 가장 긴 축의 최종 해상도
		*/
		void Bake(Mesh& mesh, uint32_t baseResolution = kSDFBaseResolution);

	private:
		RootSignature bakeRootSig;
		ComputePSO bakePSO;
	};
}

