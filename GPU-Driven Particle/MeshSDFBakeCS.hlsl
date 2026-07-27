float dot2(float3 vec)
{
	return dot(vec, vec);
}
// unsigned distance 반환 (square)
float SqDistancePointTriangle(float3 position, float3 v1, float3 v2, float3 v3)
{
	float3 v21 = v2 - v1;
	float3 v32 = v3 - v2;
	float3 v13 = v1 - v3;
	float3 p1 = position - v1;
	float3 p2 = position - v2;
	float3 p3 = position - v3;

	float3 normal = cross(v21, v13);

	return
	// 합 3.0이면 삼각형 내부의 프리즘에 있는 것
	   (sign(dot(cross(v21, normal), p1)) +
        sign(dot(cross(v32, normal), p2)) +
        sign(dot(cross(v13, normal), p3)) < 2.0f)
	?
	// 밖: 세 선분까지 제일 짧은 거리 택
	min(min(
		dot2(v21 * clamp(dot(v21, p1) / dot2(v21), 0.0, 1.0) - p1),
        dot2(v32 * clamp(dot(v32, p2) / dot2(v32), 0.0, 1.0) - p2)),
        dot2(v13 * clamp(dot(v13, p3) / dot2(v13), 0.0, 1.0) - p3))
        :
	// 안: 프리즘 어딘가에 위치하니까 평면에 수직으로 내린 거리
	// 투영 d = |normal p1| / |normal|
        dot(normal, p1) * dot(normal, p1) / dot2(normal);
}

[numthreads(64, 1, 1)]
void main( uint3 tid : SV_DispatchThreadID )
{
	// point에 대해서 모든 삼각형을 순회하며 최단 거리 갱신 후 볼륨 텍스처에 저장
}
