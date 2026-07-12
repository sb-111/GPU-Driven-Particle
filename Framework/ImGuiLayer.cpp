#include "pch.h"
#include "ImGuiLayer.h"
#include "GraphicsCore.h"
#include "CommandContext.h"
#include "CommandListManager.h"
#include "BufferManager.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

#include <vector>

// imgui_impl_win32.h 에 이 선언만 주석처리돼 있어서 (windows.h 의존 때문) 여기서 직접 선언
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
    bool s_Initialized = false; // 초기화 여부 

    constexpr UINT kNumDescriptors = 64;	   // 디스크립터 개수
    ID3D12DescriptorHeap* s_SrvHeap = nullptr; // 힙
    UINT s_DescriptorSize = 0;				   // 디스크립터 한 칸 크기
    std::vector<UINT> s_FreeIndices;		   // 빈 칸 번호 목록

    void AllocSrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* outCpu, D3D12_GPU_DESCRIPTOR_HANDLE* outGpu)
    {
        ASSERT(!s_FreeIndices.empty(), "ImGui SRV 힙 고갈, kNumDescriptors를 늘릴 것");
		// [63, 62, ... , 0] -> 0부터 꺼내짐 (초기에)
        UINT idx = s_FreeIndices.back();
        s_FreeIndices.pop_back();
		// 시작주소 + 번호 x 칸 크기
        outCpu->ptr = s_SrvHeap->GetCPUDescriptorHandleForHeapStart().ptr + (SIZE_T)idx * s_DescriptorSize;
        outGpu->ptr = s_SrvHeap->GetGPUDescriptorHandleForHeapStart().ptr + (UINT64)idx * s_DescriptorSize;
    }

    void FreeSrvDescriptor(ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE)
    {
		// (cpu.ptr - 힙시작.ptr) / s_DescriptorSize -> 인덱스 역산
        UINT idx = (UINT)((cpu.ptr - s_SrvHeap->GetCPUDescriptorHandleForHeapStart().ptr) / s_DescriptorSize);
        s_FreeIndices.push_back(idx);
    }
}

namespace ImGuiLayer
{
    bool g_ShowDemoWindow = true;

    void Initialize(HWND hwnd)
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

		// 힙 준비
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; // 디스크립터 힙 타입 정의
        desc.NumDescriptors = kNumDescriptors;				// 64칸
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // shader visible 힙 (GPU가 읽을 수 있는 메모리)
        ASSERT_SUCCEEDED(Graphics::g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&s_SrvHeap)));
        s_SrvHeap->SetName(L"ImGui SRV Heap");
        s_DescriptorSize = Graphics::g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); // 칸 크기 조회
        s_FreeIndices.reserve(kNumDescriptors);    // 프리리스트 확보
        for (UINT i = 0; i < kNumDescriptors; ++i) // [63, 62, ..., 0]
            s_FreeIndices.push_back(kNumDescriptors - 1 - i);

		// Win32 백엔드를 이 창에 연결
        ImGui_ImplWin32_Init(hwnd);

		// 그리기 담당 연결
        ImGui_ImplDX12_InitInfo info;
        info.Device = Graphics::g_Device;
        info.CommandQueue = Graphics::g_CommandManager.GetCommandQueue();
        info.NumFramesInFlight = 3;   // Display.cpp SWAP_CHAIN_BUFFER_COUNT와 일치 필요
        info.RTVFormat = Graphics::g_OverlayBuffer.GetFormat();
        info.DSVFormat = DXGI_FORMAT_UNKNOWN;
        info.SrvDescriptorHeap = s_SrvHeap;
        info.SrvDescriptorAllocFn = AllocSrvDescriptor;
        info.SrvDescriptorFreeFn = FreeSrvDescriptor;
        ImGui_ImplDX12_Init(&info);

        s_Initialized = true;
    }

    void Shutdown()
    {
        if (!s_Initialized)
            return;

        ImGui_ImplDX12_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        s_SrvHeap->Release();
        s_SrvHeap = nullptr;
        s_FreeIndices.clear();
        s_Initialized = false;
    }

    void BeginFrame()
    {
        ImGui_ImplDX12_NewFrame();	// 그리기 담당
        ImGui_ImplWin32_NewFrame();	// 입력 담당
        ImGui::NewFrame();			// 위젯 접수 시작

        if (g_ShowDemoWindow)
            ImGui::ShowDemoWindow(&g_ShowDemoWindow);
    }

    void Render(GraphicsContext& Context)
    {
        ImGui::Render();	// 위젯 접수 마감
        Context.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, s_SrvHeap);	// 커맨드 리스트가 참조할 힙을 ImGui 힙으로 교체
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), Context.GetCommandList());  // 커맨드 리스트에 실제 드로우콜 기록
    }

    bool WantCaptureMouse()
    {
        return s_Initialized && ImGui::GetIO().WantCaptureMouse; // ImGui 패널 위에 있거나 위젯 드래그 중이면 true 
    }

    bool WantCaptureKeyboard()
    {
        return s_Initialized && ImGui::GetIO().WantCaptureKeyboard; // ImGui 패널 키보드 입력창이면 true
    }

    bool WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        if (!s_Initialized)
            return false;
        return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam) != 0;
    }
}
