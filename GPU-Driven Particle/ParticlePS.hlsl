#include "ParticleShared.h"
struct PSInput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};
Texture2D g_SpriteTex : register(t2);
SamplerState g_LinearClamp : register(s0);
cbuffer DrawCB : register(b2)
{
	ParticleDrawCB drawParams;
}

float4 main(PSInput input) : SV_Target
{
	float4 sampleColor = g_SpriteTex.Sample(g_LinearClamp, input.uv);
	float4 finalColor = sampleColor * input.color;
	if (drawParams.blendMode == BLEND_ADDITIVE_MODE)
	{
		// 가산일 때만 색에다 모양 반영
		finalColor.rgb *= sampleColor.a;
	}
	// 알파 모드는 모양이 이미 a채널에 있음.
	return finalColor;
}
