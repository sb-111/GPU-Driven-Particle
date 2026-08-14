#ifndef SDF_COLLISION_HLSLI
#define SDF_COLLISION_HLSLI

#include "ParticleShared.h"

#define COLLIDER_TYPE_NONE   0
#define COLLIDER_TYPE_PLANE  1
#define COLLIDER_TYPE_SPHERE 2
#define COLLIDER_TYPE_SDF    3

struct SDFQueryResult
{
	float distance;	   // 월드 거리
	float voxelSize;   // 월드 복셀 사이즈
	uint colliderType; // COLLIDER_TYPE_NONE/PLANE/SPHERE/SDF
	uint colliderIndex;// SDF일 때만 의미
};

cbuffer CollisionCB : register(b2)
{
	ParticleCollisionCB collisionParams;
}
Texture3D<float> SDFTextures[MAX_SDF_COUNT] : register(t0);
StructuredBuffer<BVHNode> BVHNodes : register(t65);
SamplerState LinearClamp  : register(s0);


float dAABB(float3 p, float3 bmin, float3 bmax)
{
	// p : 파티클 월드 위치
	// bmin: 검사하려는 노드 박스의 min
	// bmax: 검사하려는 노드 박스의 max
	// 반환: 파티클에서 박스까지 거리, 박스 안이면 0
	return length(max(max(bmin - p, p - bmax), 0.0f));
}

// 인스턴스 하나에 대한 '월드 거리' 반환 헬퍼 (인스턴스 i까지 실제 거리를 넘지 않는 값 반환)
float QuerySDFInstanceDistance(uint i, float3 worldPos, float skipIfBeyond)
{
	float3 localPos = mul(collisionParams.sdfInstances[i].worldToLocal, float4(worldPos, 1.0f)).xyz;
	float3 closest = clamp(localPos, collisionParams.sdfInstances[i].localBoundsMin, collisionParams.sdfInstances[i].localBoundsMax);
	float outsideDist = length(localPos - closest);
	float dOut = outsideDist * collisionParams.sdfInstances[i].uniformScale;

	if (dOut >= skipIfBeyond)
		return dOut; // 바운드 거리가 skipIfBeyond 이상이면 샘플 생략

	float3 uvw = (closest - collisionParams.sdfInstances[i].localBoundsMin)
	/ (collisionParams.sdfInstances[i].localBoundsMax - collisionParams.sdfInstances[i].localBoundsMin);
	float sampled = SDFTextures[NonUniformResourceIndex(i)].SampleLevel(LinearClamp, uvw, 0)
					* collisionParams.sdfInstances[i].uniformScale;

	return (outsideDist > 0.0f) ? max(dOut, sampled - dOut) : sampled;
}
// 모든 콜라이더 중 최근접 결과 반환
SDFQueryResult QuerySceneSDF(float3 worldPos)
{
	SDFQueryResult result;
	result.distance = 1e30f;
	result.voxelSize = 0.05; // 해석적 콜라이더용 기본 값
	result.colliderType = COLLIDER_TYPE_NONE;
	result.colliderIndex = 0;

	if (collisionParams.colliderMask & COLLISION_PLANE)
	{
		float dp = dot(worldPos, collisionParams.collisionPlane.xyz)
				- collisionParams.collisionPlane.w;
		if (dp < result.distance)
		{
			result.distance = dp;
			result.colliderType = COLLIDER_TYPE_PLANE;
		}
	}
	if (collisionParams.colliderMask & COLLISION_SPHERE)
	{
		float ds = length(worldPos - collisionParams.collisionSphere.xyz)
				- collisionParams.collisionSphere.w;
		if (ds < result.distance)
		{
			result.distance = ds;
			result.colliderType = COLLIDER_TYPE_SPHERE;
		}
	}
	if (collisionParams.colliderMask & COLLISION_SDF)
	{
		if(collisionParams.useBVH != 0)
		{
			// BVH 순회
			uint stack[8]; // 값: 노드 번호
			int top = 0;
			stack[top++] = 0;
			while(top > 0)
			{
				BVHNode node = BVHNodes[stack[--top]];
				// 파티클 월드 위치~노드 바운드 거리가 기존 최소 보다 크면 밑의 노드 검사 생략
				if(dAABB(worldPos, node.boundsMin, node.boundsMax) >= result.distance)
					continue;
				// 리프라면
				if(node.right == 0xFFFFFFFF)
				{
					uint i = node.left; // 콜라이더 번호
					float dInst = QuerySDFInstanceDistance(i, worldPos, result.distance);
					if(dInst < result.distance)
					{
						result.distance = dInst;
						result.voxelSize = collisionParams.sdfInstances[i].worldVoxelSize;
						result.colliderType = COLLIDER_TYPE_SDF;
						result.colliderIndex = i;
					}
				}
				else
				{
					uint nearChild = node.left;
					uint farChild = node.right;

					float dNear = dAABB(worldPos, BVHNodes[nearChild].boundsMin, BVHNodes[nearChild].boundsMax);
					float dFar = dAABB(worldPos, BVHNodes[farChild].boundsMin, BVHNodes[farChild].boundsMax);

					if (dNear > dFar)
					{
						uint tmpIndex = nearChild;
						nearChild = farChild;
						farChild = tmpIndex;
						float tmpDist = dNear;
						dNear = dFar;
						dFar = tmpDist;
					}

					// 먼 쪽을 먼저 push → 나중에 pop
					if (dFar < result.distance)
						stack[top++] = farChild;
					// 가까운 쪽을 나중에 push → 먼저 pop
					if (dNear < result.distance)
						stack[top++] = nearChild;
				}

			}
		}
		else
		{
			// 브루트포스
			for (uint i = 0; i < collisionParams.activeSDFCount; ++i)
			{
				float dInst = QuerySDFInstanceDistance(i, worldPos, result.distance);
				if(dInst < result.distance)
				{
					result.distance = dInst;
					result.voxelSize = collisionParams.sdfInstances[i].worldVoxelSize;
					result.colliderType = COLLIDER_TYPE_SDF;
					result.colliderIndex = i;
				}
			}
		}
	}
	return result;
}
// 단일 콜라이더에 대한 거리 반환
float QueryColliderDistance(uint type, uint index, float3 worldPos)
{
	switch (type)
	{
		case COLLIDER_TYPE_PLANE:
			return dot(worldPos, collisionParams.collisionPlane.xyz)
				- collisionParams.collisionPlane.w;
		case COLLIDER_TYPE_SPHERE:
			return length(worldPos - collisionParams.collisionSphere.xyz)
				- collisionParams.collisionSphere.w;
		case COLLIDER_TYPE_SDF:
			return QuerySDFInstanceDistance(index, worldPos, 1e30f); // 스킵없이 항상 샘플
		default:
			return 1e30f;
	}
}
// 단일 콜라이더에 대한 노말 반환
float3 QueryColliderNormal(uint type, uint index, float3 worldPos)
{
	switch (type)
	{
		case COLLIDER_TYPE_PLANE:
			return collisionParams.collisionPlane.xyz;
		case COLLIDER_TYPE_SPHERE:
			return normalize(worldPos - collisionParams.collisionSphere.xyz);
		case COLLIDER_TYPE_SDF:
		{
			float h = collisionParams.sdfInstances[index].worldVoxelSize;
			float3 gradient;
			// 중앙차분법
			gradient.x = QuerySDFInstanceDistance(index, worldPos + float3(h, 0, 0), 1e30f) - QuerySDFInstanceDistance(index, worldPos - float3(h, 0, 0), 1e30f);
			gradient.y = QuerySDFInstanceDistance(index, worldPos + float3(0, h, 0), 1e30f) - QuerySDFInstanceDistance(index, worldPos - float3(0, h, 0), 1e30f);
			gradient.z = QuerySDFInstanceDistance(index, worldPos + float3(0, 0, h), 1e30f) - QuerySDFInstanceDistance(index, worldPos - float3(0, 0, h), 1e30f);
			float len = length(gradient);
			return (len > 1e-5f) ? gradient / len : float3(0.0f, 1.0f, 0.0f); // 정규화
		}
		default:
			return float3(0.0f, 1.0f, 0.0f);
	}
}
// 밀어내기 + 반사
void ResolveCollision(inout float3 worldPos, inout float3 velocity,
	SDFQueryResult q, float collisionThreshold, float restitution, float friction)
{
	// 위치 보정 (침투한만큼 외부로 밀어내기)
	float3 normal = QueryColliderNormal(q.colliderType, q.colliderIndex, worldPos);
	worldPos -= normal * (q.distance - collisionThreshold); // normal은 표면 바깥 방향, 괄호 안은 음수
	
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
