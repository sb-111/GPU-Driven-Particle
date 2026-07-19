struct PSInput
{
	float4 position : SV_Position;
	float3 normal : NORMAL;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_TARGET
{
	float3 N = normalize(input.normal);
	float3 L = normalize(float3(0.4f, 1.0f, 0.2f));
	float nDotL = dot(N, L);
	float bright = saturate(nDotL) * 0.8f + 0.25f;
	return float4(bright * input.color.rgb, input.color.a);
}
