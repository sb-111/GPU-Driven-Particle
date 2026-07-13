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

// Uniform, RandomUniform, NonUniform, RandomNonUniform
#define UNIFORM_MODE 0
#define RANDOM_UNIFORM_MODE 1
#define NON_UNIFORM_MODE 2
#define RANDOM_NON_UNIFORM_MODE 3


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
		float3 position; float lifeTime;
		float3 velocity; float initialLife; // 태어날 때 수명
		float4 color;

		// size: Uniform이면 x, NonUniform은 x, y / z는 메시용
		float3 size;	 float spinSpeed;
		float3 angle; // 스프라이트는 z만 사용
		float pad0;
	};

	struct GP_CB_ALIGN ParticleFrameCB
	{
		float3 emitterPosition;	// world pos
		uint emitCount;			// spawn count in this frame

		float3 emitterDirection; // look direction
		float deltaTime;		// dt

		float4 startColor;		// color
		float4 endColor;

		float speedMin;
		float speedMax;
		float lifeTimeMin;		// life
		float lifeTimeMax;

		float spinSpeedMin;
		float spinSpeedMax;
		float initAngleMin;
		float initAngleMax;

		int sizeMode;
		float3 sizeMin;

		float3 sizeMax;
		float pad_size;

		float3 gravity;
		uint randomeSeed;

		float dirSpread;
		float posSpread;
		float pad0;
		float pad1;
	};
	struct GP_CB_ALIGN ParticleDrawCB
	{
		float4x4 viewProj;

		float3 camRight;
		float pad0;

		float3 camUp;
		float pad1;
	};
#ifdef __cplusplus
	}
#endif // __cplusplus
