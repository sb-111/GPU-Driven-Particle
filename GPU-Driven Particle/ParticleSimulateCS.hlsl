#include "ParticleShared.h"
#include "Quaternion.hlsli"
#include "SDFCollision.hlsli"
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
		if(params.force.flags != 0)
		{
			SDFQueryResult q = QuerySceneSDF(p.position);
			// force 범위 내에 있으면
			if(q.distance < params.force.influenceRadius)
			{
				float3 N = QueryColliderNormal(q.colliderType, q.colliderIndex, p.position);
				float w = saturate(1.0f - q.distance / params.force.influenceRadius); // 표면 1 -> 반경 밖 0
				float3 force = 0.0f;
				if(params.force.flags & FORCE_AVOID)
				{
					force += (N * w * params.force.avoidStrength);
				}
				if(params.force.flags & FORCE_TANGENT)
				{
					float3 upRef = abs(N.y) > 0.99 ? float3(1, 0, 0) : float3(0, 1, 0);
					force += (normalize(cross(N, upRef)) * w *params.force.tangentStrength);
				}
				if(params.force.flags & FORCE_ATTRACT)
				{
					// 지정 메시에 대해 쿼리 필요
					// 표면으로 밀어낼 필요
					// 표면에 도착한 속도로 진동하지 않도록 감쇠
					float d = QueryColliderDistance(COLLIDER_TYPE_SDF, params.force.attractTargetSDF, p.position);
					float3 N = QueryColliderNormal(COLLIDER_TYPE_SDF, params.force.attractTargetSDF, p.position);
					force += -d * N * params.force.attractStrength;
					p.velocity *= exp(-2.0f * sqrt(params.force.attractStrength) * params.deltaTime); // 감쇠

				}
				p.velocity += force * params.deltaTime;
			}
		}
		if(params.collisionEnabled != 0)
		{
			float radius = 0.5f * max(p.size.x, max(p.size.y, p.size.z)); // 최장축을 반경으로
			float frametimeBudget = params.deltaTime; // 이번 프레임 시간 예산
			// 매 반복 SDF에 질의한 d값 만큼 이동하고, 이동한 시간을 예산에서 차감
			// 기존 step 방식은 스텝 수에 상한을 걸어서 속도가 상한을 넘으면 스텝 크기가 다시 증가해 통과 가능했음
			// 이 방식은 스텝 크기가 표면까지 거리 이하로 강제되어 속도가 빨라도 통과 불가
			// 상한에 걸리면 남은 이동 포기
			for (uint s = 0; s < 16 && frametimeBudget > 0.0f; ++s)
			{
				SDFQueryResult q = QuerySceneSDF(p.position);
				float collisionThreshold = radius + 0.5f * q.voxelSize;
				float safe = q.distance - collisionThreshold; // 표면까지 안전하게 갈 수 있는 이동거리

				// 침투 상태: 밀어낸 후 반사
				if (safe < 0.0f)
				{
					ResolveCollision(p.position, p.velocity, q,
						collisionThreshold, params.restitution, params.friction);
					safe = 0.0f; // 밀어낸 직후엔 다시 재질의 필요없이 0.0f 확정
				}

				// 속력
				float speed = length(p.velocity);
				if(speed < 1e-6f)
					break; // 속력이 없는 파티클은 이동 불가

				// 스텝 거리
				// speed * frameTimeBudget = 남은 시간 동안 이 속력으로 갈 수 있는 최대 거리
				// max(safe, 1복셀) = 표면 안넘음 보장 거리
				float stepDist = min(speed * frametimeBudget,				// 가야 하는 거리
										max(safe, 1.0f * q.voxelSize));	// 가도 되는 거리

				// 속도 방향으로 스텝 거리만큼 이동
				p.position += (p.velocity / speed) * stepDist;
				
				// 쓴 시간 = 스텝 거리 / 속력
				frametimeBudget -= stepDist / speed;
			}
		}
		else
		{
			p.position += p.velocity * params.deltaTime;
		}
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
		if (params.keyMode == SORT_KEY_DEPTH)
		{
			SortKeys[prevAlive] = -length(viewParams.camPos - p.position); // 거리에 -를 붙여야 정렬 후에 back to front
		}
		else
		{
			SortKeys[prevAlive] = p.initialLife - p.lifeTime;
		}
	}
	
}
