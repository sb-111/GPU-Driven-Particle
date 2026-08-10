#pragma once
#include "pch.h"
#include "RenderTypes.h"
#include <string>

class GraphicsContext;
namespace GP
{
	class Mesh;

	class SceneObject
	{
	public:
		explicit SceneObject(Mesh* mesh) : m_Mesh(mesh) {}
		void SetMesh(Mesh* mesh) { m_Mesh = mesh; }
		Mesh* GetMesh() const { return m_Mesh; }
		void Draw(GraphicsContext& gfx);
		Math::UniformTransform& GetTransform() { return m_Transform; }
		Math::Matrix4 GetWorldMatrix() const { return Math::Matrix4(Math::AffineTransform(m_Transform)); }

		const float (&GetRotationEuler() const)[3] { return m_RotationEuler; }
		void SetRotationEuler(const float (&eulerDeg)[3])
		{
			m_RotationEuler[0] = eulerDeg[0];
			m_RotationEuler[1] = eulerDeg[1];
			m_RotationEuler[2] = eulerDeg[2];
			m_Transform.SetRotation(Math::Quaternion(
				DirectX::XMConvertToRadians(eulerDeg[0]),
				DirectX::XMConvertToRadians(eulerDeg[1]),
				DirectX::XMConvertToRadians(eulerDeg[2])));
		}
		void SetRotation(const Math::Quaternion& rotation)
		{
			m_Transform.SetRotation(rotation);
			const Math::Matrix3 basis(rotation);
			const float sinPitch = fminf(1.0f, fmaxf(-1.0f, -(float)basis.GetZ().GetY()));
			m_RotationEuler[0] = DirectX::XMConvertToDegrees(asinf(sinPitch));
			m_RotationEuler[1] = DirectX::XMConvertToDegrees(
				atan2f((float)basis.GetZ().GetX(), (float)basis.GetZ().GetZ()));
			m_RotationEuler[2] = DirectX::XMConvertToDegrees(
				atan2f((float)basis.GetX().GetY(), (float)basis.GetY().GetY()));
		}

		// 색은 정점 소유 x
		MaterialCB& GetMaterial() { return m_Material; }
		const MaterialCB& GetMaterial() const { return m_Material; }

		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }

		bool IsSDFCollider() const { return m_SDFCollider; }
		void SetSDFCollider(bool active) { m_SDFCollider = active; }

		bool IsVisible() const { return m_Visible; }
		void SetVisible(bool visible) { m_Visible = visible; }
	private:
		Math::UniformTransform m_Transform = Math::UniformTransform(Math::kIdentity);
		float m_RotationEuler[3] = { 0.0f, 0.0f, 0.0f }; // 패널 표시용 (deg)
		Mesh* m_Mesh = nullptr; // 메시 에셋 참조 (소유 X)
		MaterialCB m_Material;
		std::string m_Name;
		bool m_SDFCollider = false;
		bool m_Visible = true;
	};
}

