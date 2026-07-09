#include "GameCore.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "RenderTypes.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include "ShaderCompiler.h"
#include "ParticleSystem.h"

#include "Camera.h"
#include "CameraController.h"

using namespace GameCore;
using namespace Graphics;
using namespace GP;

// 상수버퍼 구조체 cbuffer(b0) 와 1:1 대응
__declspec(align(16)) struct VSConstants
{
	Math::Matrix4 viewProj;
};

class ParticleApp : public IGameApp
{
public:
	// ==============================================================
	// 준비 단계 (앱 시작 시 1회). 이후 거의 안 바뀜.
	// ==============================================================
	void Startup(void) override
	{
		// 0. 씬(바닥) 셰이더 컴파일 - 파티클 셰이더는 ParticleSystem::Init이 담당
		auto triVS = CompileShader(L"Shaders/TriangleVS.hlsl", L"main", L"vs_6_2");
		auto triPS = CompileShader(L"Shaders/TrianglePS.hlsl", L"main", L"ps_6_2");
		ASSERT(triVS && triPS, "셰이더 컴파일 실패 - VS 출력창 확인");

		// 1. 바닥 쿼드 (정점 4개 + 인덱스 6개)
		Vertex floorVerts[4] =
		{
			{{ -20.0f, -2.0f, -20.0f }, { 0.1f, 0.1f, 0.12f, 1.0f }},   // 0: 뒤-왼
			{{  20.0f, -2.0f, -20.0f }, { 0.1f, 0.1f, 0.12f, 1.0f }},   // 1: 뒤-오
			{{  20.0f, -2.0f,  20.0f }, { 0.12f, 0.12f, 0.15f, 1.0f }}, // 2: 앞-오
			{{ -20.0f, -2.0f,  20.0f }, { 0.12f, 0.12f, 0.15f, 1.0f }}, // 3: 앞-왼
		};
		uint16_t floorIndices[6] = { 0, 2, 1,  0, 3, 2 };
		m_FloorVB.Create(L"Floor VB", 4, sizeof(Vertex), floorVerts);
		m_FloorIB.Create(L"Floor IB", 6, sizeof(uint16_t), floorIndices);

		// 2. 씬(바닥) 루트시그 + PSO
		m_SceneRootSig.Reset(3, 0);
		m_SceneRootSig[0].InitAsConstantBuffer(0);   // b0 카메라
		m_SceneRootSig[1].InitAsBufferSRV(0);        // t0
		m_SceneRootSig[2].InitAsBufferSRV(1);        // t1
		m_SceneRootSig.Finalize(L"Scene", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_ScenePSO.SetRootSignature(m_SceneRootSig);
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =
		{
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
		m_ScenePSO.SetInputLayout(_countof(inputLayout), inputLayout);
		m_ScenePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_ScenePSO.SetVertexShader(triVS->GetBufferPointer(), triVS->GetBufferSize());
		m_ScenePSO.SetPixelShader(triPS->GetBufferPointer(), triPS->GetBufferSize());
		m_ScenePSO.SetRasterizerState(RasterizerDefault);
		m_ScenePSO.SetBlendState(BlendDisable);
		m_ScenePSO.SetDepthStencilState(DepthStateReadWrite);   // 불투명: 테스트 + 쓰기
		m_ScenePSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), g_SceneDepthBuffer.GetFormat());
		m_ScenePSO.Finalize();

		// 3. 파티클 시스템
		m_Particles.Init(m_ParticleNum);

		// 4. 카메라 투영 설정 (fov/near/far 고정 → 1회. 창 리사이즈 때만 갱신)
		float aspect = (float)g_SceneColorBuffer.GetHeight() / (float)g_SceneColorBuffer.GetWidth(); // ※ 높이/너비
		m_Camera.SetPerspective(3.14159f / 3.0f, aspect, 1.0f, 1000.0f); // 60도
	}

	void Cleanup(void) override {}

	void Update(float deltaT) override
	{
		m_CamController.Update(deltaT);
		m_Particles.Update(deltaT);
	}

	// ==============================================================
	//  매 프레임. GPU 명령을 "기록"만 함 (실행은 Finish 이후 GPU가)
	// ==============================================================
	void RenderScene(void) override
	{
		GraphicsContext& gfx = GraphicsContext::Begin(L"Clear");

		// =============== 컴퓨트: 파티클 시뮬레이션 ==============
		m_Particles.UpdateGPU(gfx.GetComputeContext());

		// =============== 그래픽스 공통 준비 ==============
		gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true); // RTV
		gfx.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);   // DSV

		float clearColor[4] = { 0.02f, 0.02f, 0.05f, 1.0f };
		gfx.ClearColor(g_SceneColorBuffer, clearColor);
		gfx.ClearDepth(g_SceneDepthBuffer);

		gfx.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
		gfx.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());

		m_Camera.Update();
		VSConstants cb;
		cb.viewProj = m_Camera.GetViewProj();

		// =============== 바닥 ==============
		gfx.SetRootSignature(m_SceneRootSig);   // 루트 인자보다 먼저
		gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb);
		gfx.SetPipelineState(m_ScenePSO);
		gfx.SetVertexBuffer(0, m_FloorVB.VertexBufferView());
		gfx.SetIndexBuffer(m_FloorIB.IndexBufferView());
		gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		gfx.DrawIndexed(6, 0, 0);

		// =============== 파티클 ==============
		m_Particles.Draw(gfx, m_Camera.GetViewProj());

		gfx.Finish();

		m_Particles.EndFrame();   // 핑퐁 스왑
	}

private:
	// 초기 카메라 위치 지정, 회전 X
	Camera m_Camera{ Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 5.0f)) };
	CameraController m_CamController{ m_Camera };

	static const uint32_t m_ParticleNum = 1000000;
	ParticleSystem m_Particles;

	// 씬(바닥)
	RootSignature m_SceneRootSig;
	GraphicsPSO   m_ScenePSO;
	ByteAddressBuffer m_FloorVB;
	ByteAddressBuffer m_FloorIB;
};

CREATE_APPLICATION(ParticleApp)
