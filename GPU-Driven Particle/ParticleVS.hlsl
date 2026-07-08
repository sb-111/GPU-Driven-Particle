#include "ParticleShared.h"

cbuffer VSConstants : register(b0)
{
	float4x4 g_ViewProj;
}
StructuredBuffer<Particle> g_ParticleBuffer : register(t0);
ByteAddressBuffer NewAliveList : register(t1);

struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
};
VSOutput main(uint vid : SV_VertexID)
{
	VSOutput output;

	uint index = NewAliveList.Load(vid * 4);
	output.pos = mul(g_ViewProj, float4(g_ParticleBuffer[index].position, 1.0f));
	output.color = g_ParticleBuffer[index].color;
	return output;
}
