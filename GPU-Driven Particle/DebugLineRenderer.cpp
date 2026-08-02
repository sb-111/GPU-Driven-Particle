#include "pch.h"
#include "DebugLineRenderer.h"
#include "ShaderCompiler.h"
#include "GraphicsCommon.h"
#include "BufferManager.h"
#include "CommandContext.h"
using namespace Graphics;
void GP::DebugLineRenderer::Init()
{
	auto debugLineVS = CompileShader(L"DebugLineVS.hlsl", L"main", L"vs_6_2");
	auto debugLinePS = CompileShader(L"DebugLinePS.hlsl", L"main", L"ps_6_2");
	ASSERT(debugLineVS && debugLinePS, "셰이더 컴파일 실패 - VS 출력창 확인");

	m_RootSig.Reset(1, 0);
	m_RootSig[0].InitAsConstantBuffer(0); // b0: DebugLineCB
	m_RootSig.Finalize(L"Debug Line",
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_PSO.SetRootSignature(m_RootSig);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	static const D3D12_INPUT_ELEMENT_DESC kDebugLineLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
	m_PSO.SetInputLayout(2, kDebugLineLayout);
	m_PSO.SetVertexShader(debugLineVS->GetBufferPointer(), debugLineVS->GetBufferSize());
	m_PSO.SetPixelShader(debugLinePS->GetBufferPointer(), debugLinePS->GetBufferSize());
	m_PSO.SetRasterizerState(RasterizerDefault);
	m_PSO.SetBlendState(BlendDisable);
	m_PSO.SetDepthStencilState(DepthStateReadOnly);
	m_PSO.SetRenderTargetFormat(
		g_SceneColorBuffer.GetFormat(),
		g_SceneDepthBuffer.GetFormat());
	m_PSO.Finalize();
}

void GP::DebugLineRenderer::AddLine(const Math::Vector3& start, const Math::Vector3& end, const Math::Vector4& color)
{
	DebugLineVertex v0 = {};
	v0.position[0] = start.GetX();
	v0.position[1] = start.GetY();
	v0.position[2] = start.GetZ();

	v0.color[0] = color.GetX();
	v0.color[1] = color.GetY();
	v0.color[2] = color.GetZ();
	v0.color[3] = color.GetW();

	DebugLineVertex v1 = v0;
	v1.position[0] = end.GetX();
	v1.position[1] = end.GetY();
	v1.position[2] = end.GetZ();

	m_Vertices.push_back(v0);
	m_Vertices.push_back(v1);

}

void GP::DebugLineRenderer::AddAABB(const Math::Vector3& boundsMin, const Math::Vector3& boundsMax, const Math::Vector4& color)
{
	const Math::Vector3 corners[8] =
	{
		{ boundsMin.GetX(), boundsMin.GetY(), boundsMin.GetZ() }, // 0
		{ boundsMax.GetX(), boundsMin.GetY(), boundsMin.GetZ() }, // 1
		{ boundsMax.GetX(), boundsMax.GetY(), boundsMin.GetZ() }, // 2
		{ boundsMin.GetX(), boundsMax.GetY(), boundsMin.GetZ() }, // 3

		{ boundsMin.GetX(), boundsMin.GetY(), boundsMax.GetZ() }, // 4
		{ boundsMax.GetX(), boundsMin.GetY(), boundsMax.GetZ() }, // 5
		{ boundsMax.GetX(), boundsMax.GetY(), boundsMax.GetZ() }, // 6
		{ boundsMin.GetX(), boundsMax.GetY(), boundsMax.GetZ() }, // 7
	};

	static constexpr uint8_t kEdges[12][2] =
	{
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, // min Z 면
		{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, // max Z 면
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }, // 두 면 연결
	};

	m_Vertices.reserve(m_Vertices.size() + 24);

	for (const auto& edge : kEdges)
		AddLine(corners[edge[0]], corners[edge[1]], color);
}

void GP::DebugLineRenderer::AddAABB(
	const Math::Vector3& boundsMin,
	const Math::Vector3& boundsMax,
	const Math::Matrix4& localToWorld,
	const Math::Vector4& color)
{
	const Math::Vector3 localCorners[8] =
	{
		{ boundsMin.GetX(), boundsMin.GetY(), boundsMin.GetZ() },
		{ boundsMax.GetX(), boundsMin.GetY(), boundsMin.GetZ() },
		{ boundsMax.GetX(), boundsMax.GetY(), boundsMin.GetZ() },
		{ boundsMin.GetX(), boundsMax.GetY(), boundsMin.GetZ() },
		{ boundsMin.GetX(), boundsMin.GetY(), boundsMax.GetZ() },
		{ boundsMax.GetX(), boundsMin.GetY(), boundsMax.GetZ() },
		{ boundsMax.GetX(), boundsMax.GetY(), boundsMax.GetZ() },
		{ boundsMin.GetX(), boundsMax.GetY(), boundsMax.GetZ() },
	};

	Math::Vector3 worldCorners[8];
	for (uint32_t i = 0; i < 8; ++i)
		worldCorners[i] = Math::Vector3(localToWorld * localCorners[i]);

	static constexpr uint8_t kEdges[12][2] =
	{
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 },
		{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 },
	};

	m_Vertices.reserve(m_Vertices.size() + 24);
	for (const auto& edge : kEdges)
		AddLine(worldCorners[edge[0]], worldCorners[edge[1]], color);
}

void GP::DebugLineRenderer::Render(GraphicsContext& gfx, const Math::Matrix4& viewProj)
{
	if (m_Vertices.empty())
		return;

	DebugLineCB cb = {};
	cb.viewProj = viewProj;

	gfx.SetRootSignature(m_RootSig);
	gfx.SetPipelineState(m_PSO);
	gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	gfx.SetDynamicConstantBufferView(0, sizeof(cb), &cb);
	gfx.SetDynamicVB(
		0,
		m_Vertices.size(),
		sizeof(DebugLineVertex),
		m_Vertices.data());
	gfx.Draw(static_cast<UINT>(m_Vertices.size()));

	Clear();
}

