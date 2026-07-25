#include "ParticleShared.h"
struct PSInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};
cbuffer DrawCB : register(b2)
{
	ParticleDrawCB drawParams;
}

float4 main(PSInput input) : SV_TARGET
{
	float3 N = normalize(input.normal);
	float3 L = normalize(float3(0.4f, 1.0f, 0.2f));
	float nDotL = dot(N, L);
	float bright = saturate(nDotL) * 0.8f + 0.25f;
	float4 finalColor = float4(bright * input.color.rgb, input.color.a); // a = 수명 페이드
	finalColor.rgb *= finalColor.a; // pre-multiplied
	if (drawParams.blendMode == BLEND_ADDITIVE_MODE)
	{
		finalColor.a = 0.0f;
	}
	return finalColor;
}
