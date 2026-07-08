#include "GameCore.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "RenderTypes.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include "ShaderCompiler.h"
#include "ParticleShared.h"
#include "ParticleEmitter.h"

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
		// 0. 셰이더 런타임 컴파일 (DXC). 실패하면 출력창에 에러
		//	  Blob은 PSO Finalize 시점까지만 살아있으면 됨.
		auto triVS  = CompileShader(L"Shaders/TriangleVS.hlsl", L"main", L"vs_6_2");
		auto triPS  = CompileShader(L"Shaders/TrianglePS.hlsl", L"main", L"ps_6_2");
		auto partVS = CompileShader(L"ParticleVS.hlsl",         L"main", L"vs_6_2");
		auto partPS = CompileShader(L"ParticlePS.hlsl",         L"main", L"ps_6_2");
		auto particleKickoffCS = CompileShader(L"ParticleKickoffCS.hlsl", L"main", L"cs_6_2");
		auto particleEmitCS = CompileShader(L"ParticleEmitCS.hlsl", L"main", L"cs_6_2");
		auto particleSimulateCS = CompileShader(L"ParticleSimulateCS.hlsl", L"main", L"cs_6_2");
		ASSERT(triVS && triPS && partVS && partPS && particleKickoffCS && particleEmitCS && particleSimulateCS
			, "셰이더 컴파일 실패 - VS 출력창 확인");

		// 1. 정점 버퍼 생성 (IA에 공급될 소스)
		Vertex verts[3] =
		{
			{{  0.0f,  1.0f, 0.0f }, { 1,0,0,1 }},
			{{ -1.0f, -1.0f, 0.0f }, { 0,0,1,1 }},
			{{  1.0f, -1.0f, 0.0f }, { 0,1,0,1 }},
		};
		m_VertexBuffer.Create(L"Triangle VB", 3, sizeof(Vertex), verts);
		// ================ Indirect에 사용할 버퍼 ================
		std::vector<uint32_t> argsInit = {
			0, 1, 1, 0, // emit dispatch
			0, 1, 1, 0, // simulate dispatch
			0, 1, 0, 0 // draw
		};
		// Emit dispatch(3) + pad + Simul dispatch(3) + pad + draw(4)
		m_indirectArgsBuffer.Create(L"IndirectArgsBuffer", 12, sizeof(uint32_t), argsInit.data());
		// ================ Indirect에 사용할 버퍼 ================
		
		// ================ CS에서 사용할 버퍼 생성 ================
		m_Alive1List.Create(L"Alive1", m_ParticleNum, sizeof(uint32_t));
		m_Alive2List.Create(L"Alive2", m_ParticleNum, sizeof(uint32_t));
		std::vector<uint32_t> deadInit(m_ParticleNum);
		for (int i = 0; i < m_ParticleNum; i++)
		{
			deadInit[i] = i;
		}
		m_DeadList.Create(L"Dead", m_ParticleNum, sizeof(uint32_t), deadInit.data());

		std::vector<uint32_t> counterInit = { 0, m_ParticleNum, 0, 0 }; // alive, dead, realEmit, afterSim
		m_Counters.Create(L"Counters", 4, sizeof(uint32_t), counterInit.data());

		std::vector<Particle> zeroInit(m_ParticleNum);
		m_ParticleStructuredBuffer.Create(L"ParticleBuffer", m_ParticleNum, sizeof(Particle), zeroInit.data());
		// ================ CS에서 사용할 버퍼 생성 ================

		// ================== Particle Compute RootSignature & PSO ==================
		m_ComputeRootSig.Reset(7, 0);
		m_ComputeRootSig[0].InitAsConstantBuffer(0); // b0
		m_ComputeRootSig[1].InitAsBufferUAV(0);		// u0
		m_ComputeRootSig[2].InitAsBufferUAV(1);		// u1
		m_ComputeRootSig[3].InitAsBufferUAV(2);		// u2
		m_ComputeRootSig[4].InitAsBufferUAV(3);		// u3
		m_ComputeRootSig[5].InitAsBufferUAV(4);		// u4
		m_ComputeRootSig[6].InitAsBufferUAV(5);		// u5
		m_ComputeRootSig.Finalize(L"ParticleCompute");

		m_ParticleKickoffPSO.SetRootSignature(m_ComputeRootSig);
		m_ParticleKickoffPSO.SetComputeShader(particleKickoffCS->GetBufferPointer(), particleKickoffCS->GetBufferSize());
		m_ParticleKickoffPSO.Finalize();

		m_ParticleEmitPSO.SetRootSignature(m_ComputeRootSig);
		m_ParticleEmitPSO.SetComputeShader(particleEmitCS->GetBufferPointer(), particleEmitCS->GetBufferSize());
		m_ParticleEmitPSO.Finalize();

		m_ParticleSimulatePSO.SetRootSignature(m_ComputeRootSig);
		m_ParticleSimulatePSO.SetComputeShader(particleSimulateCS->GetBufferPointer(), particleSimulateCS->GetBufferSize());
		m_ParticleSimulatePSO.Finalize();
		// ================== Particle Compute RootSignature & PSO ==================

		// ================= Particle Graphics RootSignature & PSO =================
		m_RootSig.Reset(3, 0);                  // 루트 파라미터 개수
		m_RootSig[0].InitAsConstantBuffer(0);   // 0번 -> b0
		m_RootSig[1].InitAsBufferSRV(0);		// 1번 -> t0
		m_RootSig[2].InitAsBufferSRV(1);		// t1
		m_RootSig.Finalize(L"triangle", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_ParticlePSO.SetRootSignature(m_RootSig);
		m_ParticlePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
		m_ParticlePSO.SetVertexShader(partVS->GetBufferPointer(), partVS->GetBufferSize());
		m_ParticlePSO.SetPixelShader(partPS->GetBufferPointer(), partPS->GetBufferSize());
		m_ParticlePSO.SetRasterizerState(RasterizerDefault);
		m_ParticlePSO.SetBlendState(BlendDisable);
		m_ParticlePSO.SetDepthStencilState(DepthStateDisabled);
		m_ParticlePSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
		m_ParticlePSO.Finalize();
		// ================= Particle Graphics RootSignature & PSO =================


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
	void Update(float deltaT) override
	{
		m_DeltaTime = deltaT;
		m_CamController.Update(deltaT);
		m_ParticleEmitter.Update(deltaT);

	}  // 입력 읽어 카메라 포즈 갱신

	// ==============================================================
	//  매 프레임. GPU 명령을 "기록"만 함 (실행은 Finish 이후 GPU가)
	// ==============================================================
	void RenderScene(void) override
	{
		// 1. 명령 리스트 시작 (풀에서 하나 빌려옴)
		GraphicsContext& gfx = GraphicsContext::Begin(L"Clear");
		// =============== Compute 구간 ==============
		ComputeContext& cpt = gfx.GetComputeContext(); // 컴퓨트 뷰
		// UAV 전환
		gfx.TransitionResource(m_ParticleStructuredBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS); // UAV 전환
		gfx.TransitionResource(*m_CurrentAlive, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		gfx.TransitionResource(*m_newAlive, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		gfx.TransitionResource(m_DeadList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		gfx.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		gfx.TransitionResource(m_indirectArgsBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// 루트 시그 + 버퍼 바인딩
		cpt.SetRootSignature(m_ComputeRootSig);
		cpt.SetBufferUAV(1, m_ParticleStructuredBuffer);	// u0
		cpt.SetBufferUAV(2, *m_CurrentAlive);				// u1
		cpt.SetBufferUAV(3, *m_newAlive);					// u2
		cpt.SetBufferUAV(4, m_DeadList);					// u3
		cpt.SetBufferUAV(5, m_Counters);					// u4
		cpt.SetBufferUAV(6, m_indirectArgsBuffer);			// u5


		ParticleFrameCB particleFrameParams = m_ParticleEmitter.MakeParams(m_DeltaTime);
		cpt.SetDynamicConstantBufferView(0, sizeof(particleFrameParams), &particleFrameParams);

		// ============ Kickoff Pass =============
		cpt.SetPipelineState(m_ParticleKickoffPSO);
		cpt.Dispatch(1, 1, 1);
		// ============ Kickoff Pass =============

		// Counters 완료되도록 대기
		cpt.InsertUAVBarrier(m_Counters);
		// 읽는 주체가 셰이더가 아닌 Command 프로세서임. 
		gfx.TransitionResource(m_indirectArgsBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

		// ============ Emit Pass =============
		cpt.SetPipelineState(m_ParticleEmitPSO);
		//cpt.Dispatch((particleFrameParams.emitCount + 63) / 64, 1, 1); // 첫번째 인자 = 그룹 수
		cpt.DispatchIndirect(m_indirectArgsBuffer, ARGS_EMIT_DISPATCH_X);
		// ============ Emit Pass =============

		// 앞선 UAV 쓰기 모두 끝난 후 다음 명령이 이 리소스 건드리도록
		cpt.InsertUAVBarrier(*m_CurrentAlive);
		cpt.InsertUAVBarrier(m_ParticleStructuredBuffer);
		cpt.InsertUAVBarrier(m_DeadList);
		cpt.InsertUAVBarrier(m_Counters);

		// ============ Simulation Pass =============
		cpt.SetPipelineState(m_ParticleSimulatePSO);
		cpt.DispatchIndirect(m_indirectArgsBuffer, ARGS_SIMULATE_DISPATCH_X);
		// ============ Simulation Pass =============

		// args 버퍼 자리에 simul 끝난 후 개수 복사 -> Draw에 알려주기 위함
		gfx.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_COPY_SOURCE); // 카피용
		cpt.CopyBufferRegion(m_indirectArgsBuffer, ARGS_DRAW_VERTEX_COUNT_PER_INSTANCE, m_Counters, COUNTER_AFTER_SIMULATE, sizeof(uint32_t));
		gfx.TransitionResource(m_indirectArgsBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT); // args용(읽기 전용)

		gfx.TransitionResource(m_ParticleStructuredBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE); // SRV 전환
		gfx.TransitionResource(*m_CurrentAlive, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		gfx.TransitionResource(*m_newAlive, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		gfx.TransitionResource(m_DeadList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		gfx.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);


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
		gfx.SetBufferSRV(1, m_ParticleStructuredBuffer); // t0
		gfx.SetBufferSRV(2, *m_newAlive);				 // t1
		gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		gfx.DrawIndirect(m_indirectArgsBuffer, ARGS_DRAW_VERTEX_COUNT_PER_INSTANCE);

		// Command List 닫고 GPU 큐에 제출
		gfx.Finish();

		// 핑퐁
		std::swap(m_CurrentAlive, m_newAlive);
	}

private:
	ByteAddressBuffer m_VertexBuffer;   // 정점 데이터
	RootSignature     m_RootSig;        // Root Signature
	RootSignature	  m_ComputeRootSig;
	GraphicsPSO       m_PSO;            // PSO
	GraphicsPSO		  m_ParticlePSO;

	// 초기 카메라 위치 지정, 회전 X
	Camera m_Camera{ Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 5.0f)) };
	CameraController m_CamController{ m_Camera };

	static const int m_ParticleNum = 1000000;
	StructuredBuffer m_ParticleStructuredBuffer;
	float m_DeltaTime;


	ParticleEmitter m_ParticleEmitter{
	Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 0.0f)),  // 원점, 회전 없음(Y=위로 분출)
	5000.0f,   // spawnRate: 초당 5000개
	2.0f,      // minLifeTime
	4.0f       // maxLifeTime
	};
	ComputePSO m_ParticleKickoffPSO;
	ComputePSO m_ParticleEmitPSO;
	ComputePSO m_ParticleSimulatePSO;
	ByteAddressBuffer m_Alive1List;
	ByteAddressBuffer m_Alive2List;
	ByteAddressBuffer m_DeadList;
	ByteAddressBuffer m_Counters;
	ByteAddressBuffer* m_CurrentAlive = &m_Alive1List; // 이번 프레임 입력
	ByteAddressBuffer* m_newAlive = &m_Alive2List; // 이번 프레임 출력

	// Indirect drawing
	IndirectArgsBuffer m_indirectArgsBuffer;
};

CREATE_APPLICATION(ParticleApp)
