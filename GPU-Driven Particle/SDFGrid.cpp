#include "pch.h"
#include "SDFGrid.h"

GP::SDFGrid GP::ResolveSDFGrid(
	const Math::Vector3& meshBoundsMin,
	const Math::Vector3& meshBoundsMax,
	uint32_t longestAxisResolution)
{
	const Math::Vector3 extents = meshBoundsMax - meshBoundsMin;
	const Math::Vector3 center = (meshBoundsMin + meshBoundsMax) * 0.5f;

	// 로컬 기준 각 축 길이
	const float extent[3] =
	{
		extents.GetX(),
		extents.GetY(),
		extents.GetZ()
	};
	// 가장 긴 축
	uint32_t longestAxis = 0;
	if (extent[1] > extent[longestAxis]) longestAxis = 1;
	if (extent[2] > extent[longestAxis]) longestAxis = 2;
	const float longestExtent = extent[longestAxis];

	ASSERT(longestAxisResolution > kSDFPaddingVoxels * 2,
		"SDF longest axis resolution must exceed total padding");
	ASSERT(
		longestAxisResolution >= kSDFMinResolution,
		"SDF longest axis resolution must be >= minimum resolution");
	ASSERT(longestExtent > 1e-6f,
		"Cannot build an SDF for a zero-size mesh");

	const uint32_t interiorResolution =
		longestAxisResolution - kSDFPaddingVoxels * 2;

	// padding 제외한 resolution으로 voxel size 계산
	const float voxelSize =
		longestExtent / static_cast<float>(interiorResolution);

	SDFGrid grid = {};
	grid.voxelSize = voxelSize;

	float volumeSize[3] = {};

	for (uint32_t axis = 0; axis < 3; ++axis)
	{
		uint32_t interiorVoxelCount;
		if (axis == longestAxis)
		{
			// 해당 축이 voxel size의 기준이니 역산 x
			interiorVoxelCount = interiorResolution;
		}
		else
		{
			interiorVoxelCount =
				static_cast<uint32_t>(
					std::ceil(extent[axis] / voxelSize));
		}

		// 축별 최종 voxel 수 계산
		grid.resolution[axis] = std::max(
			kSDFMinResolution,
			interiorVoxelCount + kSDFPaddingVoxels * 2);

		// sdf bounds = resolution * voxelSize
		volumeSize[axis] =
			static_cast<float>(grid.resolution[axis]) * voxelSize;
	}

	grid.volumeBoundsSize = Math::Vector3(
		volumeSize[0],
		volumeSize[1],
		volumeSize[2]);

	grid.volumeBoundsMin =
		center - grid.volumeBoundsSize * 0.5f;

	const float hx =
		static_cast<float>(grid.volumeBoundsSize.GetX()) /
		static_cast<float>(grid.resolution[0]);

	const float hy =
		static_cast<float>(grid.volumeBoundsSize.GetY()) /
		static_cast<float>(grid.resolution[1]);

	const float hz =
		static_cast<float>(grid.volumeBoundsSize.GetZ()) /
		static_cast<float>(grid.resolution[2]);

	ASSERT(
		fabsf(hx - hy) < 1e-5f &&
		fabsf(hy - hz) < 1e-5f,
		"SDF grid must use cubic voxels");

	return grid;
}
