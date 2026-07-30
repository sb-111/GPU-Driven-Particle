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
#include "MeshLoader.h"
#include "SceneObject.h"
#include "SDFBaker.h"
#include "SystemTime.h"
using namespace GameCore;
using namespace Graphics;
using namespace GP;

// 상수버퍼 구조체 cbuffer(b0) 와 1:1 대응
__declspec(align(16)) struct SceneConstants
{
	Math::Matrix4 world;
	Math::Matrix4 viewProj;
};
// OBJ -> RawMesh -> (노멀 보정) -> 정점 팩 -> GPU 업로드.
// TODO: MeshAssetLoader(캐시 포함)로 옮기기
static bool LoadMeshAsset(Mesh& mesh, const char* path, ENormalMode mode = ENormalMode::Smooth)
{
	RawMesh raw;
	std::string err;
	int64_t start = SystemTime::GetCurrentTick();

	if (!LoadOBJ(path, raw, &err))
	{
		Utility::Printf("[Mesh] %s 로드 실패: %s\n", path, err.c_str());
		return false;
	}
	EnsureNormals(raw, mode);

	std::vector<Vertex> verts;
	PackVertices(raw, verts);
	mesh.Create(verts, raw.indices);

	Utility::Printf("[Mesh] %s: 정점 %u, 삼각형 %u, %.1f ms\n", path,
		raw.VertexCount(), raw.TriangleCount(),
		SystemTime::TicksToMillisecs(SystemTime::GetCurrentTick() - start));
	return true;
}

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

		// Plane 메시 로드
		LoadMeshAsset(m_FloorMesh, "Meshes/plane.obj");
		m_FloorObject.SetName("Floor");
		m_FloorObject.SetMesh(&m_FloorMesh);
		m_FloorObject.GetTransform().SetTranslation(Math::Vector3(0.0f, -2.0f, 0.0f));
		m_FloorObject.GetTransform().SetScale(40.0f);
		m_FloorObject.GetMaterial() = MaterialCB{ { 0.11f, 0.11f, 0.13f, 1.0f } };

		m_SphereMesh.CreateSphere(1.5f, 16, 32);
		m_SphereObject.SetName("Sphere");
		m_SphereObject.SetMesh(&m_SphereMesh);
		m_SphereObject.GetTransform().SetTranslation(Math::Vector3(0.0f, -5.0f, 0.0f));
		m_SphereObject.GetMaterial() = MaterialCB{ { 0.85f, 0.2f, 0.15f, 1.0f } };

		LoadMeshAsset(m_CubeMesh, "Meshes/cube.obj");
		m_CubeObject.SetName("Cube");
		m_CubeObject.SetMesh(&m_CubeMesh);
		m_CubeObject.GetTransform().SetTranslation(Math::Vector3(2.5f, 0.0f, 0.0f));
		m_CubeObject.GetTransform().SetScale(2.0f);
		m_CubeObject.GetMaterial() = MaterialCB{ { 0.25f, 0.55f, 0.85f, 1.0f } };

		// 2. 씬(불투명) 루트시그 + PSO
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

		// 3. 파티클 시스템
		m_Particles.Init(m_ParticleNum);

		// 4. 카메라 투영 설정 (fov/near/far 고정 → 1회. 창 리사이즈 때만 갱신)
		float aspect = (float)g_SceneColorBuffer.GetHeight() / (float)g_SceneColorBuffer.GetWidth(); // ※ 높이/너비
		m_Camera.SetPerspective(3.14159f / 3.0f, aspect, 1.0f, 1000.0f); // 60도

		// 5. SDF Baker
		m_SDFBaker.Init();
		m_SDFBaker.Bake(m_SphereMesh, 64, 64, 64);
		m_SDFBaker.Bake(m_CubeMesh, 64, 64, 64);
		m_Particles.AddSDFCollider(&m_SphereObject);
		m_Particles.AddSDFCollider(&m_CubeObject);
	}

	void Cleanup(void) override {}

	void Update(float deltaT) override
	{
		m_CamController.Update(deltaT);

		// 튜닝 패널
		DrawParticlePanel(m_Particles, m_Paused, m_Camera, &m_SphereObject);

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

		drawObject(m_FloorObject);
		drawObject(m_SphereObject);
		drawObject(m_CubeObject);

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
	Mesh m_FloorMesh;			// Asset
	Mesh m_SphereMesh;
	Mesh m_CubeMesh;
	SceneObject m_FloorObject;	// Instance
	SceneObject m_SphereObject;
	SceneObject m_CubeObject;
	SDFBaker m_SDFBaker;

	bool m_Paused = false;
};

CREATE_APPLICATION(ParticleApp)
