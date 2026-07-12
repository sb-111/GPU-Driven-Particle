#include "ParticleShared.h"

static const float2 QuadVert[6] =
{
	float2(-1, -1), float2(1, 1), float2(-1, 1), // 좌하→우상→좌상 (CCW)
    float2(-1, -1), float2(1, -1), float2(1, 1) // 좌하→우하→우상 (CCW)
};

cbuffer VSConstants : register(b0)
{
	float4x4 g_ViewProj;
	float3 g_CamRight;
	float g_ParticleSize;
	float3 g_CamUp;
	float pad0;
}
cbuffer ParticleParams : register(b1)
{
	ParticleFrameCB frameParams;
}
StructuredBuffer<Particle> g_ParticleBuffer : register(t0);
ByteAddressBuffer NewAliveList : register(t1);

struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};
// 입력: 정점 id
VSOutput main(uint vid : SV_VertexID, uint iid: SV_InstanceID)
{
	// Alive 접근 : Instance id를 통해 접근
	uint index = NewAliveList.Load(iid * 4);
	Particle p = g_ParticleBuffer[index];

	float t = 1.0f - (p.lifeTime / p.initialLife); // 0 -> 1
	float fade = 1.0f - t; // 1 -> 0 (처음이 밝도록)

	float size = g_ParticleSize * (1.0f - t*t);
	
	// Instance 내 정점 접근
	float2 corner = QuadVert[vid];
	float3 worldPos = p.position + (g_CamRight * corner.x + g_CamUp * corner.y) * size;

	
	VSOutput output;
	output.pos = mul(g_ViewProj, float4(worldPos, 1.0f));
	output.color.rgb = lerp(p.color.rgb, frameParams.endColor.rgb, t);
	output.color.rgb *= fade;
	output.color.a = p.color.a;
	output.uv = (corner * 0.5f + 0.5f);
	output.uv.y = 1 - output.uv.y;

	
	return output;
}
