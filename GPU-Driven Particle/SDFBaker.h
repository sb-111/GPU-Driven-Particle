#pragma once

#include "pch.h"
#include "VolumeBuffer.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "ParticleShared.h"
namespace GP
{
	class Mesh;

	// SDF 생성 결과를 CPU에서 들고 있는 구조체
	struct MeshSDF
	{
		VolumeBuffer volume;
		Math::Vector3 boundsMin;
		Math::Vector3 boundsSize;
		uint32_t resolution[3] = { 0,0,0 };
	};
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
		// 1회 Dispatch
		void Bake(MeshSDF& out, Mesh& mesh, uint32_t resolutionX, uint32_t resolutionY, uint32_t resolutionZ);

	private:
		RootSignature bakeRootSig;
		ComputePSO bakePSO;
	};
}

