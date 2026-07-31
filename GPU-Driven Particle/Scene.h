#pragma once
#include "pch.h"
namespace GP
{
	class Mesh;
	class SceneObject;
	/* Instance graph*/
	class Scene
	{
	public:
		Scene();
		~Scene();

		SceneObject& CreateObject(Mesh* mesh);
		void Clear();
		const std::vector<std::unique_ptr<SceneObject>>& GetObjects() const { return m_Objects; }
	private:
		std::vector<std::unique_ptr<SceneObject>> m_Objects; // 오브젝트 소유
	};


}
