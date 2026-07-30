cbuffer OpaqueCB : register(b0)
{
	float4x4 worldMat;
	float4x4 viewProjMat;
}
struct VSInput
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
};

struct VSOutput
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
};

VSOutput main(VSInput input)
{
	VSOutput output = (VSOutput) 0;
	float4 worldPos = mul(worldMat, float4(input.pos, 1.0f));
	float4 clipPos = mul(viewProjMat, worldPos);
	output.pos = clipPos;
	output.normal = normalize(mul((float3x3) worldMat, input.normal));
	return output;
}
