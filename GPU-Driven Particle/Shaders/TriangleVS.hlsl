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
    o.pos   = float4(input.pos, 1.0f); // NDC 좌표를 그대로 (w=1)
    o.color = input.color;
    return o;
}
