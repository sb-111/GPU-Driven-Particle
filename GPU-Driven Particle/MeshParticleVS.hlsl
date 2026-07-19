#include "ParticleShared.h"
#include "Quaternion.hlsli"

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
StructuredBuffer<Particle> g_ParticleBuffer : register(t0);
ByteAddressBuffer NewAliveList : register(t1);

struct VSInput
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD0;
	uint iid : SV_InstanceID;
};
struct VSOutput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};
VSOutput main(VSInput input)
{
	// 파티클 가져오기
	uint index = NewAliveList.Load(input.iid * 4);
	Particle p = g_ParticleBuffer[index];

	// 나이 기반 페이드
	float normalizedAge = 1.0f - (p.lifeTime / p.initialLife); // 0 -> 1 (progress)
	float brightness = 1.0f - normalizedAge; // 시간 흐르면 밝기 감소 (fade)
	float sizeScale = 1.0f - (normalizedAge * normalizedAge); // 시간 흐르면 사이즈 감소 (fade)

	float3 scaledSize = p.size.xyz * sizeScale;
	float3x3 scaleMat = float3x3(
	scaledSize.x, 0, 0,
	0, scaledSize.y, 0,
	0, 0, scaledSize.z
	);
	float3x3 R = QuatToMatrix(p.orientation);
	float3x3 finalMat = mul(R, scaleMat); // S -> R
	float3 worldPos = p.position + mul(finalMat, input.position);

	VSOutput output = (VSOutput) 0;
	output.position = mul(viewParams.viewProj, float4(worldPos, 1.0f));
	float3 normalDir = mul(R, input.normal / scaledSize);
	output.normal = normalDir;
	output.uv = input.uv;

	output.color.rgb = lerp(p.color.rgb, frameParams.endColor.rgb, normalizedAge);
	if (drawParams.blendMode == BLEND_ADDITIVE_MODE)
	{
		output.color.rgb *= brightness; // 가산: 더하는 빛의 양을 줄임(색 어두워짐 = 페이드)
		output.color.a = 1.0f; // 가산은 알파 안읽음.
	}
	else // BLEND_ALPHA_MODE
	{
		output.color.a = p.color.a * brightness; // 알파: 불투명도를 줄임(색은 유지, 투명해짐)
	}
	return output;
}
