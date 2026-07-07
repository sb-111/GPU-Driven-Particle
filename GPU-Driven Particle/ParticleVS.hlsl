#include "ParticleShared.h"

cbuffer VSConstants : register(b0)
{
	float4x4 g_ViewProj;
}
StructuredBuffer<Particle> g_ParticleBuffer : register(t0);
ByteAddressBuffer NewAliveList : register(t1);
ByteAddressBuffer Counters : register(t2);



//struct VSInput
//{
	//float3 pos : POSITION;
//};
struct VSOutput
{
	float4 pos : SV_Position;
};
VSOutput main(uint vid : SV_VertexID)
{
	VSOutput output;
	uint aliveCount = Counters.Load(COUNTER_AFTER_SIMULATE);
	if (vid >= aliveCount)
	{
		output.pos = float4(0, 0, 0, 0);
		return output;
	}
	uint index = NewAliveList.Load(vid * 4);
	output.pos = mul(g_ViewProj, float4(g_ParticleBuffer[index].position, 1.0f));
	return output;
}
