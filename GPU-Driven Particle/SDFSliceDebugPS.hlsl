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
Texture3D<float> SDFTexture : register(t0);
struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD0;
};
float4 main(PSInput input) : SV_TARGET
{
	uint u;
	uint v;

	// uv를 복셀 인덱스로 
	if (axis == 0) // X slice: 화면 u,v = Z,Y
	{
		u = min((uint) (input.uv.x * resolution.z), resolution.z - 1);
		v = min((uint) (input.uv.y * resolution.y), resolution.y - 1);
	}
	else if (axis == 1) // Y slice: 화면 u,v = X,Z
	{
		u = min((uint) (input.uv.x * resolution.x), resolution.x - 1);
		v = min((uint) (input.uv.y * resolution.z), resolution.z - 1);
	}
	else // Z slice: 화면 u,v = X,Y
	{
		u = min((uint) (input.uv.x * resolution.x), resolution.x - 1);
		v = min((uint) (input.uv.y * resolution.y), resolution.y - 1);
	}

	// 3D 텍셀좌표 변환
	uint3 texelCoord;

	if (axis == 0)
		texelCoord = uint3(sliceIndex, v, u);
	else if (axis == 1)
		texelCoord = uint3(u, sliceIndex, v);
	else
		texelCoord = uint3(u, v, sliceIndex);

    // sdf 값
	const float d = SDFTexture.Load(int4(texelCoord, 0));

	// 거리값을 색으로 변환
	const float strength =
        saturate(abs(d) / max(distanceRange, 1e-6f));

	const float3 signedColor =
        (d < 0.0f)
        ? float3(0.0f, 0.0f, 1.0f) // 내부: 파랑
        : float3(1.0f, 0.0f, 0.0f); // 외부: 빨강

    // d가 0에 가까울수록 검정
	return float4(signedColor * strength, 1.0f);
}
