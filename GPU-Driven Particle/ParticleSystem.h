#pragma once
#include "pch.h"
#include "ParticleShared.h"
#include "GpuBuffer.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "ParticleEmitter.h"
#include "ShaderCompiler.h"

class GraphicsContext;
class ComputeContext;

namespace GP {

	class ParticleSystem
	{
	public:
		void Init(uint32_t maxParticles);

		// 이미터 갱신, CB 준비
		void Update(float dt);

		void UpdateGPU(ComputeContext& cpt);   
		void Draw(GraphicsContext& gfx, const Math::Matrix4& viewProj);
		void EndFrame();
	private:
		// UpdateGPU의 내부 패스들 
		void BindResources(ComputeContext& cpt);   
		void KickoffPass(ComputeContext& cpt);     
		void EmitPass(ComputeContext& cpt);        
		void SimulatePass(ComputeContext& cpt);    
		void UpdateDrawArgs(ComputeContext& cpt);  

		uint32_t m_maxParticle = 0;
		ParticleEmitter m_Emitter{
			Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 0.0f)),  
			5000.0f,   // spawnRate: 초당 5000개
			2.0f,      // minLifeTime
			4.0f       // maxLifeTime
		};
		ParticleFrameCB m_FrameParams = {};

		// GPU 리소스
		StructuredBuffer m_Pool;
		ByteAddressBuffer m_AliveList1, m_AliveList2, m_DeadList, m_Counters;
		IndirectArgsBuffer m_IndirectArgsBuffer;
		ByteAddressBuffer* m_CurrentAlive = &m_AliveList1; // 핑퐁 스왑용
		ByteAddressBuffer* m_NewAlive = &m_AliveList2;


		RootSignature m_ComputeRootSig;
		RootSignature m_GraphicsRootSig;

		ComputePSO m_KickoffPSO, m_EmitPSO, m_SimulatePSO;
		GraphicsPSO m_DrawPSO;
	};


}
