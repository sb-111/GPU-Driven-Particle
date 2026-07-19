#include "ParticleShared.h"
#include "Quaternion.hlsli"

cbuffer ParticleCB : register(b0)
{
	ParticleFrameCB params;
};
cbuffer ViewCB : register(b1)
{
	ParticleViewCB viewParams;
};
RWStructuredBuffer<Particle> g_ParticleBuffer : register(u0); // UAV
RWByteAddressBuffer AliveList1 : register(u1);
RWByteAddressBuffer AliveList2 : register(u2);
RWByteAddressBuffer DeadList : register(u3);
RWByteAddressBuffer Counters : register(u4);
RWStructuredBuffer<float> SortKeys : register(u6);

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
		p.velocity += params.gravity * params.deltaTime;
		p.position += p.velocity * params.deltaTime;

		// for mesh renderer
		float angularVelocityLength = length(p.angularVelocity);
		if(angularVelocityLength > 1e-6f) // 회전있는 파티클만 회전
		{
			float3 axis = p.angularVelocity / angularVelocityLength; // 회전 축 (각속도에서 속력 분리하고 남은 것)
			float angle = angularVelocityLength * params.deltaTime; // 이번 프레임에 돈 각도(rad)

			float4 dq = QuatFromAxisAngle(axis, angle);
			p.orientation = normalize(QuatMul(p.orientation, dq)); // dq 먼저(로컬 좌표일 때 적용)

		}
		
		g_ParticleBuffer[index] = p;

		uint prevAlive;
		Counters.InterlockedAdd(COUNTER_AFTER_SIMULATE, 1, prevAlive);
		AliveList2.Store(prevAlive * 4, index);
		SortKeys[prevAlive] = -length(viewParams.camPos - p.position); // 거리에 -를 붙여야 정렬 후에 back to front
	}
	
}
