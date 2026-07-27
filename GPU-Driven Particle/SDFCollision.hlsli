#ifndef SDF_COLLISION_HLSLI
#define SDF_COLLISION_HLSLI

#include "ParticleShared.h"

cbuffer CollisionCB : register(b2)
{
	ParticleCollisionCB collisionParams;
}
float SampleSDF(float3 worldPos, out float3 normal)
{
	// 구에 대한 거리값 반환
	// TODO: 진짜 볼륨 텍스처로 변경

	float d = 1e30f;
	normal = float3(0.0f, 1.0f, 0.0f);

	if (collisionParams.colliderMask & COLLISION_PLANE)
	{
		// dot(p, n) - h
		float dp = dot(worldPos, collisionParams.collisionPlane.xyz) - collisionParams.collisionPlane.w;
		if(dp < d)
		{
			// 표면 넘어갔으면
			d = dp;
			normal = collisionParams.collisionPlane.xyz;
		}
	}
	if (collisionParams.colliderMask & COLLISION_SPHERE)
	{
		
		float ds = length(worldPos - collisionParams.collisionSphere.xyz) - collisionParams.collisionSphere.w;
		if(ds < d)
		{
			d = ds;
			normal = normalize(worldPos - collisionParams.collisionSphere.xyz);
		}
	}
	return d;
	
}
void ApplySDFCollision(inout float3 pos, inout float3 velocity, float restitution, float friction)
{
	// 표면까지 거리 샘플
	float3 n;
	float d = SampleSDF(pos, n);
	if (d >= 0.0f)
		return;

	pos -= n * d; // d만큼 normal 방향 복귀

	// 튕겨져 나가는 중이면 리턴
	float vn = dot(velocity, n);
	if (vn >= 0.0f)
		return;

	float3 velN = vn * n;
	float3 velT = velocity - velN;

	velocity = velT * (1.0f - friction) - velN * restitution;
}
#endif
