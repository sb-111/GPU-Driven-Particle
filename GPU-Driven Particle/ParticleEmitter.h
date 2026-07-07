#pragma once
#include "ParticleShared.h"
#include "VectorMath.h"

namespace GP
{
	class ParticleEmitter
	{
	public:
		ParticleEmitter(const Math::OrthogonalTransform& transform, float spawnRate, float minLifeTime, float maxLifeTime)
		: m_EmitterTransform(transform), m_SpawnRate(spawnRate), m_MinLifeTime(minLifeTime), m_MaxLifeTime(maxLifeTime)
			{}
		void Update(float dt);
		uint32_t GetCurrentSpawnCount() const { return m_CurrentSpawnCount; }
		EmitterCBParams MakeParams(float dt) const;
	private:
		Math::OrthogonalTransform m_EmitterTransform;
		//float m_EmitterAge;
		//float m_LoopDuration;
		//float m_StartColor;
		float m_SpawnRate; // 파티클의 초당 방출 수
		float m_MinLifeTime; // 파티클의 min 수명
		float m_MaxLifeTime; // 파티클의 max 수명

		float m_SpawnAccumulator = 0.0f; // 소수점 스폰 이월용
		uint32_t m_CurrentSpawnCount = 0; // 이번 프레임 스폰 수
		uint32_t m_FrameCount = 0;		  // 프레임 카운터


	};


}
