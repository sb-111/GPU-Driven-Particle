struct PSInput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};
Texture2D g_SceneColorHalfBuffer : register(t2);
SamplerState g_LinearClamp : register(s0);
float4 main(PSInput input) : SV_TARGET
{
	float4 color = g_SceneColorHalfBuffer.Sample(g_LinearClamp, input.uv);
	return color;
}
