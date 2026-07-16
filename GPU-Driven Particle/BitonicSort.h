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
		void Sort(ComputeContext& cpt, GpuBuffer & aliveList, GpuBuffer& sortKeys, uint32_t N);

	private:
		ComputePSO m_SortPSO;
		RootSignature m_RootSig;
	};
}
