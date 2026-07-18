#pragma once
#include "pch.h"
#include "RootSignature.h"
#include "PipelineState.h"

class ComputeContext;
class GpuBuffer;

namespace GP
{
	class BitonicSort
	{
	public:
		void Init();

		// 내부에서 상태 전환/동기화 처리 - 호출자는 배리어 불필요
		void Sort(ComputeContext& cpt, GpuBuffer & aliveList, GpuBuffer& sortKeys, uint32_t N);

	private:
		ComputePSO m_SortPSO;
		RootSignature m_RootSig;
	};
}
