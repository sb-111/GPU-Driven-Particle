#pragma once

#include "Camera.h"
#include "GameInput.h"
#include "VectorMath.h"
#include <cmath>
#include "ImGuiLayer.h"
// 언리얼 에디터식 : 우클릭 마우스로 시점, WASD 이동, E/Q 상승 및 하강, 휠로 이동속도.

namespace GP
{
	class CameraController
	{
		static constexpr float kDegToRad  = XM_PI / 180.0f;
		static constexpr float kMaxPitch  = 89.0f * kDegToRad;   // 수직 근처에서 뒤집히지 않게
		static constexpr float kSpeedMin  = 0.5f;
		static constexpr float kSpeedMax  = 200.0f;
		static constexpr float kSpeedMul  = 1.25f;               // 휠 한 칸당 속도 배율
		static constexpr float kLookScale = 1.0f;                // 마우스 감도

	public:
		explicit CameraController(Camera& camera) : m_Camera(camera) {}

		void Update(float dt)
		{
			using namespace Math;
			using namespace GameInput;

			// UI가 마우스 잡고 있으면 룩 시작 안 함 (이미 룩 중이면 유지)
			const bool held = IsPressed(kMouse1) && (m_Active || !ImGuiLayer::WantCaptureMouse()); 

			if (held && !m_Active)      BeginLook(); // 우클릭 && 안 눌려있음 -> 커서 숨김
			else if (!held && m_Active) EndLook();   // 우클릭X && 눌려있음 -> 커서 복원
			m_Active = held;						// 갱신

			if (!held)
				return;                              // 우클릭 안 하면 카메라는 그대로

			// 카메라 이동 속도 조절 (휠)
			const float wheel = GetAnalogInput(kAnalogMouseScroll);
			if (wheel > 0.0f)      m_Speed *= kSpeedMul;
			else if (wheel < 0.0f) m_Speed /= kSpeedMul;
			m_Speed = Clamp(m_Speed, kSpeedMin, kSpeedMax);

			// 마우스 회전
			m_Yaw   += GetAnalogInput(kAnalogMouseX) * kLookScale;
			m_Pitch += GetAnalogInput(kAnalogMouseY) * kLookScale;
			m_Pitch  = Clamp(m_Pitch, -kMaxPitch, kMaxPitch); // 수직 클램프

			// yaw/pitch로 방향 생성
			const float cp = cosf(m_Pitch), sp = sinf(m_Pitch);
			const float cy = cosf(m_Yaw),   sy = sinf(m_Yaw);
			const Vector3 forward(cp * sy, sp, -cp * cy);
			const Vector3 right(cy, 0.0f, sy);
			const Vector3 up(0.0f, 1.0f, 0.0f);

			const float step = m_Speed * dt;
			Vector3 pos = m_Camera.GetPosition();
			pos += forward * (Axis(kKey_w, kKey_s) * step); // 앞/뒤
			pos += right   * (Axis(kKey_d, kKey_a) * step); // 우/좌
			pos += up      * (Axis(kKey_e, kKey_q) * step); // 위/아래

			m_Camera.SetEyeAtUp(pos, pos + forward, up);
		}

	private:
		// 두 키를 +1 / -1 축으로 묶는다 (둘 다면 0)
		static float Axis(GameInput::DigitalInput plus, GameInput::DigitalInput minus)
		{
			return (GameInput::IsPressed(plus) ? 1.0f : 0.0f) - (GameInput::IsPressed(minus) ? 1.0f : 0.0f);
		}
		static float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

		void BeginLook() { GetCursorPos(&m_SavedCursor); ShowCursor(FALSE); } // 뗄 때 돌려놓을 위치 기억
		void EndLook()   { SetCursorPos(m_SavedCursor.x, m_SavedCursor.y); ShowCursor(TRUE); }

		Camera& m_Camera;
		float m_Yaw   = 0.0f;
		float m_Pitch = 0.0f;
		float m_Speed = 5.0f;   // 이동속도
		bool  m_Active = false; // 우클릭 홀드 상태
		POINT m_SavedCursor{};
	};
}
