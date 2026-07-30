#ifndef SDF_COLLISION_HLSLI
#define SDF_COLLISION_HLSLI

#include "ParticleShared.h"

cbuffer CollisionCB : register(b2)
{
	ParticleCollisionCB collisionParams;
}
Texture3D<float> SDFTextures[MAX_SDF_COUNT] : register(t0);
SamplerState LinearClamp  : register(s0);
float SampleSDF(float3 worldPos, out float gradEps)
{
	float d = 1e30f;
	gradEps = 0.05f;

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
			float3 uvw = (localPos - collisionParams.sdfInstances[i].localBoundsMin)
				/ (collisionParams.sdfInstances[i].localBoundsMax - collisionParams.sdfInstances[i].localBoundsMin);

			// 바운드 밖 스킵
			if (any(uvw < 0.0f) || any(uvw > 1.0f))
				continue;

			// 로컬 공간 거리 값을 월드 거리로 변경
			float dSdf = SDFTextures[i].SampleLevel(LinearClamp, uvw, 0) * collisionParams.sdfInstances[i].uniformScale;
			if(dSdf < d)
			{
				d = dSdf;
				gradEps = collisionParams.sdfInstances[i].gradientEpsilon;
			}
		}
	}
	return d;
	
}
// 해당 위치에서 노말 방향 반환
float3 SampleSDFGradient(float3 worldPos, float h)
{
	float unusedEps;
	float3 gradient;
	// 중앙차분법
	gradient.x = SampleSDF(worldPos + float3(h, 0, 0), unusedEps) - SampleSDF(worldPos - float3(h, 0, 0), unusedEps);
	gradient.y = SampleSDF(worldPos + float3(0, h, 0), unusedEps) - SampleSDF(worldPos - float3(0, h, 0), unusedEps);
	gradient.z = SampleSDF(worldPos + float3(0, 0, h), unusedEps) - SampleSDF(worldPos - float3(0, 0, h), unusedEps);

	float len = length(gradient);
	return (len > 1e-5f) ? gradient / len : float3(0.0f, 1.0f, 0.0f);
}
void ApplySDFCollision(inout float3 worldPos, inout float3 velocity, float restitution, float friction)
{
	// 표면까지 거리 샘플
	float gradEps;
	float d = SampleSDF(worldPos, gradEps);

	// 외부에 있으면 충돌 처리 Pass
	if (d >= 0.0f)
		return;

	// 위치 보정 (외부로 밀어내기)
	float3 normal = SampleSDFGradient(worldPos, gradEps);
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
