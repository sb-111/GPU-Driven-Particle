float dot2(float3 vec)
{
	return dot(vec, vec);
}
float DistanceSqToTriangle(float3 position, float3 v1, float3 v2, float3 v3)
{
	float3 v21 = v2 - v1;
	float3 v32 = v3 - v2;
	float3 v13 = v1 - v3;
	float3 p1 = position - v1;
	float3 p2 = position - v2;
	float3 p3 = position - v3;

	float3 normal = cross(v21, v13);

	return
	   (sign(dot(cross(v21, normal), p1)) +
        sign(dot(cross(v32, normal), p2)) +
        sign(dot(cross(v13, normal), p3)) < 2.0f)
	?
	min(min(
		dot2(v21 * clamp(dot(v21, p1) / dot2(v21), 0.0, 1.0) - p1),
        dot2(v32 * clamp(dot(v32, p2) / dot2(v32), 0.0, 1.0) - p2)),
        dot2(v13 * clamp(dot(v13, p3) / dot2(v13), 0.0, 1.0) - p3))
        :
        dot(normal, p1) * dot(normal, p1) / dot2(normal);
}

[numthreads(64, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	
}
