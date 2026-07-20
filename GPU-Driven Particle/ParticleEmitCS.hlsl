#include "ParticleShared.h"
#include "Quaternion.hlsli"
cbuffer ParticleCB : register(b0)
{
	ParticleFrameCB params;
};
RWStructuredBuffer<Particle> g_ParticleBuffer : register(u0);
RWByteAddressBuffer AliveList1 : register(u1);
RWByteAddressBuffer DeadList : register(u3);
RWByteAddressBuffer Counters : register(u4);

uint wang_hash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

float rand01(inout uint seed)     // 호출할 때마다 seed가 굴러감
{
	seed = wang_hash(seed);
	return seed / 4294967295.0; // uint 최대값으로 나눔 → [0,1)
}

[numthreads(64, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
	// Counters에서 진짜 방출량 꺼내오기 (this frame)
	uint realEmitCount = Counters.Load(COUNTER_REAL);
	if (id.x >= realEmitCount) // guard
		return;

	// Dead list consume
	uint prevDead;
	Counters.InterlockedAdd(COUNTER_DEAD, -1, prevDead); // dead count 감소

	if (prevDead == 0)
	{
		uint dummy;
		Counters.InterlockedAdd(4, 1, dummy);
		return;
	}

	// 해당 스레드의 인덱스 번호
	uint poolIndex = DeadList.Load((prevDead - 1) * 4); // 맨 위 Dead

	uint seed = id.x * 1973 + params.randomeSeed * 9277 + 1;
	float3 offset = float3(rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0);
	float3 spread = float3(rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0);

	Particle p = (Particle)0;
	switch (params.shapeType)
	{
		case POINT_TYPE:
		case BOX_TYPE:
			p.position = params.emitterPosition + offset * params.shapeData;
			break;
		case SPHERE_TYPE:
		default:
		p.position = params.emitterPosition + offset * params.posSpread;
			break;
	}
	// TODO
	switch (params.velocityMode)
	{
		case VELOCITY_MODE:
			p.velocity = normalize(params.emitterDirection + spread * params.dirSpread) *
		lerp(params.speedMin, params.speedMax, rand01(seed));
			break;
		case VELOCITY_FROM_POINT_MODE:
		case VELOCITY_IN_CONE_MODE:
		default:
			p.velocity = normalize(params.emitterDirection + spread * params.dirSpread) *
		lerp(params.speedMin, params.speedMax, rand01(seed));
			break;

	}
	p.lifeTime = lerp(params.lifeTimeMin, params.lifeTimeMax, rand01(seed));
	p.initialLife = p.lifeTime;
	// TODO: 랜덤 사용시 0.6~1.0 랜덤 구간 설정할 수 있도록 변경
	float4 randomColor = float4(params.startColor.rgb * lerp(0.6f, 1.0f, rand01(seed)), params.startColor.a);
	p.color = params.useRandomSpawnBrightness ?
		randomColor : params.startColor;
	p.spinSpeed = lerp(params.spinSpeedMin, params.spinSpeedMax, rand01(seed));
	p.angle.z = lerp(params.initAngleMin, params.initAngleMax, rand01(seed));
	// 스레드별로 다른 모드가 아니므로 다른 분기 안탐
	float3 uniformSizeMin = float3(params.sizeMin.x, params.sizeMin.x, params.sizeMin.x);
	float3 uniformSizeMax = float3(params.sizeMax.x, params.sizeMax.x, params.sizeMax.x);
	switch (params.sizeMode)
	{
		case UNIFORM_MODE:
			p.size = uniformSizeMin;
			break;
		case RANDOM_UNIFORM_MODE:
			p.size = lerp(uniformSizeMin, uniformSizeMax, rand01(seed));
			break;
		case NON_UNIFORM_MODE:
			p.size = params.sizeMin;
			break;
		case RANDOM_NON_UNIFORM_MODE:
			p.size = lerp(params.sizeMin, params.sizeMax, rand01(seed));
			break;
		default: // Uniform fallback
			p.size = uniformSizeMin;
			break;
	}

	// ======= mesh renderer가 소비 (태어날 때 orientation, angularVelocity 정해야 함) =======

	// orientation -> random이면 랜덤 축에 대해서 랜덤 각도만큼 회전, 아니면 항등
	float3 randomAxisForOrientation = float3(rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0);
	float3 normalizedRandomAxisForOrientation = normalize(randomAxisForOrientation);
	float randomAngle = rand01(seed) * PI;
	p.orientation = params.useRandomInitOrientation ? QuatFromAxisAngle(normalizedRandomAxisForOrientation, randomAngle) : float4(0.0f, 0.0f, 0.0f, 1.0f);
	// 각속도의 축은 랜덤 유무 정할 수 있음, 회전 속도는 UI에서 min/max로부터 랜덤
	float3 randomAxisForAngularVelocity = float3(rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0,
	                       rand01(seed) * 2.0 - 1.0);
	float3 normalizedRandomAxisForAngularVelocity = normalize(randomAxisForAngularVelocity);
	float3 rotationAxis = params.useRandomAxis ?  normalizedRandomAxisForAngularVelocity : params.rotationAxis; // 축 정하기
	rotationAxis = normalize(rotationAxis);
	float randomSpeed = lerp(params.rotationRateMin, params.rotationRateMax, rand01(seed)); // 속력 정하기
	p.angularVelocity = rotationAxis * randomSpeed; // 각속도 = 축 * 속력
	
	// 풀의 빈 인덱스에 새 파티클 덮어쓰기
	g_ParticleBuffer[poolIndex] = p;

	// alive list에 추가
	uint prevAlive;
	Counters.InterlockedAdd(COUNTER_ALIVE, 1, prevAlive);
	AliveList1.Store(prevAlive * 4, poolIndex);
}
