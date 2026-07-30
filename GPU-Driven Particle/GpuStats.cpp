#include "pch.h"
#include "GpuStats.h"
#include "GraphicsCore.h"

#include <dxgi1_4.h>
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")

namespace
{
	// g_Device 생성 시 쓰인 어댑터를 LUID로 다시 찾아 캐싱
	// (GraphicsCore가 어댑터를 전역으로 안 들고 있어서 재획득이 필요)
	IDXGIAdapter3* GetAdapter()
	{
		static Microsoft::WRL::ComPtr<IDXGIAdapter3> s_Adapter;
		static bool s_Tried = false;

		if (s_Tried)
			return s_Adapter.Get();
		s_Tried = true;

		if (Graphics::g_Device == nullptr)
			return nullptr;

		Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
		if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))))
			return nullptr;

		Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
		if (FAILED(factory->EnumAdapterByLuid(Graphics::g_Device->GetAdapterLuid(), IID_PPV_ARGS(&adapter1))))
			return nullptr;

		adapter1.As(&s_Adapter); // 실패해도 null로 남으므로 그대로 반환
		return s_Adapter.Get();
	}
}

GP::VideoMemoryInfo GP::QueryVideoMemory()
{
	VideoMemoryInfo out;

	IDXGIAdapter3* adapter = GetAdapter();
	if (adapter == nullptr)
		return out;

	DXGI_QUERY_VIDEO_MEMORY_INFO info = {};
	if (FAILED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)))
		return out;

	out.budget = info.Budget;
	out.currentUsage = info.CurrentUsage;
	out.valid = true;
	return out;
}

uint64_t GP::QueryResourceFootprint(ID3D12Resource* resource)
{
	if (resource == nullptr || Graphics::g_Device == nullptr)
		return 0;

	D3D12_RESOURCE_DESC desc = resource->GetDesc();
	D3D12_RESOURCE_ALLOCATION_INFO info = Graphics::g_Device->GetResourceAllocationInfo(0, 1, &desc);
	return info.SizeInBytes;
}
