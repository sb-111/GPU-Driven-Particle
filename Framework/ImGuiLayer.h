//
// ImGui 통합 레이어
//
// 프레임 흐름:
//   GameCore::InitializeApplication -> Initialize(hwnd)
//   GameCore::UpdateApplication     -> BeginFrame() ... game.RenderUI() -> Render(UiContext)
//   GameCore::TerminateApplication  -> Shutdown()
//
// ImGui는 g_OverlayBuffer(UI 오버레이 RT)에 그려지고 Display::Present에서 씬과 합성
//

#pragma once

#include <windows.h>

class GraphicsContext;

namespace ImGuiLayer
{
    // 데모 창 토글(for test)
    extern bool g_ShowDemoWindow;

    void Initialize(HWND hwnd);
    void Shutdown();

    // 위젯 접수 시작 - io 갱신(창 크기/입력), 첫 프레임엔 폰트 텍스처 생성
    void BeginFrame();

    // 위젯 접수 마감 -> 쌓인 위젯들을 Context의 커맨드리스트에 드로우콜로 기록
    void Render(GraphicsContext& Context);

	// * GameInput은 DirectInput 사용 -> ImGui가 메시지 소비해도 같이 들어옴
	// GameInput이 항상 작동해서 ImGui랑 겹치는 걸 방지하기 위한 장치 
    bool WantCaptureMouse();
    bool WantCaptureKeyboard();

    // GameCore::WndProc 최상단에서 호출 (true면 메시지가 UI에 소비된 것)
    bool WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}
