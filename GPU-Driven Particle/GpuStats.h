#pragma once
#include <cstdint>

struct ID3D12Resource;

namespace GP
{
	struct VideoMemoryInfo
	{
		uint64_t budget = 0;       // 드라이버가 허용하는 예산
		uint64_t currentUsage = 0; // 현재 사용량
		bool valid = false;
	};

	// 로컬(전용) 비디오 메모리 현황. 어댑터를 못 잡으면 valid = false.
	VideoMemoryInfo QueryVideoMemory();

	// 리소스가 실제로 차지하는 바이트 (Texture3D는 정렬 패딩이 붙어 이론값과 다를 수 있음)
	uint64_t QueryResourceFootprint(ID3D12Resource* resource);
}
