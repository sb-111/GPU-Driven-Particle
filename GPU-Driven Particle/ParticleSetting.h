#pragma once
namespace GP
{
	// ===== 확장용 (런타임 미구현)  =====
	enum class ELoopMode : int { Infinite, Once, Multiple, Count};
	enum class EParticleRenderer : int { Sprite, Mesh, Ribbon, Count};
	enum class EUniformMode : int
	{
		Uniform,
		RandomUniform,
		NonUniform,
		RandomNonUniform,
		Count
	};
	enum class EBlendMode : int {Additive, Alpha, Count};
	enum class ETexture : int {Fire, Smoke, Spark, Count};
	enum class EShapeType: int {Point, Box, Sphere, Cone, Count};
	enum class EVelocityMode : int { Velocity, VelocityFromPoint, VelocityInCone, Count};
	// ImGui 튜닝 값 모은 구조체 
	struct ParticleSettings
	{
		// Emitter
		float spawnRate     = 5000.0f;	
		int   burstCount    = 0;		// 몇개 터뜨릴지
		int   loopMode		= 0;		// 루프 모드
		float loopDuration  = 2.0f;		// 루프 지속 시간
		int   loopCount		= 3;		// 루프 몇번 돌지

		// Particle Emit
		float lifeTimeMin   = 2.0f;
		float lifeTimeMax   = 4.0f;
		float speedMin      = 3.0f;	// 속도 관련
		float speedMax      = 5.0f;

		float spinSpeedMin = 1.0f; // 회전 관련
		float spinSpeedMax = 3.0f;
		float initAngleMin	= 1.0f;
		float initAngleMax	= 5.0f;

		int sizeMode = (int)EUniformMode::Uniform; // 스프라이트는 x,y만 사용
		float sizeMin[3] = { 0.05f, 0.05f, 0.0f };
		float sizeMax[3] = { 1.0f, 1.0f , 0.0f};

		float dirSpread     = 0.3f;		
		float posSpread     = 0.1f;		
		float startColor[4] = { 1.0f, 0.45f, 0.1f, 1.0f };

		int shapeType = (int)EShapeType::Point;
		int velocityMode = (int)EVelocityMode::Velocity;
		float boxExtents[3] = { 5.0f, 5.0f, 5.0f };
		float sphereRadius = 5.0f;
		bool sphereSurfaceOnly = false;
		float coneAngle = 30.0f;

		// Particle Simulate
		float gravity[3]    = { 0.0f, -9.8f, 0.0f };
		float endColor[4]   = { 1.0f, 0.0f, 0.0f, 0.0f };	// color over life 용

		// Renderer
		int blendMode = (int)EBlendMode::Additive;
		int textureIndex = (int)ETexture::Fire;

	};
}
