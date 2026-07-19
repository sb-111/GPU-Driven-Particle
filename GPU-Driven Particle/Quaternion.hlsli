#ifndef QUATERNION_HLSLI
#define QUATERNION_HLSLI

// 단위 축 n으로 각도 세타만큼 도는 회전
/*
* @param axis 단위 축
* @param angle 세타(rad)
*/
float4 QuatFromAxisAngle(float3 axis, float angle)
{
	float halfAngle = angle * 0.5f;
	float s = sin(halfAngle);
	return float4(axis * s, cos(halfAngle));
}
// 회전 합성. a*b = b 먼저, a 나중 (행렬 곱과 같은 방향)
float4 QuatMul(float4 a, float4 b)
{
	return float4(
		a.w * b.xyz + b.w * a.xyz + cross(a.xyz, b.xyz),
		a.w * b.w - dot(a.xyz, b.xyz));
}

// 쿼터니언 -> 회전 행렬, q는 단위 길이 가정
float3x3 QuatToMatrix(float4 q)
{
	float x = q.x, y = q.y, z = q.z, w = q.w;
	return float3x3(
		1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z),        2.0f * (x * z + w * y),
		2.0f * (x * y + w * z),        1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x),
		2.0f * (x * z - w * y),        2.0f * (y * z + w * x),        1.0f - 2.0f * (x * x + y * y));
}
#endif
