#include "ParticleShared.h"
static const float2 QuadVert[6] =
{
	float2(-1, -1), float2(1, 1), float2(-1, 1), // 좌하→우상→좌상 (CCW)
    float2(-1, -1), float2(1, -1), float2(1, 1) // 좌하→우하→우상 (CCW)
};
cbuffer ViewCB : register(b0)
{
	ParticleViewCB viewParams;
}
cbuffer FrameCB : register(b1)
{
	ParticleFrameCB frameParams;
}
cbuffer DrawCB : register(b2)
{
	ParticleDrawCB drawParams;
}
StructuredBuffer<Particle> ParticlePool : register(t0);
ByteAddressBuffer NewAliveList : register(t1);
ByteAddressBuffer Counters : register(t3);

struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

// 인스턴스 수 = 파티클 수, 인스턴스 i = 파티클 i와 i+1 이은 쿼드 생성
VSOutput main(uint vid: SV_VertexID, uint iid: SV_InstanceID)
{
	VSOutput output = (VSOutput) 0;
	uint aliveCount = Counters.Load(COUNTER_AFTER_SIMULATE);
	// 쿼드 조립: iid, iid+1 파티클 가져와서 쿼드 한장 조립
	uint prevIndex = NewAliveList.Load(iid * 4);
	uint currentIndex = NewAliveList.Load(min(iid + 1, aliveCount - 1) * 4);
	
	Particle prevParticle = ParticlePool[prevIndex];
	Particle currentParticle = ParticlePool[currentIndex];

	// 두 파티클을 잇는 벡터 (접선)
	float3 tangentVector = (currentParticle.position - prevParticle.position);
	// 카메라 시선과 접선의 수직 (단위벡터)
	float3 c = cross(viewParams.camForward, tangentVector);
	// 외적 결과가 0벡터인 경우 처리
	float3 widthVector = (dot(c, c) < 1e-6f) ? float3(0.0f, 0.0f, 0.0f) : normalize(c);
	// 각 폭 구하기
	float prevHalfWidth = prevParticle.size.x / 2.0f;
	float currentHalfWidth = currentParticle.size.x / 2.0f;

	//float3 v0 = prevParticle.position + prevHalfWidth * widthVector;
	//float3 v3 = prevParticle.position - prevHalfWidth * widthVector;
	//float3 v1 = currentParticle.position + currentHalfWidth * widthVector;
	//float3 v2 = currentParticle.position - currentHalfWidth * widthVector;

	// 조립할 월드상 정점 생성
	float3 localPos = float3(QuadVert[vid], 0.0f);
	float3 particlePos = (localPos.x < 0.0f) ? prevParticle.position : currentParticle.position;
	float halfWidth = (localPos.x < 0.0f) ? prevHalfWidth : currentHalfWidth;
	float3 worldPos = particlePos + halfWidth * localPos.y * widthVector;

	output.pos = mul(viewParams.viewProj, float4(worldPos, 1.0f));
	output.color = float4(1.0f, 0.0f, 0.0f, 1.0f);
	output.uv = QuadVert[vid] * 0.5f + 0.5f;
	output.uv.y = 1 - output.uv.y;
	return output;
}
