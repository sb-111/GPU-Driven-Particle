#include "ParticleShared.h"
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
Texture2D SpriteTex : register(t2);
ByteAddressBuffer Counters : register(t3);
SamplerState LinearClamp : register(s0);
struct PSInput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};
float4 main(PSInput input) : SV_TARGET
{
	float4 sampleColor = SpriteTex.Sample(LinearClamp, input.uv);
	float4 finalColor = sampleColor * input.color;
	if (drawParams.blendMode == BLEND_ADDITIVE_MODE)
	{
		finalColor.rgb *= finalColor.a; // 페이드 및 모양
	}
	return finalColor;
}
