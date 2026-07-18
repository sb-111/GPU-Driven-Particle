#include "ParticleSystem.h"
#include "GameCore.h"
#include "GraphicsCommon.h"
#include "BufferManager.h"   // g_SceneColorBuffer / g_SceneDepthBuffer
#include "CommandContext.h"

using namespace GameCore;
using namespace Graphics;
using namespace DirectX;

void GP::ParticleSystem::Init(uint32_t maxParticlesPerEmitter)
{
	m_maxParticle = maxParticlesPerEmitter;
	InitSharedResources();

	// 기본 Emitter 하나로 시작
	AddEmitter(Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 0.0f)));
}

void GP::ParticleSystem::InitSharedResources()
{
	// 셰이더 컴파일
	auto partVS = CompileShader(L"ParticleVS.hlsl", L"main", L"vs_6_2");
	auto partPS = CompileShader(L"ParticlePS.hlsl", L"main", L"ps_6_2");
	auto particleKickoffCS = CompileShader(L"ParticleKickoffCS.hlsl", L"main", L"cs_6_2");
	auto particleEmitCS = CompileShader(L"ParticleEmitCS.hlsl", L"main", L"cs_6_2");
	auto particleSimulateCS = CompileShader(L"ParticleSimulateCS.hlsl", L"main", L"cs_6_2");
	ASSERT(partVS && partPS && particleKickoffCS && particleEmitCS && particleSimulateCS
		, "셰이더 컴파일 실패 - VS 출력창 확인");

	m_Shared.sorter.Init();

	// 루트 시그 - 컴퓨트용
	m_Shared.computeRootSig.Reset(9, 0);
	m_Shared.computeRootSig[0].InitAsConstantBuffer(0); // b0 (ParticleFrameCB)
	m_Shared.computeRootSig[1].InitAsBufferUAV(0);		// u0
	m_Shared.computeRootSig[2].InitAsBufferUAV(1);		// u1
	m_Shared.computeRootSig[3].InitAsBufferUAV(2);		// u2
	m_Shared.computeRootSig[4].InitAsBufferUAV(3);		// u3
	m_Shared.computeRootSig[5].InitAsBufferUAV(4);		// u4
	m_Shared.computeRootSig[6].InitAsBufferUAV(5);		// u5
	m_Shared.computeRootSig[7].InitAsConstantBuffer(1); // b1 (ParticleViewCB)
	m_Shared.computeRootSig[8].InitAsBufferUAV(6); // u6
	m_Shared.computeRootSig.Finalize(L"ParticleCompute");

	m_Shared.kickoffPSO.SetRootSignature(m_Shared.computeRootSig);
	m_Shared.kickoffPSO.SetComputeShader(particleKickoffCS->GetBufferPointer(), particleKickoffCS->GetBufferSize());
	m_Shared.kickoffPSO.Finalize();

	m_Shared.emitPSO.SetRootSignature(m_Shared.computeRootSig);
	m_Shared.emitPSO.SetComputeShader(particleEmitCS->GetBufferPointer(), particleEmitCS->GetBufferSize());
	m_Shared.emitPSO.Finalize();

	m_Shared.simulatePSO.SetRootSignature(m_Shared.computeRootSig);
	m_Shared.simulatePSO.SetComputeShader(particleSimulateCS->GetBufferPointer(), particleSimulateCS->GetBufferSize());
	m_Shared.simulatePSO.Finalize();

	// 루트 시그 - 드로우용
	m_Shared.graphicsRootSig.Reset(6, 1); // 스태틱 샘플러 1개
	m_Shared.graphicsRootSig[0].InitAsConstantBuffer(0); // ViewCB (b0)
	m_Shared.graphicsRootSig[1].InitAsConstantBuffer(1); // FrameCB (b1)
	m_Shared.graphicsRootSig[2].InitAsConstantBuffer(2); // DrawCB (b2)
	m_Shared.graphicsRootSig[3].InitAsBufferSRV(0);		 // pool (t0) 
	m_Shared.graphicsRootSig[4].InitAsBufferSRV(1);		 // alive (t1)
	m_Shared.graphicsRootSig[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL); // 텍스처 (t2)
	m_Shared.graphicsRootSig.InitStaticSampler(0, SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_Shared.graphicsRootSig.Finalize(L"ParticleDraw", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT); 

	m_Shared.drawAdditivePSO.SetRootSignature(m_Shared.graphicsRootSig);
	// 인풋 레이아웃 X - 정점 버퍼 대신 SV_VertexID로 SRV(풀)를 직접 읽음
	m_Shared.drawAdditivePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_Shared.drawAdditivePSO.SetVertexShader(partVS->GetBufferPointer(), partVS->GetBufferSize());
	m_Shared.drawAdditivePSO.SetPixelShader(partPS->GetBufferPointer(), partPS->GetBufferSize());
	m_Shared.drawAdditivePSO.SetRasterizerState(RasterizerDefault);
	m_Shared.drawAdditivePSO.SetBlendState(BlendAdditive);              // 가산블렌딩
	m_Shared.drawAdditivePSO.SetDepthStencilState(DepthStateReadOnly);  // 테스트만, 쓰기 금지
	m_Shared.drawAdditivePSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), g_SceneDepthBuffer.GetFormat());
	m_Shared.drawAdditivePSO.Finalize();

	m_Shared.drawAlphaPSO = m_Shared.drawAdditivePSO;
	m_Shared.drawAlphaPSO.SetBlendState(BlendTraditional);
	m_Shared.drawAlphaPSO.Finalize();

	// 텍스쳐 로드 (ETexture enum 순서와 일치)
	static const char* kTexturePaths[(int)ETexture::Count] =
		{ "Textures/fire.dds", "Textures/smoke.dds", "Textures/sparkTex.dds" };
	for (int i = 0; i < (int)ETexture::Count; ++i)
		ASSERT(LoadDDSTexture(m_Shared.spriteTextures[i], kTexturePaths[i]), "dds 로드 실패");
}

void GP::ParticleSystem::AddEmitter(const Math::OrthogonalTransform& transform)
{
	auto emitter = std::make_unique<ParticleEmitter>(transform);
	emitter->Init(m_maxParticle, &m_Shared, (uint32_t)m_Emitters.size());
	m_Emitters.push_back(std::move(emitter));
}

void GP::ParticleSystem::Update(float dt)
{
	for (auto& e : m_Emitters)
		e->Update(dt);
}

void GP::ParticleSystem::UpdateGPU(ComputeContext& cpt, const ParticleViewCB& viewParams)
{
	// 모든 Emitter에 대해 Pass 실행
	for (auto& e : m_Emitters)
	{
		e->BindResources(cpt, viewParams);
		e->KickoffPass(cpt);
		e->EmitPass(cpt);
		e->SimulatePass(cpt);
		e->SortPass(cpt);
		e->UpdateDrawArgs(cpt);
	}
}

void GP::ParticleSystem::Draw(GraphicsContext& gfx, const ParticleViewCB& viewCB)
{
	// Draw 중에는 graphicsRootSig 안바뀜
	gfx.SetRootSignature(m_Shared.graphicsRootSig);               
	gfx.SetDynamicConstantBufferView(0, sizeof(viewCB), &viewCB); // b0

	// Emitter 거리별로 정렬 (앞에 있는 Emitter가 올라와야 자연스러움)
	// 거리 먼 순서대로 정렬되는 게 목표 (back to front)
	Math::Vector3 camPos(viewCB.camPos.x, viewCB.camPos.y, viewCB.camPos.z);
	std::vector<ParticleEmitter*> sortedEmitters;
	for (const auto& e : m_Emitters)
	{
		sortedEmitters.push_back(e.get());
	}
	std::sort(sortedEmitters.begin(), sortedEmitters.end(),
		[&](ParticleEmitter* a, ParticleEmitter* b) {
			float distanceA = Math::LengthSquare(a->GetPosition() - camPos);
			float distanceB = Math::LengthSquare(b->GetPosition() - camPos);
			return distanceA > distanceB;
		});

	for (auto& e : sortedEmitters)
		e->Draw(gfx);
}

void GP::ParticleSystem::EndFrame()
{
	for (auto& e : m_Emitters)
		e->EndFrame();
}
