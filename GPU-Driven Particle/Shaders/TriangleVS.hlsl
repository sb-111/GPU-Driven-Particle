
cbuffer VSConstants : register(b0)
{
	float4x4 gViewProj;
}
struct VSInput
{
    float3 pos   : POSITION;
    float4 color : COLOR;   
};

struct VSOutput
{
    float4 pos   : SV_POSITION;
    float4 color : COLOR;      
};

VSOutput main(VSInput input)
{
    VSOutput o;
	float4 clipPos = mul(gViewProj, float4(input.pos, 1.0f));
    o.pos   = clipPos; // NDC 좌표를 그대로 (w=1)
    o.color = input.color;
    return o;
}
