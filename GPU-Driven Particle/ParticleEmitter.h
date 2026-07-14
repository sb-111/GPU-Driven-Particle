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
		void ResetEmitter();
		uint32_t GetCurrentSpawnCount() const { return m_CurrentSpawnCount; }

		// ParticleSettings -> ParticleFrameCB
		ParticleFrameCB MakeParams(const ParticleSettings& s, float dt) const; 
	private:
		Math::OrthogonalTransform m_EmitterTransform;
		float m_AgeInLoop = 0.0f;	// 현재 루프에서 경과 시간
		bool m_Active = true;		// 이미터 생존 여부
		int m_CompletedLoops = 0;  // 현재 완료된 루프 수
		bool m_CanBurst = true; // 버스트

		float m_SpawnAccumulator = 0.0f; // 소수점 스폰 이월용
		uint32_t m_CurrentSpawnCount = 0; // 이번 프레임 스폰 수
		uint32_t m_FrameCount = 0;		  // 프레임 카운터


	};


}
