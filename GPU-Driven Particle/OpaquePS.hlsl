#include "SceneLighting.hlsli"

cbuffer MaterialCB : register(b1)
{
	float4 baseColor;
	float3 emissive;
	float pad0;
}
struct PSInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
	float3 lit = baseColor.rgb * LambertTerm(input.normal);
	lit += emissive;
	return float4(lit, baseColor.a);
}
