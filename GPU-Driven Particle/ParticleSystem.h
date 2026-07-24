#pragma once
#include "pch.h"
#include "ParticleShared.h"
#include "ParticleEmitter.h"
#include "ParticleSetting.h"
#include "ShaderCompiler.h"

#include <fstream>
#include <memory>
#include "Texture.h"

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
		void Draw(GraphicsContext& gfx, const ParticleViewCB& viewCB);
		void EndFrame();

		size_t GetEmitterCount() const { return m_Emitters.size(); }
		ParticleEmitter& GetEmitter(size_t i) { return *m_Emitters[i]; }

		void CompositeToMain(GraphicsContext& gfx);
	private:
		void InitSharedResources();

		uint32_t m_maxParticle = 0;
		ParticleSharedResources m_Shared;
		std::vector<std::unique_ptr<ParticleEmitter>> m_Emitters;
	};


}
