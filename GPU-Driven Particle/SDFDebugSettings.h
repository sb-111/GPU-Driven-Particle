#pragma once

#include <cstdint>

namespace GP
{
	enum class ESDFSliceAxis : uint32_t
	{
		X = 0,
		Y = 1,
		Z = 2,
	};

	// Panel/Main이 소유하는 표시 설정
	struct SDFDebugSettings
	{
		bool showDebug = true;
		ESDFSliceAxis axis = ESDFSliceAxis::Z;
		uint32_t sliceIndex = 0;
		bool resetSliceToCenter = true;
	};
}
