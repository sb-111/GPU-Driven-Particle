#pragma once
#ifdef __cplusplus
	#include <cstdint>
	struct float3 { float x, y, z; };
	typedef uint32_t uint;
	namespace GP{
#endif // __cplusplus

	struct Particle
	{
		float3 position;
		float lifeTime;
		float3 velocity;
	};

#ifdef __cplusplus
	}
#endif // __cplusplus
