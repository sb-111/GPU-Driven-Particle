#include "pch.h"
#include "SkyboxRenderer.h"
#include "GraphicsCommon.h"
#include "ShaderCompiler.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "Camera.h"
#include "TextureLoader.h"
using namespace Graphics;

namespace
{
	__declspec(align(16)) struct SkyboxCB
	{
		Math::Matrix4 invViewProj;
		float cameraPos[3];
		float pad;
	};
}

void GP::SkyboxRenderer::Init(const char* ddsPath)
{
	ASSERT(LoadDDSTexture(m_CubeMap, ddsPath), "SkyboxRenderer: skybox dds load fail");

	auto screenQuadVS = CompileShader(L"ScreenQuadVS.hlsl", L"main", L"vs_6_2");
	auto skyboxPS = CompileShader(L"SkyboxPS.hlsl", L"main", L"ps_6_2");
	ASSERT(screenQuadVS && skyboxPS, "셰이더 컴파일 실패 - VS 출력창 확인");

	m_RootSig.Reset(2, 1);
	m_RootSig[0].InitAsConstantBuffer(0); // b0: SkyboxCB
	m_RootSig[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, D3D12_SHADER_VISIBILITY_PIXEL); // t0: TextureCube
	m_RootSig.InitStaticSampler(0, SamplerLinearClampDesc);  // s0
	m_RootSig.Finalize(L"Skybox");

	m_PSO.SetRootSignature(m_RootSig);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetInputLayout(0, nullptr);
	m_PSO.SetVertexShader(screenQuadVS->GetBufferPointer(), screenQuadVS->GetBufferSize());
	m_PSO.SetPixelShader(skyboxPS->GetBufferPointer(), skyboxPS->GetBufferSize());
	m_PSO.SetRasterizerState(RasterizerTwoSided);
	m_PSO.SetBlendState(BlendDisable);
	m_PSO.SetDepthStencilState(DepthStateReadOnly);
	m_PSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), g_SceneDepthBuffer.GetFormat());
	m_PSO.Finalize();
}

void GP::SkyboxRenderer::Render(GraphicsContext& gfx, const Camera& camera)
{
	SkyboxCB cb = {};
	cb.invViewProj = Math::Invert(camera.GetViewProj());
	cb.cameraPos[0] = (float)camera.GetPosition().GetX();
	cb.cameraPos[1] = (float)camera.GetPosition().GetY();
	cb.cameraPos[2] = (float)camera.GetPosition().GetZ();

	gfx.SetRootSignature(m_RootSig);
	gfx.SetPipelineState(m_PSO);
	gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb);
	gfx.SetDynamicDescriptors(1, 0, 1, &m_CubeMap.GetSRV());
	gfx.Draw(3);
}
