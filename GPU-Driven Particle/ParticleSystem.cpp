#include "ParticleSystem.h"
#include "GameCore.h"
#include "GraphicsCommon.h"
#include "BufferManager.h"   // g_SceneColorBuffer / g_SceneDepthBuffer
#include "CommandContext.h"
#include "CubeMesh.h"

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
	auto meshParticleVS = CompileShader(L"MeshParticleVS.hlsl", L"main", L"vs_6_2");
	auto meshParticlePS = CompileShader(L"MeshParticlePS.hlsl", L"main", L"ps_6_2");
	auto ribbonParticleVS = CompileShader(L"RibbonParticleVS.hlsl", L"main", L"vs_6_2");
	auto ribbonParticlePS = CompileShader(L"RibbonParticlePS.hlsl", L"main", L"ps_6_2");
	auto particleKickoffCS = CompileShader(L"ParticleKickoffCS.hlsl", L"main", L"cs_6_2");
	auto particleEmitCS = CompileShader(L"ParticleEmitCS.hlsl", L"main", L"cs_6_2");
	auto particleSimulateCS = CompileShader(L"ParticleSimulateCS.hlsl", L"main", L"cs_6_2");
	auto screenQuadVS = CompileShader(L"ScreenQuadVS.hlsl", L"main", L"vs_6_2");
	auto compositePS = CompileShader(L"CompositePS.hlsl", L"main", L"ps_6_2");
	ASSERT(partVS && partPS && meshParticleVS && meshParticlePS && ribbonParticleVS && ribbonParticlePS &&
		particleKickoffCS && particleEmitCS && particleSimulateCS && 
		screenQuadVS && compositePS
		, "셰이더 컴파일 실패 - VS 출력창 확인");

	m_Shared.sorter.Init();

	m_Shared.meshVertexBuffer.Create(L"Cube Vertex Buffer", 24, sizeof(MeshVertex), kCubeVertices);
	m_Shared.meshIndexBuffer.Create(L"Cube Index Buffer", 36, sizeof(uint16_t), kCubeIndices);

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
	m_Shared.graphicsRootSig.Reset(7, 1); // 스태틱 샘플러 1개
	m_Shared.graphicsRootSig[0].InitAsConstantBuffer(0); // ViewCB (b0)
	m_Shared.graphicsRootSig[1].InitAsConstantBuffer(1); // FrameCB (b1)
	m_Shared.graphicsRootSig[2].InitAsConstantBuffer(2); // DrawCB (b2)
	m_Shared.graphicsRootSig[3].InitAsBufferSRV(0);		 // pool (t0) 
	m_Shared.graphicsRootSig[4].InitAsBufferSRV(1);		 // alive (t1)
	m_Shared.graphicsRootSig[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL); // 텍스처 (t2)
	m_Shared.graphicsRootSig[6].InitAsBufferSRV(3);		 // Counters (t3)
	m_Shared.graphicsRootSig.InitStaticSampler(0, SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_Shared.graphicsRootSig.Finalize(L"ParticleDraw", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT); 

	// 파티클 누적용 : 해상도 무관
	// new.rgb = src.rgb x 1 + dest.rgb x (1-src.a) : pre-multiplied
	// new.a = src.a x 0 + dest.a x (1-src.a)
	// * 메인 RT는 R11G11B10 -> A설정 무시, 하프 RT(RGBA16F)에서만 쌓임
	D3D12_BLEND_DESC blendParticle = BlendPreMultiplied;
	blendParticle.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;

	// 합성용
	// new.rgb = src.rgb x 1 + dest.rgb x src.a
	D3D12_BLEND_DESC blendComposite = BlendAdditive;
	blendComposite.RenderTarget[0].DestBlend = D3D12_BLEND_SRC_ALPHA;

	static const D3D12_INPUT_ELEMENT_DESC meshLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	/*
	* 렌더러별 PSO 설정을 위한 구조체
	*/
	struct RendererPSODesc
	{
		IDxcBlob* vs;
		IDxcBlob* ps;
		const D3D12_INPUT_ELEMENT_DESC* layout; // 메시 렌더러만 사용
		UINT layoutCount;
		const D3D12_RASTERIZER_DESC* rasterizer;
	};
	const RendererPSODesc rendererDescs[(int)EParticleRenderer::Count] =
	{
		{ partVS.Get(),          partPS.Get(),          nullptr,    0, &RasterizerDefault  },
		{ meshParticleVS.Get(),  meshParticlePS.Get(),  meshLayout, 3, &RasterizerDefault  },
		{ ribbonParticleVS.Get(),ribbonParticlePS.Get(),nullptr,    0, &RasterizerTwoSided },
	};

	// 풀/하프 PSO 변형용
	const DXGI_FORMAT rtFormats[2] = { g_SceneColorBuffer.GetFormat(), g_SceneColorHalfBuffer.GetFormat() };
	const DXGI_FORMAT dsvFormats[2] = { g_SceneDepthBuffer.GetFormat(), g_SceneDepthHalfBuffer.GetFormat() };

	// 렌더러 x 해상도 조합별 PSO 초기화
	for (int r = 0; r < (int)EParticleRenderer::Count; ++r)
	{
		const RendererPSODesc& desc = rendererDescs[r];
		for (int res = 0; res < 2; ++res)
		{
			GraphicsPSO& pso = m_Shared.drawPSO[r][res];
			pso.SetRootSignature(m_Shared.graphicsRootSig);
			pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
			pso.SetInputLayout(desc.layoutCount, desc.layout);
			pso.SetVertexShader(desc.vs->GetBufferPointer(), desc.vs->GetBufferSize());
			pso.SetPixelShader(desc.ps->GetBufferPointer(), desc.ps->GetBufferSize());
			pso.SetRasterizerState(*desc.rasterizer);
			pso.SetBlendState(blendParticle);
			pso.SetDepthStencilState(DepthStateReadOnly); // 테스트만, 쓰기 금지
			pso.SetRenderTargetFormat(rtFormats[res], dsvFormats[res]);
			pso.Finalize();
		}
	}

	// 합성 패스 PSO : Depth Test off (풀스크린 삼각형의 z는 의미가 없음)
	m_Shared.compositePSO.SetRootSignature(m_Shared.graphicsRootSig);
	m_Shared.compositePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_Shared.compositePSO.SetInputLayout(0, nullptr);
	m_Shared.compositePSO.SetVertexShader(screenQuadVS->GetBufferPointer(), screenQuadVS->GetBufferSize());
	m_Shared.compositePSO.SetPixelShader(compositePS->GetBufferPointer(), compositePS->GetBufferSize());
	m_Shared.compositePSO.SetRasterizerState(RasterizerDefault);
	m_Shared.compositePSO.SetBlendState(blendComposite);
	m_Shared.compositePSO.SetDepthStencilState(DepthStateDisabled);
	m_Shared.compositePSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
	m_Shared.compositePSO.Finalize();

	// 텍스쳐 로드 (ETexture enum 순서와 일치)
	static const char* kTexturePaths[(int)ETexture::Count] =
		{ "Textures/fire.dds", "Textures/smoke.dds", "Textures/sparkTex.dds",
		  "SpriteAtlasTextures/boom3.dds", "SpriteAtlasTextures/exp2_0.dds" };
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

void GP::ParticleSystem::Render(GraphicsContext& gfx, const ParticleViewCB& viewCB)
{
	if (m_HalfResolution)
	{
		gfx.TransitionResource(g_SceneColorHalfBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		gfx.TransitionResource(g_SceneDepthHalfBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

		gfx.ClearColor(g_SceneColorHalfBuffer);
		gfx.ClearDepth(g_SceneDepthHalfBuffer);

		gfx.SetViewportAndScissor(0, 0, g_SceneColorHalfBuffer.GetWidth(), g_SceneColorHalfBuffer.GetHeight());
		gfx.SetRenderTarget(g_SceneColorHalfBuffer.GetRTV(), g_SceneDepthHalfBuffer.GetDSV());

		DrawEmitters(gfx, viewCB, true);
		// 합성 패스
		CompositeToMain(gfx);
	}
	else
	{
		gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
		gfx.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

		gfx.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
		gfx.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());

		DrawEmitters(gfx, viewCB, false);
	}
}

void GP::ParticleSystem::DrawEmitters(GraphicsContext& gfx, const ParticleViewCB& viewCB, bool halfResolution)
{
	// Draw 중에는 graphicsRootSig 유지
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
		e->Draw(gfx, halfResolution);
}

void GP::ParticleSystem::CompositeToMain(GraphicsContext& gfx)
{
	ScopedTimer _prof(L"Particle Composite", gfx);
	gfx.TransitionResource(g_SceneColorHalfBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

	gfx.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
	gfx.SetRenderTarget(g_SceneColorBuffer.GetRTV());
	gfx.SetRootSignature(m_Shared.graphicsRootSig);
	gfx.SetPipelineState(m_Shared.compositePSO);
	gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfx.SetDynamicDescriptor(5, 0, g_SceneColorHalfBuffer.GetSRV());

	gfx.Draw(3, 0);
}
void GP::ParticleSystem::EndFrame()
{
	for (auto& e : m_Emitters)
		e->EndFrame();
}
