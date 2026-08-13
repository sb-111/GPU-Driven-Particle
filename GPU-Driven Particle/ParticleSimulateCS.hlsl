#include "ParticleShared.h"
#include "Quaternion.hlsli"
#include "SDFCollision.hlsli"
#include "CurlNoise.hlsli"
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
StructuredBuffer<MorphTargetPoint> MorphTargets : register(t64);

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
		const uint surfaceForceFlags = FORCE_AVOID | FORCE_TANGENT | FORCE_CURL;
		const bool usesSurfaceForces = (params.force.flags & surfaceForceFlags) != 0;
		const float surfaceInfluenceRadius = max(params.force.surfaceInfluenceRadius, 1e-4f);
		SDFQueryResult sceneQuery;
		bool isNearSurface = false;
		float3 surfaceNormal = 0.0f;

		// normal은 표면 영향권 내에서만 계산
		if (usesSurfaceForces)
		{
			sceneQuery = QuerySceneSDF(p.position);
			isNearSurface = sceneQuery.distance < surfaceInfluenceRadius;

			if (isNearSurface)
			{
				surfaceNormal = QueryColliderNormal(
					sceneQuery.colliderType,
					sceneQuery.colliderIndex,
					p.position);

				const float surfaceWeight = saturate(
					1.0f - sceneQuery.distance / surfaceInfluenceRadius); // 표면 1 -> 영향 반경 경계 0
				float3 force = 0.0f;
				if(params.force.flags & FORCE_AVOID)
					force += surfaceNormal * surfaceWeight * params.force.avoidStrength;
				if(params.force.flags & FORCE_TANGENT)
				{
					// tangentStrength: 목표 속력(m/s)
					const float K_STEER = 4.0f;
					// 목표 속도: 표면 법선과 지정 축이 평행 -> 0
					float3 tangentVelocityTarget = cross(surfaceNormal, params.force.tangentAxis) * params.force.tangentStrength;
					float3 steer = tangentVelocityTarget - p.velocity;
					// 법선 성분 제거 (법선 성분은 attract랑 충돌이 담당)
					steer -= surfaceNormal * dot(surfaceNormal, steer);
					p.velocity += steer * (1.0f - exp(-K_STEER * surfaceWeight * params.deltaTime));
				}
				p.velocity += force * params.deltaTime;
			}
		}

		// attract는 지정한 Target SDF만 사용
		if ((params.force.flags & FORCE_ATTRACT) &&
			params.force.attractTargetSDF < collisionParams.activeSDFCount)
		{
			int i = params.force.attractTargetSDF;
			float3 localPos = mul(collisionParams.sdfInstances[i].worldToLocal, float4(p.position, 1.0f)).xyz;
			float3 uvw = (localPos - collisionParams.sdfInstances[i].localBoundsMin) / (collisionParams.sdfInstances[i].localBoundsMax - collisionParams.sdfInstances[i].localBoundsMin);
			bool inside = all(uvw >= 0.0f) && all(uvw <= 1.0f);
			if(inside)
			{
				float distanceToTarget = QueryColliderDistance(COLLIDER_TYPE_SDF, params.force.attractTargetSDF, p.position);
				float3 targetNormal = QueryColliderNormal(COLLIDER_TYPE_SDF, params.force.attractTargetSDF, p.position);
				p.velocity += -distanceToTarget * targetNormal * params.force.attractStrength * params.deltaTime;
				float dampingRate = 2.0f * sqrt(params.force.attractStrength); // 감쇠율
				float normalSpeed = dot(p.velocity, targetNormal); // 법선 투영 길이
				p.velocity -= targetNormal * normalSpeed * (1.0f - exp(-dampingRate * params.deltaTime));
			}
			else
			{
				float3 localCenter = 0.5f * (collisionParams.sdfInstances[i].localBoundsMin + collisionParams.sdfInstances[i].localBoundsMax);
				float3 pullDirLocal = normalize(localCenter - localPos);
				float3 pullDir = normalize(mul(transpose((float3x3) collisionParams.sdfInstances[i].worldToLocal), pullDirLocal));

				float pullDistance = 0.5f * length(collisionParams.sdfInstances[i].localBoundsMax - collisionParams.sdfInstances[i].localBoundsMin) * collisionParams.sdfInstances[i].uniformScale;

				p.velocity += pullDir * pullDistance * params.force.attractStrength * params.deltaTime;

				float dampingRate = 2.0f * sqrt(params.force.attractStrength);
				float pullSpeed = dot(p.velocity, pullDir);
				p.velocity -= pullDir * pullSpeed * (1.0f - exp(-dampingRate * params.deltaTime));
			}
			
		}

		if (params.force.flags & FORCE_CURL)
		{
			// 시간에 따른 noise 좌표 이동량
			// noise pattern의 월드 이동 방향은 이 값의 반대
			const float3 noiseAdvection = float3(0.1f, 0.3f, 0.07f);
			// procedural noise 함수의 어느 좌표를 읽을지
			const float3 noisePosition =
				p.position * params.force.curlFrequency + noiseAdvection * params.totalTime;

			float3 curlVelocity;
			if (params.force.flags & FORCE_CURL_PSI)
			{
				// 논문 방식: 경계 보정을 ψ에서 처리
				curlVelocity = CurlNoisePsiBoundary(noisePosition, p.position,
		                              params.force.curlFrequency, surfaceInfluenceRadius);
			}
			else
			{
				// v = ∇ × ψ, 경계 보정 전에는 ∇·v ≈ 0
				curlVelocity = CurlNoise(noisePosition);
				if (isNearSurface)
				{
					// 현재 방식: curl 후 경계 보정을 v에서 처리
					// 표면 근처에서 접선 성분만 남음 -> 발산 0 엄밀 보장 X
					const float boundaryBlend = saturate(sceneQuery.distance / surfaceInfluenceRadius);
					curlVelocity -= (1.0f - boundaryBlend) * surfaceNormal * dot(surfaceNormal, curlVelocity);
				}
			}

			const float3 targetVelocity = curlVelocity * params.force.curlTargetSpeed;
			const float response = 1.0f - exp(-params.force.curlResponseRate * params.deltaTime);
			p.velocity += (targetVelocity - p.velocity) * response;
		}
		if(params.morph.enabled != 0 && params.morph.targetCount > 0)
		{
			uint targetIndex = index % params.morph.targetCount;
			float3 localTarget = MorphTargets[targetIndex].position;
			float3 worldTarget = mul(params.morph.targetToWorld, float4(localTarget, 1.0f)).xyz;
			float3 delta = worldTarget - p.position;
			float k = max(params.morph.strength, 0.0f);
			float damping = 2.0f * sqrt(k);
			float3 acceleration = delta * k - p.velocity * damping;
			p.velocity += acceleration * params.deltaTime;
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
