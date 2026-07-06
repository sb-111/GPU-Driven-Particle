#include "GameCore.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "RenderTypes.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include "ShaderCompiler.h"
#include "ParticleShared.h"

#include "Camera.h"
#include "CameraController.h"

#include <random> // 파티클 임의 생성용

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
		// 0. 셰이더 런타임 컴파일 (DXC). 실패하면 출력창에 에러
		//	  Blob은 PSO Finalize 시점까지만 살아있으면 됨.
		auto triVS  = CompileShader(L"Shaders/TriangleVS.hlsl", L"main", L"vs_6_2");
		auto triPS  = CompileShader(L"Shaders/TrianglePS.hlsl", L"main", L"ps_6_2");
		auto partVS = CompileShader(L"ParticleVS.hlsl",         L"main", L"vs_6_2");
		auto partPS = CompileShader(L"ParticlePS.hlsl",         L"main", L"ps_6_2");
		auto partCS = CompileShader(L"ParticleSpawnCS.hlsl",    L"main", L"cs_6_2");
		ASSERT(triVS && triPS && partVS && partPS && partCS, "셰이더 컴파일 실패 - VS 출력창 확인");

		// 1. 정점 버퍼 생성 (IA에 공급될 소스)
		Vertex verts[3] =
		{
			{{  0.0f,  1.0f, 0.0f }, { 1,0,0,1 }},
			{{ -1.0f, -1.0f, 0.0f }, { 0,0,1,1 }},
			{{  1.0f, -1.0f, 0.0f }, { 0,1,0,1 }},
		};
		m_VertexBuffer.Create(L"Triangle VB", 3, sizeof(Vertex), verts);

		// ========= Particle Test ===========
		std::vector<Particle> particles = MakeRandomParticles(m_ParticleNum);
		m_ParticleStructuredBuffer.Create(L"ParticleBuffer", m_ParticleNum, sizeof(Particle), particles.data());

		// 2. 루트 시그니처 생성
		m_RootSig.Reset(2, 0);                  // 루트 파라미터 개수
		m_RootSig[0].InitAsConstantBuffer(0);   // 0번 -> b0
		m_RootSig[1].InitAsBufferSRV(0);		// 1번 -> t0
		m_RootSig.Finalize(L"triangle", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// Compute 용
		m_ComputeRootSig.Reset(2, 0);
		m_ComputeRootSig[0].InitAsBufferUAV(0); // u0
		m_ComputeRootSig[1].InitAsConstantBuffer(0); // b0
		m_ComputeRootSig.Finalize(L"ParticleCompute");

		// 파티클 pso
		m_ParticlePSO.SetRootSignature(m_RootSig);
		m_ParticlePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		m_ParticlePSO.SetVertexShader(partVS->GetBufferPointer(), partVS->GetBufferSize());
		m_ParticlePSO.SetPixelShader(partPS->GetBufferPointer(), partPS->GetBufferSize());
		m_ParticlePSO.SetRasterizerState(RasterizerDefault);
		m_ParticlePSO.SetBlendState(BlendDisable);
		m_ParticlePSO.SetDepthStencilState(DepthStateDisabled);
		m_ParticlePSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
		m_ParticlePSO.Finalize();

		// 파티클 컴퓨트 PSO
		m_ParticleComputePSO.SetRootSignature(m_ComputeRootSig);
		m_ParticleComputePSO.SetComputeShader(partCS->GetBufferPointer(), partCS->GetBufferSize());
		m_ParticleComputePSO.Finalize();

		// 3. Pipeline의 전 단계 설정을 하나로 굳힘
		m_PSO.SetRootSignature(m_RootSig);                                        // 자원 계약 연결
		D3D12_INPUT_ELEMENT_DESC inputLayout[] =                                  // IA: 정점 바이트 해석 규칙
		{
			{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};
		m_PSO.SetInputLayout(_countof(inputLayout), inputLayout);                 // IA (POSITION/COLOR 레이아웃)
		m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);   // IA (삼각형 종류)
		m_PSO.SetVertexShader(triVS->GetBufferPointer(), triVS->GetBufferSize()); // VS
		m_PSO.SetPixelShader(triPS->GetBufferPointer(), triPS->GetBufferSize());  // PS
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
	void Update(float deltaT) override { m_DeltaTime = deltaT; m_CamController.Update(deltaT); }  // 입력 읽어 카메라 포즈 갱신

	// ==============================================================
	//  매 프레임. GPU 명령을 "기록"만 함 (실행은 Finish 이후 GPU가)
	// ==============================================================
	void RenderScene(void) override
	{
		// 1. 명령 리스트 시작 (풀에서 하나 빌려옴)
		GraphicsContext& gfx = GraphicsContext::Begin(L"Clear");
		// 컴퓨트 패스
		ComputeContext& cpt = gfx.GetComputeContext(); // 컴퓨트 뷰
		gfx.TransitionResource(m_ParticleStructuredBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS); // UAV 전환
		cpt.SetRootSignature(m_ComputeRootSig);
		cpt.SetPipelineState(m_ParticleComputePSO);
		cpt.SetBufferUAV(0, m_ParticleStructuredBuffer);
		ComputeCB ccb;
		ccb.deltaTime = m_DeltaTime;
		ccb.particleCount = m_ParticleNum;
		cpt.SetDynamicConstantBufferView(1, sizeof(ccb), &ccb);
		cpt.Dispatch((m_ParticleNum + 63) / 64, 1, 1);
		gfx.TransitionResource(m_ParticleStructuredBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); // SRV 전환

		// =================== 공통 ==================
		// 2. 배리어: 씬버퍼 상태 전환 (Present → Render Target)  ※ Clear/그리기보다 먼저
		gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
		// 3. 배경 클리어
		float clearColor[4] = { 0.2f, 0.4f, 0.8f, 1.0f };
		gfx.ClearColor(g_SceneColorBuffer, clearColor);
		// 루트 시그니처 공통 (주의: View 꽂기전에 먼저 바인딩 되어있어야 함)
		gfx.SetRootSignature(m_RootSig);
		// RS 공통
		gfx.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
		// OM 공통 (PS/Blend/Depth 등은 PSO에 이미 있음)
		gfx.SetRenderTarget(g_SceneColorBuffer.GetRTV());

		// 카메라 업데이트 -> 업로드
		m_Camera.Update();
		VSConstants cb;
		cb.viewProj = m_Camera.GetViewProj();
		gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb); // 루트 파라미터 인덱스 0

		// 패스 A
		gfx.SetPipelineState(m_PSO);
		gfx.SetVertexBuffer(0, m_VertexBuffer.VertexBufferView());      // 정점 소스 (뷰 = 주소 + stride)
		gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);  // 삼각형 목록
		gfx.Draw(3);

		// 패스 B
		gfx.SetPipelineState(m_ParticlePSO);
		gfx.SetBufferSRV(1, m_ParticleStructuredBuffer);
		gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		gfx.Draw(m_ParticleNum);

		// Command List 닫고 GPU 큐에 제출
		gfx.Finish();
	}

private:
	ByteAddressBuffer m_VertexBuffer;   // 정점 데이터
	RootSignature     m_RootSig;        // Root Signature
	RootSignature	  m_ComputeRootSig;
	GraphicsPSO       m_PSO;            // PSO
	GraphicsPSO		  m_ParticlePSO;
	ComputePSO		  m_ParticleComputePSO;

	// 초기 카메라 위치 지정, 회전 X
	Camera m_Camera{ Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 5.0f)) };
	CameraController m_CamController{ m_Camera };

	static const int m_ParticleNum = 100000;
	StructuredBuffer m_ParticleStructuredBuffer;
	float m_DeltaTime;
	std::vector<Particle> MakeRandomParticles(int n)
	{
		std::vector<Particle> particles(n);
		std::mt19937 rng(12345);
		std::uniform_real_distribution<float> dist(-2.0f, 2.0f);
		for (int i = 0; i < n; i++)
		{
			particles[i].position.x = dist(rng); 
			particles[i].position.y = dist(rng); 
			particles[i].position.z = dist(rng);
			particles[i].velocity = { dist(rng), dist(rng), dist(rng) };
		}

		return particles;
	}
};

CREATE_APPLICATION(ParticleApp)
