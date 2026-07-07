#include "ParticleShared.h"

cbuffer ParticleCB : register(b0)
{
	EmitterCBParams params;
};
RWStructuredBuffer<Particle> g_ParticleBuffer : register(u0);
RWByteAddressBuffer AliveList1 : register(u1);
RWByteAddressBuffer DeadList : register(u2);
RWByteAddressBuffer Counters : register(u3);

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
	// 이번 프레임 방출량 이상 접근 가드
	if (id.x >= params.emitCount)
		return;

	// Dead list consume
	uint prevDead; 
	Counters.InterlockedAdd(4, -1, prevDead); // dead count 감소

	if(prevDead == 0)
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

	Particle p;
	p.position = params.emitterPosition + offset*0.1f;
	p.velocity = normalize(params.emitterDirection + spread * 0.3 +
	lerp(3.0f, 5.0f, rand01(seed)));
	p.lifeTime = lerp(params.minLifeTime, params.maxLifeTime, rand01(seed));

	// 풀의 빈 인덱스에 새 파티클 덮어쓰기
	g_ParticleBuffer[poolIndex] = p;

	// alive list에 추가
	uint prevAlive;
	Counters.InterlockedAdd(0, 1, prevAlive);
	AliveList1.Store(prevAlive * 4, poolIndex);
}
