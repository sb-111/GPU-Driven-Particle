cbuffer BakeCB : register(b0)
{
	float3 boundsMin;
	uint triangleCount;
	float3 boundsSize;
	uint pad0;
	uint3 resolution;
	uint pad1;
}
ByteAddressBuffer Vertices : register(t0);
ByteAddressBuffer Indices : register(t1);
RWTexture3D<float> SDFTexture : register(u0);
float dot2(float3 vec)
{
	return dot(vec, vec);
}
bool RayIntersectsTriangle(float3 origin, float3 dir, float3 v1, float3 v2, float3 v3)
{
	float3 e1 = v2 - v1;
	float3 e2 = v3 - v1;
	float3 h = cross(dir, e2);
	float det = dot(e1, h);
	if (abs(det) < 1e-8f)
		return false; // 레이랑 평면 평행

	float invDet = 1.0f / det;
	float3 s = origin - v1;
	float u = dot(s, h) * invDet;
	if (u < 0.0f || u > 1.0f)
		return false;

	float3 q = cross(s, e1);
	float v = dot(dir, q) * invDet;
	if (v < 0.0f || u + v > 1.0f)
		return false;

	float t = dot(e2, q) * invDet;
	return t > 1e-6f; // 교차하면 true
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

[numthreads(8, 8, 8)]
void main( uint3 tid : SV_DispatchThreadID )
{
	// point에 대해서 모든 삼각형을 순회하며 최단 거리 갱신 후 볼륨 텍스처에 저장
	// 스레드 중 하나라도 해상도 넘어가면 리턴
	if (any(tid >= resolution))
		return;

	float3 t = (float3(tid) + 0.5f) / float3(resolution);
	float3 pos = lerp(boundsMin, boundsMin + boundsSize, t);

	uint crossCount = 0;
	
	float minSqDistance = 1e30f;
	float3 rayDir = float3(0.3f, 0.6f, 0.4f); // 임의 ray
	// 모든 삼각형에 대해 순회
	for (uint triangleIdx = 0; triangleIdx < triangleCount; ++triangleIdx)
	{
		uint3 idx = Indices.Load3(triangleIdx * 12); // 삼각형을 이루는 인덱스가 3개 퍼오기
		float3 v1 = asfloat(Vertices.Load3(idx.x * 28)); // 정점 Stride = 28, pos(12B)만 퍼오기
		float3 v2 = asfloat(Vertices.Load3(idx.y * 28)); // 정점 Stride = 28, pos(12B)만 퍼오기
		float3 v3 = asfloat(Vertices.Load3(idx.z * 28)); // 정점 Stride = 28, pos(12B)만 퍼오기

		float sqDistance = SqDistancePointTriangle(pos, v1, v2, v3);
		minSqDistance = min(minSqDistance, sqDistance); // 더 작은 값 갱신

		if(RayIntersectsTriangle(pos, rayDir,v1,v2,v3))
			crossCount++;
	}
	float d = sqrt(minSqDistance);
	if (crossCount & 1) // 홀수면 메시 내부 판정
		d = -d;
	SDFTexture[tid] = d;
}
