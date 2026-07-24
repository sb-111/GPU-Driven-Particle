struct VSOutput
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD0;
};

VSOutput main(uint vid : SV_VertexID)
{
	VSOutput output = (VSOutput) 0 ;

	// [0,0], [2,0], [0,2]
	output.uv = float2(vid & 2, (vid << 1) & 2);
	output.position = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
	
	return output;
}
