#pragma once
#include "VectorMath.h"
namespace GP
{

	struct SDFGrid
	{
		// mesh bounds + padding을 포함한 volume texture 범위
		Math::Vector3 volumeBoundsMin;
		Math::Vector3 volumeBoundsSize;
		uint32_t resolution[3] = { 0, 0, 0 };
		float voxelSize = 0.0f;
	};
	constexpr uint32_t kSDFLongestAxisResolution = 64; // padding 포함 가장 긴 축 resolution
	constexpr uint32_t kSDFPaddingVoxels = 2;          // Mesh 양쪽에 최소한 확보할 padding voxel 수
	constexpr uint32_t kSDFMinResolution = 8;
	/*
	* @param meshBoundsMin localMin
	* @param meshBoundsMax localMax
	* @param longestAxisResolution padding 포함 가장 긴 축의 최종 resolution
	*/
	SDFGrid ResolveSDFGrid(const Math::Vector3& meshBoundsMin, const Math::Vector3& meshBoundsMax, uint32_t longestAxisResolution);

}
