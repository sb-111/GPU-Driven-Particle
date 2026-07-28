#pragma once
#include "pch.h"

class GraphicsContext;
namespace GP
{
	class Mesh;

	class SceneObject
	{
	public:
		SceneObject() {}
		void SetMesh(Mesh* mesh) { m_Mesh = mesh; }
		void Draw(GraphicsContext& gfx);
		Math::UniformTransform& GetTransform() { return m_Transform; }
		Math::Matrix4 GetWorldMatrix() const { return Math::Matrix4(Math::AffineTransform(m_Transform)); }
	private:
		Math::UniformTransform m_Transform = Math::UniformTransform(Math::kIdentity);
		Mesh* m_Mesh = nullptr;

	};
}

