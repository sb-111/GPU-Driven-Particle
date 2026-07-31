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
		Mesh* m_Mesh = nullptr; // 메시 에셋 참조 (소유 X)
		MaterialCB m_Material;
		std::string m_Name;
		bool m_SDFCollider = false;
		bool m_Visible = true;
	};
}

