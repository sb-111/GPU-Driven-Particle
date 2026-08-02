cbuffer SDFSliceCB : register(b0)
{
	float4x4 localToWorld;
	float4x4 viewProj;

	float3 volumeBoundsMin;
	uint axis;

	float3 volumeBoundsSize;
	uint sliceIndex;

	uint3 resolution;
	float distanceRange;
};
struct VSOutput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};
VSOutput main(uint vid : SV_VertexID)
{
	VSOutput output = (VSOutput) 0;

	const float2 kUV[6] =
	{
		float2(0.0f, 0.0f), float2(1.0f, 0.0f), float2(1.0f, 1.0f),
        float2(0.0f, 0.0f), float2(1.0f, 1.0f), float2(0.0f, 1.0f),
	};
	const float2 uv = kUV[vid];
	const float3 volumeBoundsMax =
        volumeBoundsMin + volumeBoundsSize;
	const float slicePosition =
        volumeBoundsMin[axis] + (float(sliceIndex) + 0.5f) / float(resolution[axis]) * volumeBoundsSize[axis];

	float3 localPosition;

	if (axis == 0) // X slice: YZ 평면
	{
		localPosition = float3(
            slicePosition,
            lerp(volumeBoundsMin.y, volumeBoundsMax.y, uv.y),
            lerp(volumeBoundsMin.z, volumeBoundsMax.z, uv.x));
	}
	else if (axis == 1) // Y slice: XZ 평면
	{
		localPosition = float3(
            lerp(volumeBoundsMin.x, volumeBoundsMax.x, uv.x),
            slicePosition,
            lerp(volumeBoundsMin.z, volumeBoundsMax.z, uv.y));
	}
	else // Z slice: XY 평면
	{
		localPosition = float3(
            lerp(volumeBoundsMin.x, volumeBoundsMax.x, uv.x),
            lerp(volumeBoundsMin.y, volumeBoundsMax.y, uv.y),
            slicePosition);
	}

	output.position = mul(viewProj, mul(localToWorld, float4(localPosition, 1.0f)));
	output.uv = uv;
	return output;
}
