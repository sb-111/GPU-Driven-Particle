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
#include "MorphTargetSet.h"
#include "ParticleSequence.h"
class ComputeContext;
class GraphicsContext;

namespace GP
{
	class Mesh;
	class TextureLibrary;

	// Emitter들이 공유하는 자원
	struct ParticleSharedResources
	{
		RootSignature computeRootSig;
		RootSignature graphicsRootSig;
		ComputePSO kickoffPSO, emitPSO, simulatePSO;
		TextureLibrary* textureLibrary = nullptr;
		BitonicSort sorter;

		// [렌더러][해상도]
		GraphicsPSO drawPSO[(int)EParticleRenderer::Count][2];

		// 불투명 메시 파티클 전용 / 정렬 대신 깊이 버퍼로 앞뒤를 가림 [해상도]
		GraphicsPSO meshOpaquePSO[2];

		ByteAddressBuffer meshVertexBuffer, meshIndexBuffer;

		// DownSample용
		GraphicsPSO downsampleDepthPSO;

		// 저해상도 파티클 렌더 후 합성할 때만 사용
		GraphicsPSO compositePSO;

		// 타겟 없을 때 쓰는 더미
		StructuredBuffer defaultMorphTargetBuffer;
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
		uint32_t GetMaxParticles() const { return m_maxParticle; }
		const std::string& GetName() const { return m_Name; }
		void SetName(const std::string& name) { m_Name = name; }
		ParticleSettings& GetSettings() { return m_Settings; }
		Math::Vector3 GetPosition() const { return m_EmitterTransform.GetTranslation(); }

		Math::Vector3 GetBasePosition() const { return m_BasePosition; }
		void SetBasePosition(const Math::Vector3& p) { m_BasePosition = p; }

		const float (&GetBaseRotationEuler() const)[3] { return m_BaseRotationEuler; }
		void SetBaseRotationEuler(const float (&eulerDeg)[3])
		{
			m_BaseRotationEuler[0] = eulerDeg[0];
			m_BaseRotationEuler[1] = eulerDeg[1];
			m_BaseRotationEuler[2] = eulerDeg[2];
			m_EmitterTransform.SetRotation(Math::Quaternion(
				DirectX::XMConvertToRadians(eulerDeg[0]),
				DirectX::XMConvertToRadians(eulerDeg[1]),
				DirectX::XMConvertToRadians(eulerDeg[2])));
		}

		// Emitter 별 Pass (Particle System이 호출)
		void BindResources(ComputeContext& cpt, const ParticleViewCB& viewParams, const ParticleCollisionCB& collisionParams,
			const D3D12_CPU_DESCRIPTOR_HANDLE* sdfSRVs, uint32_t sdfCount,
			const BVHNode* bvhNodes, uint32_t bvhNodeCount);
		void KickoffPass(ComputeContext& cpt);
		void EmitPass(ComputeContext& cpt);
		void SimulatePass(ComputeContext& cpt);
		void SortPass(ComputeContext& cpt);
		void UpdateDrawArgs(ComputeContext& cpt);
		void Draw(GraphicsContext& gfx, bool halfResolution);
		bool IsOpaque() const; // 불투명 PSO 사용하는지
		void EndFrame();

		void SetMorphTarget(MorphTargetSet* morphTarget) { m_CurrentMorphTarget = morphTarget; }
		MorphTargetSet* GetMorphTarget() const { return m_CurrentMorphTarget; } // 비소유 참조
		void SetMorphTargetPath(const std::string& path) { m_MorphTargetPath = path; }
		void SetMorphTargetSampleCount(uint32_t count) { m_MorphTargetSampleCount = count; }
		void SetMorphTargetSeed(uint32_t seed) { m_MorphTargetSeed = seed; }
		const std::string& GetMorphTargetPath() const { return m_MorphTargetPath; }
		uint32_t GetMorphTargetSampleCount() const { return m_MorphTargetSampleCount; }
		uint32_t GetMorphTargetSeed() const { return m_MorphTargetSeed; }

		void SetParticleMesh(Mesh* mesh, const std::string& path)
		{
			m_ParticleMesh = mesh;
			m_ParticleMeshPath = mesh != nullptr ? path : std::string();
		}
		Mesh* GetParticleMesh() const { return m_ParticleMesh; }
		const std::string& GetParticleMeshPath() const { return m_ParticleMeshPath; }

		// ============= 파티클 시퀀스 관련 =============
		EmitterSequence& GetSequence() { return m_Sequence; }
		void TickSequence(float dt); 	// 시퀀스 Tick
		void ApplyStage();				// 현재 SequenceStage 이미터에 적용
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

		float m_TotalTime = 0.0f;		 // 누적 시간(초)
		float m_SpawnAccumulator = 0.0f; // 소수점 스폰 이월용
		uint32_t m_CurrentSpawnCount = 0; // 이번 프레임 스폰 수
		uint32_t m_FrameCount = 0;		  // 프레임 카운터

		uint32_t m_maxParticle = 0;
		std::string m_Name;
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

		MorphTargetSet* m_CurrentMorphTarget = nullptr;
		Mesh* m_ParticleMesh = nullptr;       // 없으면 큐브 사용
		std::string m_ParticleMeshPath;       // 직렬화용
		std::string m_MorphTargetPath;
		uint32_t m_MorphTargetSampleCount = 0;
		uint32_t m_MorphTargetSeed = 0;

		EmitterSequence m_Sequence;

		// 이번 프레임에 살아있는 파티클의 예측량(2의 거듭제곱 상한)
		uint32_t m_SortN = 64;
		float m_Timer = 0.0f;

		Math::Vector3 m_BasePosition = Math::Vector3(Math::kZero); // Emitter 기준 위치
		float m_BaseRotationEuler[3] = { 0.0f, 0.0f, 0.0f }; // Emitter 기준 회전 (deg)
		float m_OrbitAngle = 0.0f;
	};


}
