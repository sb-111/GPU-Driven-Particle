#pragma once
#include "pch.h"
#include "ParticleShared.h"
#include "ParticleEmitter.h"
#include "ParticleSetting.h"
#include "ShaderCompiler.h"

#include <fstream>
#include <memory>
#include "Texture.h"
#include "MorphTargetLibrary.h"

class GraphicsContext;
class ComputeContext;

namespace GP {

	class SceneObject;

	// 멀티 이미터 소유
	class ParticleSystem
	{
	public:
		void Init(uint32_t maxParticlesPerEmitter);
		// Emitter 추가
		void AddEmitter(const Math::OrthogonalTransform& transform);

		// 이미터 갱신, CB 준비
		void Update(float dt);

		void UpdateGPU(ComputeContext& cpt, const ParticleViewCB& viewParams);
		void Render(GraphicsContext& gfx, const ParticleViewCB& viewCB);
		void EndFrame();

		size_t GetEmitterCount() const { return m_Emitters.size(); }
		ParticleEmitter& GetEmitter(size_t i) { return *m_Emitters[i]; }
		bool& GetHalfResolution() { return m_HalfResolution; }
		CollisionSettings& GetCollisionSettings() { return m_CollisionSettings; }

		// SDF Collider 등록
		void AddSDFCollider(SceneObject* pObject);
		void ClearMorphTargets();
		void ClearSDFColliders();
		void ClearEmitters();

		MorphTargetSet* ResolveSurfaceMorphTarget(const std::string& name, const Mesh& mesh,
			uint32_t sampleCount, uint32_t seed);


	private:
		void InitSharedResources();
		void DownSampleSceneDepth(GraphicsContext& gfx);
		void DrawEmitters(GraphicsContext& gfx, const ParticleViewCB& viewCB, bool halfResolution);
		void CompositeToMain(GraphicsContext& gfx);

		// half resolution 렌더 여부
		bool m_HalfResolution = true;
		CollisionSettings m_CollisionSettings;
		uint32_t m_maxParticle = 0;
		ParticleSharedResources m_Shared;
		std::vector<std::unique_ptr<ParticleEmitter>> m_Emitters;

		// SDF Collider 목록
		std::vector<SceneObject*> m_SDFColliders;

		// Morph target 용
		MorphTargetLibrary m_MorphTargetLibrary;
	};
}
