#include "ParticleEmitter.h"

GP::ParticleFrameCB GP::ParticleEmitter::MakeParams(const ParticleSettings& s, float dt) const
{
	ParticleFrameCB params = {}; 
	Math::Vector3 pos = m_EmitterTransform.GetTranslation();

	params.emitterPosition = { pos.GetX(), pos.GetY(), pos.GetZ() };
	params.emitCount = m_CurrentSpawnCount;

	Math::Vector3 dir = Math::Matrix3(m_EmitterTransform.GetRotation()).GetY();
	params.emitterDirection = { dir.GetX(), dir.GetY(), dir.GetZ() };
	params.deltaTime = dt;

	params.startColor = { s.startColor[0], s.startColor[1], s.startColor[2], s.startColor[3] };
	params.endColor   = { s.endColor[0],   s.endColor[1],   s.endColor[2],   s.endColor[3] };

	params.speedMin = s.speedMin;
	params.speedMax = s.speedMax;
	params.lifeTimeMin = s.lifeTimeMin;
	params.lifeTimeMax = s.lifeTimeMax;

	params.gravity = { s.gravity[0], s.gravity[1], s.gravity[2] };
	params.randomeSeed = m_FrameCount;

	params.dirSpread = s.dirSpread;
	params.posSpread = s.posSpread;

	// UI에서는 deg 값을 GPU에서는 radian으로 보도록 변환
	params.spinSpeedMin = DirectX::XMConvertToRadians(s.spinSpeedMin);
	params.spinSpeedMax = DirectX::XMConvertToRadians(s.spinSpeedMax);
	params.initAngleMin = DirectX::XMConvertToRadians(s.initAngleMin);
	params.initAngleMax = DirectX::XMConvertToRadians(s.initAngleMax);
	params.sizeMode = s.sizeMode;
	params.sizeMin = { s.sizeMin[0], s.sizeMin[1] ,s.sizeMin[2] };
	params.sizeMax = { s.sizeMax[0], s.sizeMax[1] ,s.sizeMax[2] };

	return params;
}
void GP::ParticleEmitter::Update(float dt, const ParticleSettings& s)
{
	m_SpawnAccumulator += s.spawnRate * dt;
	m_CurrentSpawnCount = (uint32_t)m_SpawnAccumulator; // 내림 처리 (소수점 스폰은 할 수 없으므로)
	m_SpawnAccumulator -= m_CurrentSpawnCount; // 다음 프레임에 넘겨줄 것
	m_FrameCount++;
}
