#pragma once
#include "pch.h"
#include "ParticleShared.h"
#include "ParticleSetting.h"
#include "VectorMath.h"
#include "GpuBuffer.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "Texture.h"
#include "BitonicSort.h"

class ComputeContext;
class GraphicsContext;

namespace GP
{
	// Emitter들이 공유하는 자원
	struct ParticleSharedResources
	{
		RootSignature computeRootSig;
		RootSignature graphicsRootSig;
		ComputePSO kickoffPSO, emitPSO, simulatePSO;
		GraphicsPSO drawAdditivePSO, drawAlphaPSO;
		Texture spriteTextures[(int)ETexture::Count];
		BitonicSort sorter;

		GraphicsPSO meshAdditivePSO, meshAlphaPSO;
		ByteAddressBuffer meshVertexBuffer, meshIndexBuffer;

		GraphicsPSO ribbonAdditivePSO, ribbonAlphaPSO;

		// 스프라이트 가산용(하프)
		GraphicsPSO drawAdditiveHalfPSO;

		// 합성용
		GraphicsPSO compositePSO;
	};

	class ParticleEmitter
	{
	public:
		explicit ParticleEmitter(const Math::OrthogonalTransform& transform)
			: m_EmitterTransform(transform) {}
		void Init(uint32_t maxParticles, ParticleSharedResources* shared, uint32_t index);

		// 루프 및 버스트 제어, 요청 스폰량 계산 + FrameCB 준비
		void Update(float dt);
		void ResetEmitter();
		uint32_t GetCurrentSpawnCount() const { return m_CurrentSpawnCount; }
		ParticleSettings& GetSettings() { return m_Settings; }
		Math::Vector3 GetPosition() const { return m_EmitterTransform.GetTranslation(); }

		// Emitter 별 Pass (Particle System이 호출)
		void BindResources(ComputeContext& cpt, const ParticleViewCB& viewParams);
		void KickoffPass(ComputeContext& cpt);
		void EmitPass(ComputeContext& cpt);
		void SimulatePass(ComputeContext& cpt);
		void SortPass(ComputeContext& cpt);
		void UpdateDrawArgs(ComputeContext& cpt);
		void Draw(GraphicsContext& gfx);
		void EndFrame();

	private:
		// ParticleSettings -> ParticleFrameCB
		ParticleFrameCB MakeParams(const ParticleSettings& s, float dt) const;
		bool NeedsSort() const;
		void UpdateOrbit(float dt);

		Math::OrthogonalTransform m_EmitterTransform;
		float m_AgeInLoop = 0.0f;	// 현재 루프에서 경과 시간
		bool m_Active = true;		// 이미터 생존 여부
		int m_CompletedLoops = 0;  // 현재 완료된 루프 수
		bool m_CanBurst = true; // 버스트

		float m_SpawnAccumulator = 0.0f; // 소수점 스폰 이월용
		uint32_t m_CurrentSpawnCount = 0; // 이번 프레임 스폰 수
		uint32_t m_FrameCount = 0;		  // 프레임 카운터

		uint32_t m_maxParticle = 0;
		ParticleSettings m_Settings;
		ParticleFrameCB m_FrameParams = {};
		//ParticleViewCB m_ViewParams = {};

		// GPU Resource - 이미터마다 독립적
		StructuredBuffer m_Pool;
		ByteAddressBuffer m_AliveList1, m_AliveList2, m_DeadList, m_Counters;
		IndirectArgsBuffer m_IndirectArgsBuffer;
		StructuredBuffer m_SortKeys;
		ByteAddressBuffer* m_CurrentAlive = &m_AliveList1; // 핑퐁 스왑용
		ByteAddressBuffer* m_NewAlive = &m_AliveList2;

		// Emitter들이 공유
		ParticleSharedResources* m_Shared = nullptr;

		// 이번 프레임에 살아있는 파티클의 예측량(2의 거듭제곱 상한)
		uint32_t m_SortN = 64;
		float m_Timer = 0.0f;

		// 궤도 운동 상태
		float m_OrbitAngle = 0.0f;
		Math::Vector3 m_OrbitCenter = Math::Vector3(Math::kZero);
	};


}
