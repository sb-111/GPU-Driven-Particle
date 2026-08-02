#include "GameCore.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "RenderTypes.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include "ShaderCompiler.h"
#include "ParticleSystem.h"
#include "ParticlePanel.h"

#include "Camera.h"
#include "CameraController.h"
#include "MathConvert.h"

#include "Mesh.h"
#include "SceneObject.h"
#include "SDFBaker.h"

#include "MeshLibrary.h"
#include "Scene.h"
#include "SceneObjectPanel.h"
#include "LevelLoader.h"
using namespace GameCore;
using namespace Graphics;
using namespace GP;

// 상수버퍼 구조체 cbuffer(b0) 와 1:1 대응
__declspec(align(16)) struct SceneConstants
{
	Math::Matrix4 world;
	Math::Matrix4 viewProj;
};

static ParticleViewCB makeViewCB(const Camera& camera)
{
	ParticleViewCB cb = {};
	cb.viewProj = ToF4x4(camera.GetViewProj());
	cb.camPos = ToF3(camera.GetPosition());
	cb.camUp = ToF3(camera.GetUp());
	cb.camForward = ToF3(camera.GetForward());
	cb.camRight = ToF3(camera.GetRight());
	return cb;
}

class ParticleApp : public IGameApp
{
public:
	// ==============================================================
	// 준비 단계 (앱 시작 시 1회). 이후 거의 안 바뀜.
	// ==============================================================
	void Startup(void) override
	{
		// 0. 씬(바닥) 셰이더 컴파일 - 파티클 셰이더는 ParticleSystem::Init이 담당
		auto opaqueVS = CompileShader(L"OpaqueVS.hlsl", L"main", L"vs_6_2");
		auto opaquePS = CompileShader(L"OpaquePS.hlsl", L"main", L"ps_6_2");
		ASSERT(opaqueVS && opaquePS, "셰이더 컴파일 실패 - VS 출력창 확인");

		// 1. 씬(불투명) 루트시그 + PSO
		m_OpaqueRootSig.Reset(4, 0);
		m_OpaqueRootSig[0].InitAsConstantBuffer(0);   // b0 카메라
		m_OpaqueRootSig[1].InitAsBufferSRV(0);        // t0
		m_OpaqueRootSig[2].InitAsBufferSRV(1);        // t1
		m_OpaqueRootSig[3].InitAsConstantBuffer(1);	  // b1 머티리얼
		m_OpaqueRootSig.Finalize(L"Opaque", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		m_OpaquePSO.SetRootSignature(m_OpaqueRootSig);
		m_OpaquePSO.SetInputLayout(kVertexLayoutCount, kVertexLayout);
		m_OpaquePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		m_OpaquePSO.SetVertexShader(opaqueVS->GetBufferPointer(), opaqueVS->GetBufferSize());
		m_OpaquePSO.SetPixelShader(opaquePS->GetBufferPointer(), opaquePS->GetBufferSize());
		m_OpaquePSO.SetRasterizerState(RasterizerDefault);
		m_OpaquePSO.SetBlendState(BlendDisable);
		m_OpaquePSO.SetDepthStencilState(DepthStateReadWrite);   // 불투명: 테스트 + 쓰기
		m_OpaquePSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), g_SceneDepthBuffer.GetFormat());
		m_OpaquePSO.Finalize();

		// 2. 파티클 시스템
		m_Particles.Init(m_ParticleNum);

		// 3. 씬 로드
		std::string err;
		if (!LevelLoader::Load(
			"Scenes/defaultScene.json",
			m_Scene,
			m_MeshLibrary,
			m_Camera,
			m_Particles,
			err))
		{
			ASSERT(false, err.c_str());
		}

		for (const auto& object : m_Scene.GetObjects())
		{
			if (object->IsSDFCollider())
			{
				m_PanelTarget = object.get();
				break;
			}
		}

		// 4. SDF Baker
		m_SDFBaker.Init();
	
		for (const auto& object : m_Scene.GetObjects())
		{
			if (!object->IsSDFCollider())
				continue;

			// 같은 메시를 쓰는 콜라이더가 여럿이어도 베이크는 한 번
			if (object->GetMesh()->GetSDF() == nullptr)
				m_SDFBaker.Bake(*object->GetMesh());

			m_Particles.AddSDFCollider(object.get());
		}
	}

	void Cleanup(void) override {}

	void Update(float deltaT) override
	{
		m_CamController.Update(deltaT);

		// 튜닝 패널
		DrawParticlePanel(m_Particles, m_Paused, m_Camera);
		DrawSceneObjectPanel(m_Scene, m_PanelTarget);

		// 멈춤 요청 들어오면 이미터 업데이트 정지
		m_Particles.Update(m_Paused ? 0.0f : deltaT);
	}

	// ==============================================================
	//  매 프레임. GPU 명령을 "기록"만 함 (실행은 Finish 이후 GPU가)
	// ==============================================================
	void RenderScene(void) override
	{
		GraphicsContext& gfx = GraphicsContext::Begin(L"Frame");
		m_Camera.Update();

		// =============== 컴퓨트: 파티클 시뮬레이션 ==============
		// View 데이터 (이미터/시스템은 카메라를 모름)
		ParticleViewCB viewCB = makeViewCB(m_Camera);
		m_Particles.UpdateGPU(gfx.GetComputeContext(), viewCB);

		// =============== 그래픽스 공통 준비 ==============
		gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true); // RTV
		gfx.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);   // DSV

		float clearColor[4] = { 0.02f, 0.02f, 0.05f, 1.0f };
		gfx.ClearColor(g_SceneColorBuffer, clearColor);
		gfx.ClearDepth(g_SceneDepthBuffer);

		gfx.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
		gfx.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());

		SceneConstants cb = {};
		cb.viewProj = m_Camera.GetViewProj();

		// =============== 씬 오브젝트 ==============
		gfx.SetRootSignature(m_OpaqueRootSig);   // 루트 인자보다 먼저
		gfx.SetPipelineState(m_OpaquePSO);
		gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		auto drawObject = [&](SceneObject& obj)
		{
			cb.world = obj.GetWorldMatrix();
			gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb);
			gfx.SetDynamicConstantBufferView(3, sizeof(MaterialCB), &obj.GetMaterial()); // b1
			obj.Draw(gfx);
		};

		for (const auto& object : m_Scene.GetObjects())
		{
			if (!object->IsVisible())
				continue;
			drawObject(*object);
		}

		// =============== 파티클 ==============
		m_Particles.Render(gfx, viewCB);
		gfx.Finish();

		m_Particles.EndFrame();   // 핑퐁 스왑
	}

private:
	// 초기 카메라 위치 지정, 회전 X
	Camera m_Camera{ Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 5.0f)) };
	CameraController m_CamController{ m_Camera };

	static const uint32_t m_ParticleNum = 1 << 20;
	ParticleSystem m_Particles;

	// 씬 (불투명). TODO: Scene 클래스로 통째로 이관할 부분
	RootSignature m_OpaqueRootSig;
	GraphicsPSO   m_OpaquePSO;
	SDFBaker m_SDFBaker;

	MeshLibrary m_MeshLibrary; // 모든 메시를 소유
	Scene m_Scene;			   // 모든 SceneObject를 소유

	SceneObject* m_PanelTarget = nullptr;

	bool m_Paused = false;
};

CREATE_APPLICATION(ParticleApp)
