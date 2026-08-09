#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace GP
{
	class Mesh;
	class MorphTargetSet;

	// MorphTargetSet을 생성하고 공유하는 캐시
	class MorphTargetLibrary
	{
	public:
		MorphTargetLibrary() = default;
		~MorphTargetLibrary();

		MorphTargetLibrary(const MorphTargetLibrary&) = delete;
		MorphTargetLibrary& operator=(const MorphTargetLibrary&) = delete;

		MorphTargetSet& GetOrCreateSurfaceTarget(
			const std::string& name,
			const Mesh& mesh,
			uint32_t sampleCount,
			uint32_t seed);
		void Clear();

	private:
		// 해시 키
		struct SurfaceTargetKey
		{
			// 이 세개가 같으면 같은 MorphTargetSet
			const Mesh* mesh = nullptr;
			uint32_t sampleCount = 0;
			uint32_t seed = 0;

			bool operator==(const SurfaceTargetKey& other) const
			{
				return mesh == other.mesh &&
					sampleCount == other.sampleCount &&
					seed == other.seed;
			}
		};

		struct SurfaceTargetKeyHash
		{
			size_t operator()(const SurfaceTargetKey& key) const
			{
				const size_t meshHash = std::hash<const Mesh*>{}(key.mesh);
				const size_t countHash = std::hash<uint32_t>{}(key.sampleCount);
				const size_t seedHash = std::hash<uint32_t>{}(key.seed);

				return meshHash ^ (countHash << 1) ^ (seedHash << 2);
			}
		};

		std::unordered_map<SurfaceTargetKey, std::unique_ptr<MorphTargetSet>, SurfaceTargetKeyHash> m_SurfaceTargets;
	};
}
