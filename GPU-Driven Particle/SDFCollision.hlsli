#ifndef SDF_COLLISION_HLSLI
#define SDF_COLLISION_HLSLI

#include "ParticleShared.h"

cbuffer CollisionCB : register(b2)
{
	ParticleCollisionCB collisionParams;
}
Texture3D<float> SDFTextures[MAX_SDF_COUNT] : register(t0);
SamplerState LinearClamp  : register(s0);
float SampleSDF(float3 worldPos)
{
	float d = 1e30f;

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
			// 살아있는 SDF까지만 루프

			// 샘플링하여 표면까지 최단 거리 구하기
			// worldPos -> 볼륨 내 0~1 구간으로 변환
			float3 uvw = (worldPos - collisionParams.sdfInstances[i].boundsMin)
				/ (collisionParams.sdfInstances[i].boundsMax - collisionParams.sdfInstances[i].boundsMin);
			float dSdf = SDFTextures[i].SampleLevel(LinearClamp, uvw, 0);
			if(dSdf < d)
			{
				d = dSdf;
			}
		}
	}
	return d;
	
}
// 해당 위치에서 노말 방향 반환
float3 SampleSDFGradient(float3 worldPos)
{
	const float h = 0.05f;
	float3 gradient;
	// 중앙차분법
	gradient.x = SampleSDF(worldPos + float3(h, 0, 0)) - SampleSDF(worldPos - float3(h, 0, 0));
	gradient.y = SampleSDF(worldPos + float3(0, h, 0)) - SampleSDF(worldPos - float3(0, h, 0));
	gradient.z = SampleSDF(worldPos + float3(0, 0, h)) - SampleSDF(worldPos - float3(0, 0, h));

	// 정규화
	gradient = normalize(gradient);

	return gradient;
}
void ApplySDFCollision(inout float3 worldPos, inout float3 velocity, float restitution, float friction)
{
	// 표면까지 거리 샘플
	float d = SampleSDF(worldPos);
	
	// 외부에 있으면 충돌 처리 Pass
	if (d >= 0.0f)
		return;

	// 위치 보정 (외부로 밀어내기)
	float3 normal = SampleSDFGradient(worldPos);
	worldPos -= normal * d; // normal은 표면 바깥 방향, d는 음수
	
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
