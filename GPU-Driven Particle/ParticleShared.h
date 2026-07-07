#pragma once
#define COUNTER_ALIVE 0
#define COUNTER_DEAD 4
#define COUNTER_REAL 8
#define COUNTER_AFTER_SIMULATE 12

#ifdef __cplusplus
#define GP_CB_ALIGN alignas(16)
#else
#define GP_CB_ALIGN
#endif

#ifdef __cplusplus
	#include <cstdint>
	struct float3 { float x, y, z; };
	struct float4 { float x, y, z, w; };
	typedef uint32_t uint;
	namespace GP{
#endif // __cplusplus

	struct Particle
	{
		float3 position;
		float lifeTime;

		
		float3 velocity;
		float initialLife; // 태어날 때 수명
		float4 color;
	};


	struct GP_CB_ALIGN ParticleFrameCB
	{
		float3 emitterPosition;	// world pos
		uint emitCount;			// spawn count in this frame

		float3 emitterDirection; // look direction
		float deltaTime;		// dt

		float4 startColor;		// color
		float minLifeTime;		// life
		float maxLifeTime;
		uint randomeSeed;
		uint pad0;
	};
#ifdef __cplusplus
	}
#endif // __cplusplus
