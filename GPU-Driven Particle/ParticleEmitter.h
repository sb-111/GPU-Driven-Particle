#pragma once
#include "ParticleShared.h"
#include "ParticleSetting.h"
#include "VectorMath.h"

namespace GP
{
	class ParticleEmitter
	{
	public:
		explicit ParticleEmitter(const Math::OrthogonalTransform& transform)
			: m_EmitterTransform(transform) {}
		void Init();
		void Update(float dt, const ParticleSettings& s); // 튜닝 값은 Settings에서 읽음
		uint32_t GetCurrentSpawnCount() const { return m_CurrentSpawnCount; }

		// ParticleSettings -> ParticleFrameCB
		ParticleFrameCB MakeParams(const ParticleSettings& s, float dt) const; 
	private:
		Math::OrthogonalTransform m_EmitterTransform;
		//float m_EmitterAge;
		//float m_LoopDuration;

		float m_SpawnAccumulator = 0.0f; // 소수점 스폰 이월용
		uint32_t m_CurrentSpawnCount = 0; // 이번 프레임 스폰 수
		uint32_t m_FrameCount = 0;		  // 프레임 카운터


	};


}
