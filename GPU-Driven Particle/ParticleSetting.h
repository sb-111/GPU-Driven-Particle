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
	enum class ETexture : int {Fire, Smoke, Spark, Boom, Explosion, Count};
	enum class EShapeType: int {Point, Box, Sphere, Cone, Count};
	enum class EVelocityMode : int { Velocity, VelocityFromPoint, VelocityInCone, Count};
	enum class EAlignmentMode : int { UnAligned, VelocityAligned, Count}; // up을 뭐로 정의할지: 카메라 up, 속도벡터를 쿼드에 투영한 걸 up
	// ImGui 튜닝 값 모은 구조체 
	struct ParticleSettings
	{
		// Emitter
		float spawnRate     = 5000.0f;	
		int   burstCount    = 0;		// 몇개 터뜨릴지
		int   loopMode		= 0;		// 루프 모드
		float loopDuration  = 2.0f;		// 루프 지속 시간
		int   loopCount		= 3;		// 루프 몇번 돌지

		bool  orbitEnabled  = false;	// 이미터 궤도 운동할지 (데모용)
		float orbitRadius   = 2.0f;
		float orbitSpeed    = 2.0f;		// rad/s

		// Particle Emit
		float lifeTimeMin   = 2.0f;
		float lifeTimeMax   = 4.0f;
		float speedMin      = 3.0f;	// 속도 관련
		float speedMax      = 5.0f;

		float spinSpeedMin = 1.0f; // 회전 관련
		float spinSpeedMax = 3.0f;
		float initAngleMin	= 1.0f;
		float initAngleMax	= 5.0f;

		float rotationRateMin = 90.0f; // 태어날 때 회전 속력 랜덤 범위 (deg/s)
		float rotationRateMax = 360.0f;
		float rotationAxis[3] = { 0.0f, 0.0f, 1.0f }; // 태어날 때 회전 축
		bool randomRotationAxis = true;				  // 회전 축 랜덤으로 줄지
		bool randomInitOrientation = true;			  // 초기 자세 랜덤으로 줄지 (끄면 identity)

		int sizeMode = (int)EUniformMode::Uniform; // 스프라이트는 x,y만 사용
		float sizeMin[3] = { 0.05f, 0.05f, 0.0f };
		float sizeMax[3] = { 1.0f, 1.0f , 0.0f};

		float dirSpread     = 0.3f;
		float posSpread     = 0.1f;
		float startColor[4] = { 1.0f, 0.45f, 0.1f, 1.0f };
		bool randomSpawnBrightness = true; // 스폰 시 밝기 랜덤으로 줄지 (랜덤이면 현재는 0.6~1.0f 사이, TODO: 그 랜덤 값 범위 지정도 할 수 있게 변경) 

		int shapeType = (int)EShapeType::Point;
		int velocityMode = (int)EVelocityMode::Velocity;
		float boxExtents[3] = { 5.0f, 5.0f, 5.0f };
		float sphereRadius = 5.0f;
		bool sphereSurfaceOnly = false;
		float coneAngle = 30.0f;

		// Particle Simulate
		float gravity[3]    = { 0.0f, -9.8f, 0.0f };
		float endColor[4]   = { 1.0f, 0.0f, 0.0f, 0.0f };	// color over life 용
		bool sizeOverLife = true; // 수명 따라 사이즈 감쇠 줄지

		// Renderer
		int rendererType = (int)EParticleRenderer::Sprite;
		int blendMode = (int)EBlendMode::Additive;
		int alignmentMode = (int)EAlignmentMode::UnAligned;
		int textureIndex = (int)ETexture::Fire;
		int subImagesX = 1; // 아틀라스 격자 개수
		int subImagesY = 1;
		bool sortEnabled = true; // 알파 모드에서만 의미 있음, before/after 비교용

	};
}
