#include "Camera.h"
#include <cmath>

using namespace Math;

void GP::Camera::SetEyeAtUp(const Math::Vector3& eye, const Math::Vector3& at, const Math::Vector3& up)
{
	SetLookDirection(at - eye, up);        // forward 구해서 회전 설정
	m_CameraToWorld.SetTranslation(eye);   // 카메라 월드 위치 세팅
}

void GP::Camera::SetLookDirection(const Math::Vector3& forward, const Math::Vector3& up)
{
	// ex) forward = (0,0,-1), up = (0,1,0)
	Vector3 normalizedForward = Normalize(forward);
	Vector3 right = Normalize(Cross(normalizedForward, up));
	Vector3 cameraUp = Cross(right, normalizedForward);

	// 열벡터 기저 (x, y, -z)
	Matrix3 basis(right, cameraUp, -normalizedForward);
	// 내부적으로 회전을 쿼터니언으로 저장
	m_CameraToWorld.SetRotation(Quaternion(basis));
}

void GP::Camera::SetPerspective(float fovY, float aspect, float nearZ, float farZ)
{
	float Y = 1 / std::tanf(fovY * 0.5f); // 세로 스케일(화면 세로 절반 크기 역수)
	float X = Y * aspect; // 가로 스케일 *aspect = h/w (MiniEngine 관례)
	// near -> 1, far -> 0
	float Q1, Q2;
	Q1 = nearZ / (farZ - nearZ); 
	Q2 = Q1 * farZ;

	m_ProjMat = Matrix4(
		Vector4(X, 0.0f, 0.0f, 0.0f),
		Vector4(0.0f, Y, 0.0f, 0.0f),
		Vector4(0.0f, 0.0f, Q1, -1.0f),
		Vector4(0.0f, 0.0f, Q2, 0.0f)
	);
}

void GP::Camera::Update()
{
	m_ViewMat = Matrix4(~m_CameraToWorld); // 뷰 행렬 = 카메라 월드 트랜스폼의 역

	m_ViewProjMat = m_ProjMat * m_ViewMat; // 뷰프로젝션 = 투영 * 뷰 (열벡터라 Proj가 왼쪽)
}
