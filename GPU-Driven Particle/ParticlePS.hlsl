struct PSInput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};
Texture2D g_SpriteTex : register(t2);
SamplerState g_LinearClamp : register(s0);


float4 main(PSInput input) : SV_Target
{
	float4 sampleColor = g_SpriteTex.Sample(g_LinearClamp, input.uv);
	float4 finalColor = sampleColor * input.color;
	finalColor.rgb *= sampleColor.a;
	return finalColor;
}
