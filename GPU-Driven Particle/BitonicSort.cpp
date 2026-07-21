#include "BitonicSort.h"
#include "CommandContext.h"
#include "GpuResource.h"
#include "ShaderCompiler.h"
void GP::BitonicSort::Init()
{
	auto bitonicSortCS = CompileShader(L"BitonicSort.hlsl", L"main", L"cs_6_2");
	ASSERT(bitonicSortCS, "Bitonic Sort 셰이더 컴파일 실패");

	// 셰이더에서 필요한 것: UAV 2개, 루트 상수(k,j)
	m_RootSig.Reset(3, 0);
	m_RootSig[0].InitAsConstants(0, 2);
	m_RootSig[1].InitAsBufferUAV(0);
	m_RootSig[2].InitAsBufferUAV(1);
	m_RootSig.Finalize(L"Bitonic Sort Root Sig");

	// Compute PSO 정의
	m_SortPSO.SetRootSignature(m_RootSig);
	m_SortPSO.SetComputeShader(bitonicSortCS->GetBufferPointer(), bitonicSortCS->GetBufferSize());
	m_SortPSO.Finalize();

}
void GP::BitonicSort::Sort(ComputeContext& cpt, GpuBuffer& aliveList, GpuBuffer& sortKeys, uint32_t N)
{
	ASSERT(((N & (N - 1)) == 0 && N >= 64), "N이 2의 거듭제곱이어야 함");
	ScopedTimer _prof(L"Bitonic Sort", cpt);
	cpt.TransitionResource(aliveList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(sortKeys, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cpt.SetRootSignature(m_RootSig);
	cpt.SetBufferUAV(1, aliveList);
	cpt.SetBufferUAV(2, sortKeys);
	cpt.SetPipelineState(m_SortPSO);
	// stage는 2의 거듭제곱으로 증가 (한 stage의 목표: 올라가거나 내려가도록 정렬)
	// 예를들어 stage k가 2면 2개씩 묶은 블록에서 올라가거나 내려가거나
	for (uint32_t k = 2; k <= N; k *= 2)
	{
		// stage 내 pass: 자기 인덱스의 짝끼리 비교해서 정렬하기 위함
		for (uint32_t j = k / 2; j >= 1; j /= 2)
		{
			// 루트 상수 값은 매번 바뀌니 여기서
			cpt.SetConstants(0, k, j);
			// stage 내 pass에 대해서 Dispatch
			// 스레드 N개 일하도록 Dispatch (*실제 일하는 스레드는 N/2 -> 스왑은 한 쌍끼리 일어나기때문)
			// 그룹수 구해서 Dispatch 진행해주는 함수
			// Indirect 아닌 이유: 디스패치 여러개를 GPU 버퍼에서 읽어 실행해도 그 명령들 사이 배리어 못넣기때문
			cpt.Dispatch1D(N);

			// 다음 Dispatch 전에 배리어 삽입 (오염 방지)
			cpt.InsertUAVBarrier(aliveList);
			cpt.InsertUAVBarrier(sortKeys);

		}
	}
}
