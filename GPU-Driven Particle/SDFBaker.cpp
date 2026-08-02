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
void GP::SDFBaker::Bake(Mesh& mesh, uint32_t baseResolution)
{
	auto sdf = std::make_unique<MeshSDF>();
	sdf->grid = ResolveSDFGrid(
		mesh.GetBoundsMin(),
		mesh.GetBoundsMax(),
		baseResolution);

	const uint32_t resolutionX = sdf->grid.resolution[0];
	const uint32_t resolutionY = sdf->grid.resolution[1];
	const uint32_t resolutionZ = sdf->grid.resolution[2];
	sdf->volume.Create(L"Mesh SDF", resolutionX, resolutionY, resolutionZ, DXGI_FORMAT_R16_FLOAT);

	const uint32_t triangleCount = mesh.GetIndexCount() / 3;
	const uint64_t voxelCount = (uint64_t)resolutionX * resolutionY * resolutionZ;
	const VideoMemoryInfo vramBefore = QueryVideoMemory();
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

	const double bakeMs = SystemTime::TicksToMillisecs(SystemTime::GetCurrentTick() - bakeStart); // 시간
	const VideoMemoryInfo vramAfter = QueryVideoMemory();										  // vram

	// 이론값: 복셀 수 x 2바이트(R16_FLOAT). 실제 할당은 정렬 패딩이 붙을 수 있음
	const uint64_t theoretical = voxelCount * 2;
	const uint64_t footprint = QueryResourceFootprint(sdf->volume.GetResource());

	Utility::Printf("[SDF] %ux%ux%u, tri %u, %.1f ms (%.4f ns/voxel/tri)\n",
		resolutionX, resolutionY, resolutionZ, triangleCount, bakeMs,
		(triangleCount && voxelCount) ? (bakeMs * 1e6) / (double)(voxelCount * triangleCount) : 0.0);
	Utility::Printf("[SDF] 이론 %.2f MB / 실할당 %.2f MB",
		theoretical / 1048576.0, footprint / 1048576.0);
	if (vramBefore.valid && vramAfter.valid)
		Utility::Printf(" / VRAM 델타 %.2f MB (사용 %.0f MB, 예산 %.0f MB)",
			(double)((int64_t)vramAfter.currentUsage - (int64_t)vramBefore.currentUsage) / 1048576.0,
			vramAfter.currentUsage / 1048576.0, vramAfter.budget / 1048576.0);
	Utility::Printf("\n");

	mesh.SetSDF(std::move(sdf));
}
