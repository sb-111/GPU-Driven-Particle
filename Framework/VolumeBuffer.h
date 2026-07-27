#pragma once

#include "PixelBuffer.h"

class VolumeBuffer : public PixelBuffer
{
public:
	VolumeBuffer()
	{
		m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_UAVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Depth, DXGI_FORMAT Format);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_SRVHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const { return m_UAVHandle; }

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle;
};
