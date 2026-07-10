#pragma once
#define COUNTER_ALIVE 0
#define COUNTER_DEAD 4
#define COUNTER_REAL 8
#define COUNTER_AFTER_SIMULATE 12

// Args 버퍼의 offset 정의
#define ARGS_EMIT_DISPATCH_X 0
#define ARGS_EMIT_DISPATCH_Y 4
#define ARGS_EMIT_DISPATCH_Z 8
#define ARGS_SIMULATE_DISPATCH_X 16
#define ARGS_SIMULATE_DISPATCH_Y 20
#define ARGS_SIMULATE_DISPATCH_Z 24
#define ARGS_DRAW_VERTEX_COUNT_PER_INSTANCE 32
#define ARGS_DRAW_INSTANCE_COUNT 36

#ifdef __cplusplus
#define GP_CB_ALIGN alignas(16)
#else
#define GP_CB_ALIGN
#endif

#ifdef __cplusplus
	#include <cstdint>
	namespace GP{
	struct float3 { float x, y, z; };
	struct float4 { float x, y, z, w; };
	struct float4x4 { float m[16]; };
	typedef uint32_t uint;
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
	struct GP_CB_ALIGN ParticleDrawCB
	{
		float4x4 viewProj;

		float3 camRight;
		float particleSize;

		float3 camUp;
		float pad0;
	};
#ifdef __cplusplus
	}
#endif // __cplusplus
