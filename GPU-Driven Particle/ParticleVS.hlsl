cbuffer VSConstants : register(b0)
{
	float4x4 g_ViewProj;
}
struct Particle
{
	float3 position;
};

StructuredBuffer<Particle> g_ParticleBuffer : register(t0);

struct VSInput
{
	float3 pos : POSITION;
};
struct VSOutput
{
	float4 pos : SV_Position;
};
VSOutput main(uint vid : SV_VertexID)
{
	VSOutput output;
	output.pos = mul(g_ViewProj, float4(g_ParticleBuffer[vid].position, 1.0f));
	return output;
}
