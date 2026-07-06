#include "ParticleShared.h"

RWStructuredBuffer<Particle> g_Particles : register(u0); // UAV
cbuffer CB : register(b0)
{
	float deltaTime;
	uint particleCount;
};

[numthreads(64,1,1)]
void main(uint3 id : SV_DispatchThreadID)
{
	// 파티클 개수 넘게 접근하지 않도록 가드
	if (id.x >= particleCount)
		return;
	
	g_Particles[id.x].position += g_Particles[id.x].velocity * deltaTime;
}
