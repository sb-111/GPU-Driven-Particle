#ifndef SDF_COLLISION_HLSLI
#define SDF_COLLISION_HLSLI

#include "ParticleShared.h"

cbuffer CollisionCB : register(b2)
{
	ParticleCollisionCB collisionParams;
}
Texture3D<float> SDFTextures[MAX_SDF_COUNT] : register(t0);
SamplerState LinearClamp  : register(s0);

// plane, sphere는 수식 계산
// SDF 볼륨 내부 = 텍스처 샘플
// 볼륨 밖 = 하한 값 추정
// 모든 콜라이더 중 최솟값 선택
float QuerySceneDistance(float3 worldPos, out float worldVoxelSize)
{
	float d = 1e30f;
	worldVoxelSize = 0.05f; // 해석적 콜라이더용 기본값 (법선 샘플 간격)

	if (collisionParams.colliderMask & COLLISION_PLANE)
	{
		// dot(p, n) - h
		float dp = dot(worldPos, collisionParams.collisionPlane.xyz) - collisionParams.collisionPlane.w;
		if(dp < d)
		{
			// 표면 넘어갔으면
			d = dp;
		}
	}
	if (collisionParams.colliderMask & COLLISION_SPHERE)
	{
		
		float ds = length(worldPos - collisionParams.collisionSphere.xyz) - collisionParams.collisionSphere.w;
		if(ds < d)
		{
			d = ds;
		}
	}
	if(collisionParams.colliderMask & COLLISION_SDF)
	{
		for (uint i = 0; i < collisionParams.activeSDFCount; ++i)
		{
			float3 localPos = mul(collisionParams.sdfInstances[i].worldToLocal, float4(worldPos, 1.0f)).xyz;
			float3 closest = clamp(localPos, collisionParams.sdfInstances[i].localBoundsMin, collisionParams.sdfInstances[i].localBoundsMax);
			float outsideDist = length(localPos - closest); // 0보다 크면 바깥에 존재했던 것
			// 바운드까지 거리
			float dOut = outsideDist * collisionParams.sdfInstances[i].uniformScale;

			// dInst >= dOut이므로 dOut이 이미 현재 d 이상이면 min 경쟁에서 못 이김 → 샘플 생략
			if (dOut >= d)
				continue;

			// closest는 바운드 안이므로 uvw[0,1] 보장
			float3 uvw = (closest - collisionParams.sdfInstances[i].localBoundsMin)
			/ (collisionParams.sdfInstances[i].localBoundsMax - collisionParams.sdfInstances[i].localBoundsMin);
			// closest의 샘플 값
			float sampled = SDFTextures[i].SampleLevel(LinearClamp, uvw, 0) * collisionParams.sdfInstances[i].uniformScale;

			// 안: 샘플값 그대로 / 밖: 두 하한(바운드까지 거리, 경계샘플-바운드거리) 중 큰 쪽
			float dInst = (outsideDist > 0.0f) ? max(dOut, sampled - dOut) : sampled;
			if(dInst < d)
			{
				d = dInst;
				worldVoxelSize = collisionParams.sdfInstances[i].worldVoxelSize;
			}
		}
	}
	return d;
}
// 해당 위치에서 노말 방향 반환
float3 QuerySceneNormal(float3 worldPos, float h)
{
	float unusedVoxelSize;
	float3 gradient;
	// 중앙차분법
	gradient.x = QuerySceneDistance(worldPos + float3(h, 0, 0), unusedVoxelSize) - QuerySceneDistance(worldPos - float3(h, 0, 0), unusedVoxelSize);
	gradient.y = QuerySceneDistance(worldPos + float3(0, h, 0), unusedVoxelSize) - QuerySceneDistance(worldPos - float3(0, h, 0), unusedVoxelSize);
	gradient.z = QuerySceneDistance(worldPos + float3(0, 0, h), unusedVoxelSize) - QuerySceneDistance(worldPos - float3(0, 0, h), unusedVoxelSize);

	float len = length(gradient);
	return (len > 1e-5f) ? gradient / len : float3(0.0f, 1.0f, 0.0f);
}

// 침투한 경우에 대해 처리하는 함수
// 밀어내기 + 반사
void ResolveCollision(inout float3 worldPos, inout float3 velocity,
	float d, float collisionThreshold, float worldVoxelSize,
	float restitution, float friction)
{
	// 위치 보정 (침투한만큼 외부로 밀어내기)
	float3 normal = QuerySceneNormal(worldPos, worldVoxelSize);
	worldPos -= normal * (d - collisionThreshold); // normal은 표면 바깥 방향, 괄호 안은 음수
	
	// 이미 튕겨져 나가는 방향이면 반사 X
	float velNLength = dot(velocity, normal);
	if (velNLength >= 0.0f)
		return;

	// 물리 법칙 적용 (속도의 수평, 수직 성분 분해)
	float3 velN = velNLength * normal; // 아래 방향
	float3 velT = velocity - velN;

	velocity = velT * saturate(1.0f - friction) - velN * restitution;
}
#endif
