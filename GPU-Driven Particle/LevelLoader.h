#pragma once
#include <string>

namespace GP
{
	class Scene;
	class MeshLibrary;
	class Camera;
	class ParticleSystem;

	class LevelLoader
	{
	public:
		static bool Load(
			const char* path,
			Scene& scene,
			MeshLibrary& meshLibrary,
			Camera& camera,
			ParticleSystem& particles,
			std::string& outError);
	};
}
