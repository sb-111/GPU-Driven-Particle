#ifndef PARTICLE_RANDOM_HLSLI
#define PARTICLE_RANDOM_HLSLI

uint wang_hash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

float rand01(inout uint seed)     // 호출할 때마다 seed가 굴러감
{
	seed = wang_hash(seed);
	return seed / 4294967295.0; // uint 최대값으로 나눔 → [0,1)
}

float HashCell(int3 c)   // 격자 꼭짓점 -> [-1,1]
{
	uint seed = (uint) c.x * 73856093u
              ^ (uint) c.y * 19349663u
              ^ (uint) c.z * 83492791u;
	return wang_hash(seed) / 4294967295.0 * 2.0 - 1.0;
}

#endif // PARTICLE_RANDOM_HLSLI
