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

// DrawIndexed용 구간 (for Mesh)
#define ARGS_DRAW_INDEXED_INDEX_COUNT 48
#define ARGS_DRAW_INDEXED_INSTANCE_COUNT 52
#define ARGS_DRAW_INDEXED_START_INDEX 56
#define ARGS_DRAW_INDEXED_BASE_VERTEX 60
#define ARGS_DRAW_INDEXED_START_INSTANCE 64

// Mode: Uniform, RandomUniform, NonUniform, RandomNonUniform
#define UNIFORM_MODE 0
#define RANDOM_UNIFORM_MODE 1
#define NON_UNIFORM_MODE 2
#define RANDOM_NON_UNIFORM_MODE 3

// Blend Mode: Additive, Alpha
#define BLEND_ADDITIVE_MODE 0
#define BLEND_ALPHA_MODE 1

// Shape Type: (Emit 그룹에서 방출 모양 결정용)
#define POINT_TYPE 0
#define BOX_TYPE 1
#define SPHERE_TYPE 2
#define CONE_TYPE 3

#define VELOCITY_MODE 0
#define VELOCITY_FROM_POINT_MODE 1
#define VELOCITY_IN_CONE_MODE 2

#define ALIGN_UNALIGNED_MODE 0
#define ALIGN_VELOCITY_MODE 1


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
		int shapeType;

		float3 gravity;
		uint randomeSeed;

		float dirSpread;
		float posSpread;
		float pad0;
		float pad1;

		float3 shapeData;
		int velocityMode;

		// 콘 앵글 추가하기 (velocity용)
		float coneAngle;
		float3 pad2;
	};
	// 프레임당 1번, Compute/Graphics 공용
	struct GP_CB_ALIGN ParticleViewCB
	{
		float4x4 viewProj;

		float3 camPos; float pad0;
		float3 camRight; float pad1;
		float3 camUp; float pad2;
		float3 camForward; float pad3;
	};
	// Emitter 별 렌더 설정
	struct GP_CB_ALIGN ParticleDrawCB
	{
		int blendMode;
		int alignmentMode;
		int pad0;
		int pad1;
	};
#ifdef __cplusplus
	}
#endif // __cplusplus
