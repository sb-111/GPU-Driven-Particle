#include "GameCore.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "RenderTypes.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include "CompiledShaders/TriangleVS.h"
#include "CompiledShaders/TrianglePS.h"

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
		// 1. 정점 버퍼 생성 (IA에 공급될 소스)
		Vertex verts[3] =
		{
			{{  0.0f,  1.0f, 0.0f }, { 1,0,0,1 }},
			{{ -1.0f, -1.0f, 0.0f }, { 0,0,1,1 }},
			{{  1.0f, -1.0f, 0.0f }, { 0,1,0,1 }},
		};
		m_VertexBuffer.Create(L"Triangle VB", 3, sizeof(Vertex), verts);

		// 2. 루트 시그니처 생성
		// 지금은 CBV 1개(b0)
		m_RootSig.Reset(1, 0);                  // 루트 파라미터 1개
		m_RootSig[0].InitAsConstantBuffer(0);   // 0번 루트 파라미터에 0번 슬롯 바인딩
		m_RootSig.Finalize(L"triangle", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// 3. Pipeline의 전 단계 설정을 하나로 굳힘
		m_PSO.SetRootSignature(m_RootSig);                                        // 자원 계약 연결
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =                                  // IA: 정점 바이트 해석 규칙
		{
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
		m_PSO.SetInputLayout(_countof(inputLayout), inputLayout);                 // IA (POSITION/COLOR 레이아웃)
		m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);   // IA (삼각형 종류)
		m_PSO.SetVertexShader(g_pTriangleVS, sizeof(g_pTriangleVS));              // VS 
		m_PSO.SetPixelShader(g_pTrianglePS, sizeof(g_pTrianglePS));               // PS 
		m_PSO.SetRasterizerState(RasterizerDefault);                              // RS (컬링/채우기)
		m_PSO.SetBlendState(BlendDisable);                                        // OM (블렌딩 끔)
		m_PSO.SetDepthStencilState(DepthStateDisabled);                           // OM (깊이 테스트 끔 — 지금은 삼각형뿐)
		m_PSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN); // OM (RT 포맷, 깊이 없음)
		m_PSO.Finalize();                                                         // 확정

		// 4. 카메라 투영 설정 (fov/near/far 고정 → 1회. 창 리사이즈 때만 갱신)
		float aspect = (float)g_SceneColorBuffer.GetHeight() / (float)g_SceneColorBuffer.GetWidth(); // ※ 높이/너비
		m_Camera.SetPerspective(3.14159f / 3.0f, aspect, 1.0f, 1000.0f); // 60도
	}

	void Cleanup(void) override {}
	void Update(float deltaT) override { m_CamController.Update(deltaT); }  // 입력 읽어 카메라 포즈 갱신

	// ==============================================================
	//  매 프레임. GPU 명령을 "기록"만 함 (실행은 Finish 이후 GPU가)
	// ==============================================================
	void RenderScene(void) override
	{
		// 1. 명령 리스트 시작 (풀에서 하나 빌려옴)
		GraphicsContext& gfx = GraphicsContext::Begin(L"Clear");

		// 2. 배리어: 씬버퍼 상태 전환 (Present → Render Target)  ※ Clear/그리기보다 먼저
		gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		// 3. 배경 클리어
		float clearColor[4] = { 0.2f, 0.4f, 0.8f, 1.0f };
		gfx.ClearColor(g_SceneColorBuffer, clearColor);

		// 4. 루트 시그니처 → PSO   
		gfx.SetRootSignature(m_RootSig);
		gfx.SetPipelineState(m_PSO);

		// 5. IA
		gfx.SetVertexBuffer(0, m_VertexBuffer.VertexBufferView());      // 정점 소스 (뷰 = 주소 + stride)
		gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  // 삼각형 목록

		// 6. VS 동적 데이터 업로드 (viewProj는 매 프레임 바뀜 → dynamic)
		m_Camera.Update();
		VSConstants cb;
		cb.viewProj = m_Camera.GetViewProj();
		gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb);          // 루트 b0

		// 7. RS
		gfx.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());

		// 8. OM (PS/Blend/Depth 등은 PSO에 이미 있음)
		gfx.SetRenderTarget(g_SceneColorBuffer.GetRTV());

		// 9. Draw 
		gfx.Draw(3);

		// 10. Command List 닫고 GPU 큐에 제출
		gfx.Finish();
	}

private:
	ByteAddressBuffer m_VertexBuffer;   // 정점 데이터
	RootSignature     m_RootSig;        // Root Signature
	GraphicsPSO       m_PSO;            // PSO

	// 초기 카메라 위치 지정, 회전 X
	Camera m_Camera{ Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 5.0f)) };
	CameraController m_CamController{ m_Camera };
};

CREATE_APPLICATION(ParticleApp)
