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

	float normalizedAge = 1.0f - (p.lifeTime / p.initialLife); // 0 -> 1 (progress)
	float sizeScale = frameParams.useSizeOverLife ? 1.0f - (normalizedAge * normalizedAge) : 1.0f; // 시간 흐르면 사이즈 감소 (fade)

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

	output.color = lerp(p.color, frameParams.endColor, normalizedAge); // RGBA 보간 (color over life)
	return output;
}
