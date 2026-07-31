#include "Scene.h"
#include "SceneObject.h"
namespace GP
{
	Scene::Scene() = default;
	Scene::~Scene() = default;

	SceneObject& GP::Scene::CreateObject(Mesh* mesh)
	{
		// Mesh를 가진 SceneObject를 생성하고 목록에 삽입 
		m_Objects.push_back(std::make_unique<SceneObject>(mesh));
		return *m_Objects.back();
	}
	void GP::Scene::Clear()
	{
		m_Objects.clear();
	}

}
