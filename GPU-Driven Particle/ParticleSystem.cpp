#include "ParticleSystem.h"
#include "GameCore.h"
#include "GraphicsCommon.h"
#include "BufferManager.h"   // g_SceneColorBuffer / g_SceneDepthBuffer
#include "CommandContext.h"
#include "Camera.h"

using namespace GameCore;
using namespace Graphics;
using namespace DirectX;

// Vector3 형(16바이트) -> 자체 정의 GP::float3(12바이트) 변환
static inline GP::float3 ToF3(const Math::Vector3& v)
{
	GP::float3 r;
	DirectX::XMStoreFloat3((DirectX::XMFLOAT3*)&r, v);
	return r;
}

void GP::ParticleSystem::Init(uint32_t maxParticles)
{
	// 버퍼 초기화
	m_maxParticle = maxParticles;
	std::vector<Particle> zeroInit(maxParticles);
	m_Pool.Create(L"Particle Pool", maxParticles, sizeof(Particle), zeroInit.data());
	m_AliveList1.Create(L"Alive1", maxParticles, sizeof(uint32_t));
	m_AliveList2.Create(L"Alive2", maxParticles, sizeof(uint32_t));
	std::vector<uint32_t> deadInit(maxParticles);
	for (uint32_t i = 0; i < maxParticles; i++)
	{
		deadInit[i] = i;
	}
	m_DeadList.Create(L"Dead", maxParticles, sizeof(uint32_t), deadInit.data());

	std::vector<uint32_t> counterInit = { 0, maxParticles, 0, 0 }; // alive, dead, realEmit, afterSim
	m_Counters.Create(L"Counters", 4, sizeof(uint32_t), counterInit.data());

	std::vector<uint32_t> argsInit = {
			0, 1, 1, 0, // emit dispatch
			0, 1, 1, 0, // simulate dispatch
			6, 0, 0, 0 // draw: 파티클당 정점 6개 고정, 인스턴스 수는 GPU가 채움
	};
	m_IndirectArgsBuffer.Create(L"IndirectArgsBuffer", 12, sizeof(uint32_t), argsInit.data());

	// 셰이더 컴파일
	auto partVS = CompileShader(L"ParticleVS.hlsl", L"main", L"vs_6_2");
	auto partPS = CompileShader(L"ParticlePS.hlsl", L"main", L"ps_6_2");
	auto particleKickoffCS = CompileShader(L"ParticleKickoffCS.hlsl", L"main", L"cs_6_2");
	auto particleEmitCS = CompileShader(L"ParticleEmitCS.hlsl", L"main", L"cs_6_2");
	auto particleSimulateCS = CompileShader(L"ParticleSimulateCS.hlsl", L"main", L"cs_6_2");
	ASSERT(partVS && partPS && particleKickoffCS && particleEmitCS && particleSimulateCS
		, "셰이더 컴파일 실패 - VS 출력창 확인");

	// 루트 시그 - 컴퓨트용
	m_ComputeRootSig.Reset(7, 0);
	m_ComputeRootSig[0].InitAsConstantBuffer(0); // b0
	m_ComputeRootSig[1].InitAsBufferUAV(0);		// u0
	m_ComputeRootSig[2].InitAsBufferUAV(1);		// u1
	m_ComputeRootSig[3].InitAsBufferUAV(2);		// u2
	m_ComputeRootSig[4].InitAsBufferUAV(3);		// u3
	m_ComputeRootSig[5].InitAsBufferUAV(4);		// u4
	m_ComputeRootSig[6].InitAsBufferUAV(5);		// u5
	m_ComputeRootSig.Finalize(L"ParticleCompute");

	m_KickoffPSO.SetRootSignature(m_ComputeRootSig);
	m_KickoffPSO.SetComputeShader(particleKickoffCS->GetBufferPointer(), particleKickoffCS->GetBufferSize());
	m_KickoffPSO.Finalize();

	m_EmitPSO.SetRootSignature(m_ComputeRootSig);
	m_EmitPSO.SetComputeShader(particleEmitCS->GetBufferPointer(), particleEmitCS->GetBufferSize());
	m_EmitPSO.Finalize();

	m_SimulatePSO.SetRootSignature(m_ComputeRootSig);
	m_SimulatePSO.SetComputeShader(particleSimulateCS->GetBufferPointer(), particleSimulateCS->GetBufferSize());
	m_SimulatePSO.Finalize();

	// 루트 시그 - 드로우용
	// b0 카메라, t0 풀, t1 alive
	m_GraphicsRootSig.Reset(4, 1); // 스태틱 샘플러 1개
	m_GraphicsRootSig[0].InitAsConstantBuffer(0);
	m_GraphicsRootSig[1].InitAsBufferSRV(0);
	m_GraphicsRootSig[2].InitAsBufferSRV(1);
	m_GraphicsRootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
	m_GraphicsRootSig.InitStaticSampler(0, SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);

	m_GraphicsRootSig.Finalize(L"ParticleDraw"); 

	m_DrawPSO.SetRootSignature(m_GraphicsRootSig);
	// 인풋 레이아웃 X - 정점 버퍼 대신 SV_VertexID로 SRV(풀)를 직접 읽음
	m_DrawPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_DrawPSO.SetVertexShader(partVS->GetBufferPointer(), partVS->GetBufferSize());
	m_DrawPSO.SetPixelShader(partPS->GetBufferPointer(), partPS->GetBufferSize());
	m_DrawPSO.SetRasterizerState(RasterizerDefault);
	m_DrawPSO.SetBlendState(BlendAdditive);              // 가산블렌딩
	//m_DrawPSO.SetBlendState(BlendDisable);              // 블렌딩 X
	m_DrawPSO.SetDepthStencilState(DepthStateReadOnly);  // 테스트만, 쓰기 금지
	m_DrawPSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), g_SceneDepthBuffer.GetFormat());
	m_DrawPSO.Finalize();

	// 텍스쳐 로드
	ASSERT(LoadDDSTexture(m_SpriteTex, "Textures/fire.dds"), "dds 로드 실패");
	
}

void GP::ParticleSystem::Update(float dt)
{
	m_Emitter.Update(dt);
	m_FrameParams = m_Emitter.MakeParams(dt);
}

void GP::ParticleSystem::UpdateGPU(ComputeContext& cpt)
{
	BindResources(cpt);
	KickoffPass(cpt);
	EmitPass(cpt);
	SimulatePass(cpt);
	UpdateDrawArgs(cpt);
}

void GP::ParticleSystem::BindResources(ComputeContext& cpt)
{
	// UAV 전환
	cpt.TransitionResource(m_Pool, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(*m_CurrentAlive, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(*m_NewAlive, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(m_DeadList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(m_IndirectArgsBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// 루트 시그 + 버퍼 바인딩
	cpt.SetRootSignature(m_ComputeRootSig);
	cpt.SetBufferUAV(1, m_Pool);				// u0
	cpt.SetBufferUAV(2, *m_CurrentAlive);		// u1
	cpt.SetBufferUAV(3, *m_NewAlive);			// u2
	cpt.SetBufferUAV(4, m_DeadList);			// u3
	cpt.SetBufferUAV(5, m_Counters);			// u4
	cpt.SetBufferUAV(6, m_IndirectArgsBuffer);	// u5

	cpt.SetDynamicConstantBufferView(0, sizeof(m_FrameParams), &m_FrameParams);
}

void GP::ParticleSystem::KickoffPass(ComputeContext& cpt)
{
	cpt.SetPipelineState(m_KickoffPSO);
	cpt.Dispatch(1, 1, 1);
}

void GP::ParticleSystem::EmitPass(ComputeContext& cpt)
{
	// 대기: 킥오프의 카운터 쓰기(COUNTER_REAL) 완료
	cpt.InsertUAVBarrier(m_Counters);
	// 대기 겸 용도 변경: 지시서를 커맨드 프로세서가 읽을 수 있게
	cpt.TransitionResource(m_IndirectArgsBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	cpt.SetPipelineState(m_EmitPSO);
	cpt.DispatchIndirect(m_IndirectArgsBuffer, ARGS_EMIT_DISPATCH_X);
}

void GP::ParticleSystem::SimulatePass(ComputeContext& cpt)
{
	// 대기: emit의 UAV 쓰기
	cpt.InsertUAVBarrier(*m_CurrentAlive);
	cpt.InsertUAVBarrier(m_Pool);
	cpt.InsertUAVBarrier(m_DeadList);
	cpt.InsertUAVBarrier(m_Counters);

	cpt.SetPipelineState(m_SimulatePSO);
	cpt.DispatchIndirect(m_IndirectArgsBuffer, ARGS_SIMULATE_DISPATCH_X);
}

void GP::ParticleSystem::UpdateDrawArgs(ComputeContext& cpt)
{
	// simulation이 확정한 생존자 수를 args 복사
	cpt.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_COPY_SOURCE);
	cpt.CopyBufferRegion(m_IndirectArgsBuffer, ARGS_DRAW_INSTANCE_COUNT, m_Counters, COUNTER_AFTER_SIMULATE, sizeof(uint32_t));
	cpt.TransitionResource(m_IndirectArgsBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	// Draw가 읽을 리소스 SRV 전환
	cpt.TransitionResource(m_Pool, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(*m_NewAlive, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void GP::ParticleSystem::Draw(GraphicsContext& gfx, const Camera& camera)
{
	ParticleDrawCB cb;
	cb.camRight = ToF3(camera.GetRight());
	cb.camUp = ToF3(camera.GetUp());
	cb.particleSize = m_ParticleSize;
	DirectX::XMStoreFloat4x4(
		reinterpret_cast<DirectX::XMFLOAT4X4*>(&cb.viewProj),
		camera.GetViewProj());

	gfx.SetRootSignature(m_GraphicsRootSig);	// 루트 인자보다 먼저
	gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb); // b0
	gfx.SetBufferSRV(1, m_Pool);				// t0
	gfx.SetBufferSRV(2, *m_NewAlive);			// t1
	gfx.SetDynamicDescriptor(3, 0, m_SpriteTex.GetSRV());
	gfx.SetPipelineState(m_DrawPSO);
	gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfx.DrawIndirect(m_IndirectArgsBuffer, ARGS_DRAW_VERTEX_COUNT_PER_INSTANCE);
}

void GP::ParticleSystem::EndFrame()
{
	std::swap(m_CurrentAlive, m_NewAlive);
}
