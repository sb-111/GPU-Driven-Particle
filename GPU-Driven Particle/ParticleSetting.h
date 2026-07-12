#pragma once
namespace GP
{
	// ===== 확장용 (런타임 미구현)  =====
	enum class ELoopMode : int
	{
		Infinite, Once, Multiple
	};
	enum class EParticleRenderer : int
	{
		Sprite, Mesh, Ribbon
	};
	enum class ESpriteSizeMode : int
	{
		Uniform,
		RandomUniform,
		NonUniform,
		RandomNonUniform
	};

	// ImGui 튜닝 값 모은 구조체 
	struct ParticleSettings
	{
		// Emitter
		float spawnRate     = 5000.0f;	
		int   burstCount    = 0;		

		// Particle Emit
		float lifeTimeMin   = 2.0f;
		float lifeTimeMax   = 4.0f;
		float speedMin      = 3.0f;		
		float speedMax      = 5.0f;
		float dirSpread     = 0.3f;		
		float posSpread     = 0.1f;		
		float startColor[4] = { 1.0f, 0.45f, 0.1f, 1.0f };

		// Particle Simulate
		float gravity[3]    = { 0.0f, -9.8f, 0.0f };
		float endColor[4]   = { 1.0f, 0.0f, 0.0f, 0.0f };	// color over life 용

		// Renderer
		float particleSize  = 0.05f;
	};
}
