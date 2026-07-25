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
	float4 finalColor = sampleColor * input.color; // a채널 = 텍스처 a x 수명 페이드

	// 블렌드 설정이 SrcBlend = ONE이라 여기서 미리 곱해둠 (pre-multiplied)
	finalColor.rgb *= finalColor.a;

	if (drawParams.blendMode == BLEND_ADDITIVE_MODE)
	{
		finalColor.a = 0.0f; // 가산은 뒤를 안 가림
	}
	return finalColor;
}
