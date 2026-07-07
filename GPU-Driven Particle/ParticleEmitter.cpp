#include "ParticleEmitter.h"

GP::EmitterCBParams GP::ParticleEmitter::MakeParams(float dt) const
{
	EmitterCBParams params;
	Math::Vector3 pos = m_EmitterTransform.GetTranslation();

	params.emitterPosition = { pos.GetX(), pos.GetY(), pos.GetZ() };
	params.emitCount = m_CurrentSpawnCount;

	Math::Vector3 dir = Math::Matrix3(m_EmitterTransform.GetRotation()).GetY();
	params.emitterDirection = { dir.GetX(), dir.GetY(), dir.GetZ() };
	params.deltaTime = dt;

	params.startColor = { 1.0f,1.0f,1.0f, 1.0f };
	params.minLifeTime = m_MinLifeTime;
	params.maxLifeTime = m_MaxLifeTime;
	params.randomeSeed = m_FrameCount;
	params.pad0 = 0;

	return params;
}
void GP::ParticleEmitter::Update(float dt)
{
	m_SpawnAccumulator += m_SpawnRate * dt;
	m_CurrentSpawnCount = (uint32_t)m_SpawnAccumulator; // 내림 처리 (소수점 스폰은 할 수 없으므로)
	m_SpawnAccumulator -= m_CurrentSpawnCount; // 다음 프레임에 넘겨줄 것
	m_FrameCount++;
}
