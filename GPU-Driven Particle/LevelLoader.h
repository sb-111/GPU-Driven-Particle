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

		// Load가 읽는 필드만 그대로 기록
		static bool Save(
			const char* path,
			const Scene& scene,
			const Camera& camera,
			ParticleSystem& particles,
			std::string& outError);
	};
}
