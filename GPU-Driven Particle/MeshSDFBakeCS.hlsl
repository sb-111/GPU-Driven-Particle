#define RAY_COUNT 15
static const float3 kRayDirs[RAY_COUNT] =
{
	float3(0.359011f, 0.933333f, 0.000000f),
	float3(-0.442421f, 0.800000f, 0.405294f),
	float3(0.065163f, 0.666667f, -0.742502f),
	float3(0.514682f, 0.533333f, 0.671311f),
	float3(-0.902505f, 0.400000f, -0.159640f),
	float3(0.813202f, 0.266667f, -0.517292f),
	float3(-0.257286f, 0.133333f, 0.957092f),
	float3(-0.460907f, 0.000000f, -0.887448f),
	float3(0.930934f, -0.133333f, 0.339976f),
	float3(-0.890874f, -0.266667f, 0.367740f),
	float3(0.388461f, -0.400000f, -0.830119f),
	float3(0.253166f, -0.533333f, 0.807132f),
	float3(-0.644890f, -0.666667f, -0.373727f),
	float3(0.586005f, -0.800000f, -0.128832f),
	float3(-0.206478f, -0.933333f, 0.293693f),
};
cbuffer BakeCB : register(b0)
{
	float3 boundsMin;
	uint triangleCount;
	float3 boundsSize;
	uint vertexStride;
	uint3 resolution;
	uint positionOffset;
}
ByteAddressBuffer Vertices : register(t0);
ByteAddressBuffer Indices : register(t1);
RWTexture3D<float> SDFTexture : register(u0);
float dot2(float3 vec)
{
	return dot(vec, vec);
}
bool RayIntersectsTriangle(float3 origin, float3 dir, float3 v1, float3 v2, float3 v3, out float hitT)
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

	hitT = dot(e2, q) * invDet;
	return hitT > 1e-6f; // 교차하면 true
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

	float3 uvw = (float3(tid) + 0.5f) / float3(resolution);
	float3 pos = lerp(boundsMin, boundsMin + boundsSize, uvw);

	float minSqDistance = 1e30f;
	float nearestT[RAY_COUNT]; // 이 ray가 지금까지 발견한 가장 가까운 교차 거리
	bool nearestBackface[RAY_COUNT]; // 가장 가까운 triangle이 backface인지

	// 초기화
	[unroll]
	for (uint r = 0; r < RAY_COUNT;++r)
	{
		nearestT[r] = 1e30f;
		nearestBackface[r] = false;
	}
	
	// 모든 삼각형에 대해 순회
	for (uint triangleIdx = 0; triangleIdx < triangleCount; ++triangleIdx)
	{
		uint3 idx = Indices.Load3(triangleIdx * 12); // 삼각형을 이루는 인덱스가 3개 퍼오기
		float3 v1 = asfloat(Vertices.Load3(idx.x * vertexStride + positionOffset)); // pos만 퍼오기 
		float3 v2 = asfloat(Vertices.Load3(idx.y * vertexStride + positionOffset));
		float3 v3 = asfloat(Vertices.Load3(idx.z * vertexStride + positionOffset));
		float3 normal = cross(v2 - v1, v3 - v1);
		if(dot(normal, normal) < 1e-12f)
			continue;
		
		// point to triangle distance (Unsigned)
		float sqDistance = SqDistancePointTriangle(pos, v1, v2, v3);
		minSqDistance = min(minSqDistance, sqDistance); // 더 작은 값 갱신

		// ================== Sign Vote ==================
		// 여러 ray에 대해 현재 triangle 교차 검사
		[unroll]
		for (uint r = 0; r < RAY_COUNT; ++r)
		{
			float hitT = 0.0f;
			float3 rayDir = kRayDirs[r];
			if (RayIntersectsTriangle(pos, rayDir, v1, v2, v3, hitT))
			{
				// 해당 ray가 더 가까운 삼각형과 부딪혔으면
				if(hitT < nearestT[r])
				{
					// 최단 교차거리 갱신 및 backface 여부
					nearestT[r] = hitT;
					nearestBackface[r] = dot(rayDir, normal) > 0.0f; // 내적 양수면 뒷면
				}
			}
		}
	}
	float d = sqrt(minSqDistance);
	// Sign Vote
	uint backfaceCount = 0;
	[unroll]
	for (uint r = 0; r < RAY_COUNT; ++r)
	{
		if(nearestBackface[r])
			++backfaceCount;
	}
	bool inside = backfaceCount * 2 > RAY_COUNT;
	if(inside)
		d = -d;
	SDFTexture[tid] = d;
}
