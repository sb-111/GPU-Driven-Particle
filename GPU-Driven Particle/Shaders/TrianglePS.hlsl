struct PSInput
{
    float4 pos   : SV_POSITION; 
    float4 color : COLOR;      
};

float4 main(PSInput input) : SV_TARGET // 렌더 타겟에 쓸 최종 색
{
    return input.color;
}
