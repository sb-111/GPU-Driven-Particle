#pragma once

#include "pch.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "ParticleShared.h"
namespace GP
{
	class Mesh;

	__declspec(align(16)) struct BakeConstants
	{
		float3 boundsMin; uint32_t triangleCount;
		float3 boundsSize; uint32_t pad0;
		uint32_t resolution[3]; uint32_t pad1;
	};
	class SDFBaker
	{
	public:
		void Init();
		// 1회 Dispatch로 구운 결과(MeshSDF)를 Mesh로 이동
		void Bake(Mesh& mesh, uint32_t resolutionX, uint32_t resolutionY, uint32_t resolutionZ);

	private:
		RootSignature bakeRootSig;
		ComputePSO bakePSO;
	};
}

