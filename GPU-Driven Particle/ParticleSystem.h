#pragma once
#include "pch.h"
#include "ParticleShared.h"
#include "GpuBuffer.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "ParticleEmitter.h"
#include "ParticleSetting.h"
#include "ShaderCompiler.h"

#include <fstream>
#include "Texture.h"

#include "BitonicSort.h"

namespace GP { class Camera; }
class GraphicsContext;
class ComputeContext;

namespace GP {

	static bool LoadDDSTexture(Texture& tex, const char* path)
	{
		// binary: 바이트 그대로 읽어라, ate: at end - 열자마자 커서 파일 끝에
		std::ifstream file(path, std::ios::binary | std::ios::ate); 
		if (!file.is_open()) return false;
		size_t size = (size_t)file.tellg(); // 지금 커서 위치 = 파일 크기
		std::vector<uint8_t> data(size); // 크기만큼 공간 확보
		file.seekg(0); // 커서를 처음으로 되감기. 이거 빼먹으면 끝에서부터 0바이트 읽음
		file.read((char*)data.data(), size); // 처음부터 size 바이트를 통으로 읽기
		return tex.CreateDDSFromMemory(data.data(), size, false); 
	}
	class ParticleSystem
	{
	public:
		void Init(uint32_t maxParticles);

		// 이미터 갱신, CB 준비
		void Update(float dt);

		void UpdateGPU(ComputeContext& cpt, const ParticleViewCB& viewParams);
		void Draw(GraphicsContext& gfx, const Camera& camera);
		void EndFrame();
		void ResetEmitter() { m_Emitter.ResetEmitter(); }
		ParticleSettings& GetSettings() { return m_Settings; } 
	private:
		// UpdateGPU의 내부 패스들 
		void BindResources(ComputeContext& cpt);   
		void KickoffPass(ComputeContext& cpt);     
		void EmitPass(ComputeContext& cpt);        
		void SimulatePass(ComputeContext& cpt);
		void SortPass(ComputeContext& cpt);
		void UpdateDrawArgs(ComputeContext& cpt);  

		uint32_t m_maxParticle = 0;
		ParticleSettings m_Settings;	
		ParticleEmitter m_Emitter{
			Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 0.0f))
		};
		ParticleFrameCB m_FrameParams = {};
		ParticleViewCB m_ViewParams = {};

		// GPU 리소스
		StructuredBuffer m_Pool;
		ByteAddressBuffer m_AliveList1, m_AliveList2, m_DeadList, m_Counters;
		IndirectArgsBuffer m_IndirectArgsBuffer;
		ByteAddressBuffer* m_CurrentAlive = &m_AliveList1; // 핑퐁 스왑용
		ByteAddressBuffer* m_NewAlive = &m_AliveList2;


		RootSignature m_ComputeRootSig;
		RootSignature m_GraphicsRootSig;

		ComputePSO m_KickoffPSO, m_EmitPSO, m_SimulatePSO;
		GraphicsPSO m_DrawAdditivePSO, m_DrawAlphaPSO;

		Texture m_SpriteTextures[(int)ETexture::Count];

		BitonicSort m_BitonicSorter;
		StructuredBuffer m_SortKeys;
	};


}
