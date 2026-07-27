#include "pch.h"
#include "VolumeBuffer.h"
#include "GraphicsCore.h"

using namespace Graphics;
void VolumeBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Depth, DXGI_FORMAT Format)
{
	Destroy();

	D3D12_RESOURCE_DESC Desc = {};
	Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D; // 3D 텍스처
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.DepthOrArraySize = (UINT16)Depth;
	Desc.MipLevels = 1;
	Desc.Format = Format;
	Desc.SampleDesc.Count = 1;
	Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	Desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // UAV 가능

	// 클리어 값 nullptr (RT/DS 플래그 없는 리소스에 클리어 값을 넘기면 E_INVALIDARG)
	CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
		&Desc, D3D12_RESOURCE_STATE_COMMON, nullptr, MY_IID_PPV_ARGS(&m_pResource)));

	// 생성 시 초기 상태와 일치시켜야 됨
	m_UsageState = D3D12_RESOURCE_STATE_COMMON;
	m_Width = Width;
	m_Height = Height;
	m_ArraySize = Depth;
	m_Format = Format;

#ifndef RELEASE
	m_pResource->SetName(Name.c_str());
#else
	(Name);
#endif

	if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		m_SRVHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_UAVHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = Format;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Texture3D.MipLevels = 1;
	g_Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRVHandle);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.Format = Format;
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	UAVDesc.Texture3D.WSize = Depth;
	g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAVHandle);
}
