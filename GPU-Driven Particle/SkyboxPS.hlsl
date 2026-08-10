struct PSInput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};
cbuffer SkyboxCB : register(b0)
{
	float4x4 invViewProj;
	float3 cameraPos;
	float pad;
}
TextureCube Skybox : register(t0);
SamplerState LinearClamp : register(s0);
float4 main(PSInput input) : SV_TARGET
{
	float2 ndc = input.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f); 
	float4 world = mul(invViewProj, float4(ndc, 1.0f, 1.0f)); 
	float3 dir = world.xyz / world.w - cameraPos; // 카메라에서 near plane의 점을 보는 벡터
	return Skybox.Sample(LinearClamp, dir);
}
