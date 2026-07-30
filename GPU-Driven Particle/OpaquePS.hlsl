cbuffer MaterialCB : register(b1)
{
	float4 baseColor;
}
struct PSInput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
};

float4 main(PSInput input) : SV_TARGET
{
	float3 n = normalize(input.normal);
	float3 lightDir = normalize(float3(0.4f, 0.8f, 0.45f));
	float3 lit = baseColor.rgb * (0.25f + 0.75f * saturate(dot(n, lightDir)));
	return float4(lit, baseColor.a);
}
