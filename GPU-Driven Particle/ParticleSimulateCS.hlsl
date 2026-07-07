#include "ParticleShared.h"


cbuffer ParticleCB : register(b0)
{
	ParticleFrameCB params;
};
RWStructuredBuffer<Particle> g_ParticleBuffer : register(u0); // UAV
RWByteAddressBuffer AliveList1 : register(u1);
RWByteAddressBuffer AliveList2 : register(u2);
RWByteAddressBuffer DeadList : register(u3);
RWByteAddressBuffer Counters : register(u4);


[numthreads(64,1,1)]
void main(uint3 id : SV_DispatchThreadID)
{
	// Alive Counter 이상이면 리턴
	if (id.x >= Counters.Load(COUNTER_ALIVE))
		return;
	// 살아있는 파티클 가져오기
	uint index = AliveList1.Load(id.x * 4);
	Particle p = g_ParticleBuffer[index];

	p.lifeTime -= params.deltaTime;

	if(p.lifeTime <= 0.0f)
	{
		// Dead List에 인덱스 추가
		uint prevDead;
		Counters.InterlockedAdd(COUNTER_DEAD, 1, prevDead);
		DeadList.Store(prevDead * 4, index);
		return;
	}
	else
	{
		// 살아있으면 위치 변경
		p.velocity += float3(0, -9.8f, 0) * params.deltaTime;
		p.position += p.velocity * params.deltaTime;
		g_ParticleBuffer[index] = p;

		uint prevAlive;
		Counters.InterlockedAdd(COUNTER_AFTER_SIMULATE, 1, prevAlive);
		AliveList2.Store(prevAlive * 4, index);
	}
	
}
