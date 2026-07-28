#include "SDFBaker.h"
#include "Mesh.h"
#include "ShaderCompiler.h"
#include "CommandContext.h"
#include "MathConvert.h"
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
void GP::SDFBaker::Bake(Mesh& mesh, uint32_t resolutionX, uint32_t resolutionY, uint32_t resolutionZ)
{
	auto sdf = std::make_unique<MeshSDF>();
	sdf->resolution[0] = resolutionX;
	sdf->resolution[1] = resolutionY;
	sdf->resolution[2] = resolutionZ;
	sdf->volume.Create(L"Mesh SDF", resolutionX, resolutionY, resolutionZ, DXGI_FORMAT_R16_FLOAT);

	Math::Vector3 center = (mesh.GetBoundsMin() + mesh.GetBoundsMax()) * 0.5f;
	Math::Vector3 halfSize = ((mesh.GetBoundsMax() - mesh.GetBoundsMin()) * 0.5f) * 1.1f; // margin 조금 줌(1.1배)
	Math::Vector3 boundsMin = center - halfSize;
	Math::Vector3 boundsSize = halfSize * 2.0f;
	sdf->boundsMin = boundsMin;
	sdf->boundsSize = boundsSize;

	ComputeContext& cpt = ComputeContext::Begin(L"SDF Bake");
	cpt.TransitionResource(mesh.GetVertexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(mesh.GetIndexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(sdf->volume, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

	cpt.SetRootSignature(bakeRootSig);
	cpt.SetPipelineState(bakePSO);
	BakeConstants bakeConstants = {};
	bakeConstants.boundsMin = ToF3(boundsMin);
	bakeConstants.boundsSize = ToF3(boundsSize);
	bakeConstants.resolution[0] = resolutionX;
	bakeConstants.resolution[1] = resolutionY;
	bakeConstants.resolution[2] = resolutionZ;
	bakeConstants.triangleCount = mesh.GetIndexCount() / 3;

	cpt.SetDynamicConstantBufferView(0, sizeof(bakeConstants), &bakeConstants);
	cpt.SetBufferSRV(1, mesh.GetVertexBuffer());
	cpt.SetBufferSRV(2, mesh.GetIndexBuffer());
	cpt.SetDynamicDescriptor(3, 0, sdf->volume.GetUAV());
	cpt.Dispatch((resolutionX + 7) / 8, (resolutionY + 7) / 8, (resolutionZ + 7) / 8);
	cpt.TransitionResource(sdf->volume, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.Finish(true);

	mesh.SetSDF(std::move(sdf));
}
