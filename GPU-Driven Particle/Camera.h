#pragma once

#include "VectorMath.h"

namespace GP
{
	class Camera
	{
	public:

		Camera(const Math::OrthogonalTransform& cameraToWorld)
			: m_CameraToWorld(cameraToWorld)
		{

		}
		
		/*
		* 카메라 포즈(위치 + 회전)를 "어디에 서서 / 어디를 보고 / 위가 어디인지" 로 한 번에 설정한다.
		* 내부에서 forward = (at - eye) 로 방향을 구해 SetLookDirection() 을 호출한 뒤, 위치를 eye 로 세팅.
		* @param eye  카메라 월드 위치
		* @param at   바라보는 목표 지점 (월드)
		* @param up   위쪽 힌트 (보통 (0,1,0))
		*/
		void SetEyeAtUp(const Math::Vector3& eye, const Math::Vector3& at, const Math::Vector3& up);
		/*
		* 카메라가 특정 "방향"을 바라보도록 회전만 설정한다. (위치 설정 X)
		* 정규직교 기저(right / up / -forward)를 만들어 m_CameraToWorld 의 회전에 저장.
		* @param forward 바라볼 방향 벡터 
		* @param up      위쪽 힌트
		*/
		void SetLookDirection(const Math::Vector3& forward, const Math::Vector3& up);
		void SetPerspective(float fovY, float aspect, float nearZ, float farZ);

		// 행렬 재계산
		void Update();

		const Math::Matrix4& GetViewProj() const { return m_ViewProjMat; }
		Math::Vector3 GetPosition() const { return m_CameraToWorld.GetTranslation(); }
		Math::Vector3 GetForward() const { return -Math::Matrix3(m_CameraToWorld.GetRotation()).GetZ(); }
		Math::Vector3 GetRight() const { return Math::Matrix3(m_CameraToWorld.GetRotation()).GetX(); }
		Math::Vector3 GetUp() const { return Math::Matrix3(m_CameraToWorld.GetRotation()).GetY(); }


	private:
		// 카메라 월드 행렬
		Math::OrthogonalTransform m_CameraToWorld;

		Math::Matrix4 m_ViewMat;
		Math::Matrix4 m_ProjMat;
		Math::Matrix4 m_ViewProjMat;

	};
}
