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
		/**
		* @brief Emitter 추가
		* @param transform 추가할 위치
		* @param maxParticles 풀 크기 지정 (0이면 전역 값), 2의 거듭제곱으로 올림되고, 전역값 못넘음
		*/
		void AddEmitter(const Math::OrthogonalTransform& transform, uint32_t maxParticles = 0);

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
		/**
		* @brief BVH 트리 빌드 (m_BVHNodes)
		* @param aabbMin 월드 Min
		* @param aabbMax 월드 Max
		* @param colliderCount 유효 콜라이더 수(=sdfCount)
		* @return 만들어진 노드 수(m_BVHNodeCount)
		*/
		uint32_t BuildBVH(const Math::Vector3* aabbMin, const Math::Vector3* aabbMax, uint32_t colliderCount);
		/**
		* @brief BVH Node 빌드
		* @param aabbMin 월드 Min
		* @param aabbMax 월드 Max
		* @param indices 이번 호출 구간의 시작 포인터 (정렬되는 배열)
		* @param count 담당 구간의 길이 (count == 1은 리프)
		* @return 이 서브트리 루트의 노드 인덱스
		*/
		uint32_t BuildBVHNode(const Math::Vector3* aabbMin, const Math::Vector3* aabbMax, uint32_t* indices, uint32_t count);

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

		// bvh 관련
		BVHNode m_BVHNodes[MAX_BVH_NODES];
		uint32_t m_BVHNodeCount = 0;
	};
}
