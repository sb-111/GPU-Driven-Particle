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

	params.shapeType = s.shapeType;
	params.velocityMode = s.velocityMode;
	// Shape Type 지정에 따라 다른 데이터를 GPU로 보냄
	switch ((EShapeType)s.shapeType)
	{
	case EShapeType::Box:
		params.shapeData = { s.boxExtents[0], s.boxExtents[1], s.boxExtents[2] };
		break;
	case EShapeType::Sphere:
		params.shapeData = { s.sphereRadius, s.sphereSurfaceOnly ? 1.0f : 0.0f, 0.0f };
		break;
	default:
		break;
	}
	// TODO: 모드에 따라 파라미터 다르게 줄 수 있도록
	switch ((EVelocityMode)s.velocityMode)
	{
	case EVelocityMode::Velocity:
		params.coneAngle = s.coneAngle; break;
	case EVelocityMode::VelocityFromPoint:
	case EVelocityMode::VelocityInCone:
	default:
		params.coneAngle = s.coneAngle;
		break;
	}
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
	m_FrameCount++;

	if (!m_Active)
	{
		m_CurrentSpawnCount = 0;
		return;
	}

	// 이미터 나이 증가
	m_AgeInLoop += dt;
	if (m_AgeInLoop >= s.loopDuration)
	{
		// 지속시간 초과 시
		switch ((ELoopMode)s.loopMode)
		{
		case ELoopMode::Infinite:
			m_AgeInLoop -= s.loopDuration;
			m_CanBurst = true;
			break;
		case ELoopMode::Once:
			m_Active = false; // 비활성화
			break;
		case ELoopMode::Multiple:
			m_AgeInLoop -= s.loopDuration;
			if (++m_CompletedLoops < s.loopCount)
			{
				m_CanBurst = true;
			}
			else
			{
				m_Active = false;
			}
			break;
		default:
			break;
		}
	}

	m_SpawnAccumulator += s.spawnRate * dt;
	m_CurrentSpawnCount = (uint32_t)m_SpawnAccumulator; // 내림 처리 (소수점 스폰은 할 수 없으므로)
	m_SpawnAccumulator -= m_CurrentSpawnCount; // 다음 프레임에 넘겨줄 것

	if (m_CanBurst)
	{
		m_CurrentSpawnCount += s.burstCount;
		m_CanBurst = false;
	}

}

void GP::ParticleEmitter::ResetEmitter()
{
	m_AgeInLoop = 0.0f;
	m_Active = true;
	m_CompletedLoops = 0;
	m_CanBurst = true;
}
