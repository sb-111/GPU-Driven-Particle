#include "SDFBaker.h"
#include "Mesh.h"
#include "RenderTypes.h"
#include "GpuStats.h"
#include "ShaderCompiler.h"
#include "CommandContext.h"
#include "GraphicsCore.h"
#include "MathConvert.h"
#include "SystemTime.h"
void GP::SDFBaker::Init()
{
	auto meshSDFBakeCS = CompileShader(L"MeshSDFBakeCS.hlsl", L"main", L"cs_6_2");
	ASSERT(meshSDFBakeCS, "SDFBaker:: 셰이더 컴파일 실패 - VS 출력창 확인");

	// CBV, VB(SRV), IB(SRV), VolumeBuffer(UAV) 
	bakeRootSig.Reset(4, 0);
	bakeRootSig[0].InitAsConstantBuffer(0); // b0
	bakeRootSig[1].InitAsBufferSRV(0); // t0
	bakeRootSig[2].InitAsBufferSRV(1); // t1
	bakeRootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1); // u0
	bakeRootSig.Finalize(L"SDF Baker");

	bakePSO.SetRootSignature(bakeRootSig);
	bakePSO.SetComputeShader(meshSDFBakeCS->GetBufferPointer(), meshSDFBakeCS->GetBufferSize());
	bakePSO.Finalize();
}
void GP::SDFBaker::Bake(Mesh& mesh, uint32_t longestAxisResolution)
{
	auto sdf = std::make_unique<MeshSDF>();
	sdf->grid = ResolveSDFGrid(
		mesh.GetBoundsMin(),
		mesh.GetBoundsMax(),
		longestAxisResolution);

	const uint32_t resolutionX = sdf->grid.resolution[0];
	const uint32_t resolutionY = sdf->grid.resolution[1];
	const uint32_t resolutionZ = sdf->grid.resolution[2];

	sdf->volume.Create(L"Mesh SDF", resolutionX, resolutionY, resolutionZ, DXGI_FORMAT_R16_FLOAT);

	const uint32_t triangleCount = mesh.GetIndexCount() / 3;
	const int64_t bakeStart = SystemTime::GetCurrentTick();

	ComputeContext& cpt = ComputeContext::Begin(L"SDF Bake");
	cpt.TransitionResource(mesh.GetVertexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(mesh.GetIndexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(sdf->volume, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

	cpt.SetRootSignature(bakeRootSig);
	cpt.SetPipelineState(bakePSO);
	BakeConstants bakeConstants = {};
	bakeConstants.boundsMin = ToF3(sdf->grid.volumeBoundsMin);
	bakeConstants.boundsSize = ToF3(sdf->grid.volumeBoundsSize);
	bakeConstants.resolution[0] = resolutionX;
	bakeConstants.resolution[1] = resolutionY;
	bakeConstants.resolution[2] = resolutionZ;
	bakeConstants.triangleCount = triangleCount;
	bakeConstants.vertexStride = kVertexStride;
	bakeConstants.positionOffset = kPositionOffset;

	cpt.SetDynamicConstantBufferView(0, sizeof(bakeConstants), &bakeConstants);
	cpt.SetBufferSRV(1, mesh.GetVertexBuffer());
	cpt.SetBufferSRV(2, mesh.GetIndexBuffer());
	cpt.SetDynamicDescriptor(3, 0, sdf->volume.GetUAV());
	cpt.Dispatch((resolutionX + 7) / 8, (resolutionY + 7) / 8, (resolutionZ + 7) / 8);
	cpt.TransitionResource(sdf->volume, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.Finish(true);

	// device removed는 ASSERT_SUCCEEDED가 원인을 안 알려주므로 여기서 한 번 찍음
	HRESULT removedReason = Graphics::g_Device->GetDeviceRemovedReason();
	if (FAILED(removedReason))
		Utility::Printf("[SDF] device removed! reason = 0x%08X\n", removedReason);

	sdf->bakeMs = SystemTime::TicksToMillisecs(SystemTime::GetCurrentTick() - bakeStart);
	sdf->allocatedBytes = QueryResourceFootprint(sdf->volume.GetResource());
	mesh.SetSDF(std::move(sdf));
}
