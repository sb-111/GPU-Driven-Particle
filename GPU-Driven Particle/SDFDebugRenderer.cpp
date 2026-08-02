#include "pch.h"
#include "SDFDebugRenderer.h"
#include "GraphicsCommon.h"
#include "Mesh.h"
#include "ShaderCompiler.h"
#include "BufferManager.h"
#include "CommandContext.h"
using namespace Graphics;
void GP::SDFDebugRenderer::Init()
{
	auto sdfSliceDebugVS = CompileShader(L"SDFSliceDebugVS.hlsl", L"main", L"vs_6_2");
	auto sdfSliceDebugPS = CompileShader(L"SDFSliceDebugPS.hlsl", L"main", L"ps_6_2");
	ASSERT(sdfSliceDebugVS && sdfSliceDebugPS, "셰이더 컴파일 실패 - VS 출력창 확인");

	m_RootSig.Reset(2, 0);
	m_RootSig[0].InitAsConstantBuffer(0); // b0: SDFSliceCB
	m_RootSig[1].InitAsDescriptorRange(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		0, 1,
		D3D12_SHADER_VISIBILITY_PIXEL);   // t0: Texture3D
	m_RootSig.Finalize(L"SDF Slice Debug",
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_PSO.SetRootSignature(m_RootSig);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetInputLayout(0, nullptr);
	m_PSO.SetVertexShader(sdfSliceDebugVS->GetBufferPointer(), sdfSliceDebugVS->GetBufferSize());
	m_PSO.SetPixelShader(sdfSliceDebugPS->GetBufferPointer(), sdfSliceDebugPS->GetBufferSize());
	m_PSO.SetRasterizerState(RasterizerTwoSided);
	m_PSO.SetBlendState(BlendDisable);
	m_PSO.SetDepthStencilState(DepthStateDisabled);
	m_PSO.SetRenderTargetFormat(
		g_SceneColorBuffer.GetFormat(),
		DXGI_FORMAT_UNKNOWN);
	m_PSO.Finalize();
}
void GP::SDFDebugRenderer::RenderSlice(GraphicsContext& gfx, MeshSDF& sdf,
	const Math::Matrix4& localToWorld, const Math::Matrix4& viewProj,
	ESDFSliceAxis axis, uint32_t sliceIndex, float distanceRange)
{
	if (sdf.volume.GetResource() == nullptr)
		return;

	const SDFGrid& grid = sdf.grid;
	const uint32_t axisIndex = static_cast<uint32_t>(axis);

	ASSERT(axisIndex < 3, "Invalid SDF slice axis");
	ASSERT(grid.resolution[axisIndex] > 0, "SDF slice resolution is zero");

	SDFSliceCB cb = {};
	cb.localToWorld = localToWorld;
	cb.viewProj = viewProj;

	cb.volumeBoundsMin[0] = (float)grid.volumeBoundsMin.GetX();
	cb.volumeBoundsMin[1] = (float)grid.volumeBoundsMin.GetY();
	cb.volumeBoundsMin[2] = (float)grid.volumeBoundsMin.GetZ();
	cb.axis = axisIndex;

	cb.volumeBoundsSize[0] = (float)grid.volumeBoundsSize.GetX();
	cb.volumeBoundsSize[1] = (float)grid.volumeBoundsSize.GetY();
	cb.volumeBoundsSize[2] = (float)grid.volumeBoundsSize.GetZ();
	cb.sliceIndex = std::min(
		sliceIndex,
		grid.resolution[axisIndex] - 1);

	cb.resolution[0] = grid.resolution[0];
	cb.resolution[1] = grid.resolution[1];
	cb.resolution[2] = grid.resolution[2];
	cb.distanceRange = distanceRange;

	gfx.TransitionResource(
		sdf.volume,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	gfx.SetRootSignature(m_RootSig);
	gfx.SetPipelineState(m_PSO);
	gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb);
	gfx.SetDynamicDescriptor(1, 0, sdf.volume.GetSRV());

	gfx.Draw(6); // Quad 생성
}
