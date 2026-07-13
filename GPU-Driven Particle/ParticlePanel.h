#pragma once
#include "ParticleSetting.h"
#include "imgui/imgui.h"
#include <cstddef>
#include <cstdint>


namespace GP
{
	enum class EParamGroup : int { Emitter, Emit, Simulate, Renderer, Count };
	enum class EParamType : int { Float, Int, Color4, Float3 };

	// ImGui 위젯 하나 정보
	struct UIParamDesc	
	{
		const char* name;	// 문자열 리터럴
		EParamGroup group;
		EParamType type;
		size_t offset;	// ParticleSettings 안에서의 위치
		float minV;		// 최소 값
		float maxV;		// 최대 값
	};

	// g_ParticleParams 그룹별로 순회하며 위젯 생성
	inline const UIParamDesc g_ParticleParams[] =
	{
		{ "Spawn Rate",    EParamGroup::Emitter,  EParamType::Float,  offsetof(ParticleSettings, spawnRate),    0.0f,  10000.0f },
		{ "Burst Count",   EParamGroup::Emitter,  EParamType::Int,    offsetof(ParticleSettings, burstCount),   0.0f,  200000.0f },

		{ "LifeTime Min",  EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, lifeTimeMin),  0.05f, 10.0f },
		{ "LifeTime Max",  EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, lifeTimeMax),  0.05f, 10.0f },
		{ "Speed Min",     EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, speedMin),     0.0f,  30.0f },
		{ "Speed Max",     EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, speedMax),     0.0f,  30.0f },
		{ "Spin Speed Min", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, spinSpeedMin), 0.0f,  360.0f  },
		{ "Spin Speed Max", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, spinSpeedMax), 0.0f,  360.0f  },
		{ "Init Angle Min", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, initAngleMin), 0.0f,  360.0f  },
		{ "Init Angle Max", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, initAngleMax), 0.0f,  360.0f  },
		{ "Dir Spread",    EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, dirSpread),    0.0f,  5.0f },
		{ "Pos Spread",    EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, posSpread),    0.0f,  10.0f },
		{ "Start Color",   EParamGroup::Emit,    EParamType::Color4, offsetof(ParticleSettings, startColor),   0.0f,  1.0f },

		{ "Gravity",       EParamGroup::Simulate,   EParamType::Float3, offsetof(ParticleSettings, gravity),     -20.0f, 20.0f },
		{ "End Color",     EParamGroup::Simulate,   EParamType::Color4, offsetof(ParticleSettings, endColor),     0.0f,  1.0f },
	};

	inline ParticleSettings MakeFirePreset()
	{
		return ParticleSettings{};
	}

	inline void DrawParticlePanel(ParticleSettings& s)
	{
		if (!ImGui::Begin("Particle Tuning"))
		{
			ImGui::End();
			return;
		}

		if (ImGui::Button("Fire")) s = MakeFirePreset();

		ImGui::Separator();

		static const char* kGroupNames[(int)EParamGroup::Count] = { "Emitter", "Emit", "Simulate", "Renderer" };
		static const char* kSizeModeNames[(int)EUniformMode::Count] = { "Uniform", "Random Uniform", "Non Uniform", "Random Non Uniform" };

		for (int g = 0; g < (int)EParamGroup::Count; ++g)
		{
			// 접혀있으면(false) 스킵
			if (!ImGui::CollapsingHeader(kGroupNames[g], ImGuiTreeNodeFlags_DefaultOpen))
				continue;

			for (const UIParamDesc& p : g_ParticleParams)
			{
				if ((int)p.group != g) // 같은 그룹 아니면 스킵
					continue;
				void* ptr = (uint8_t*)&s + p.offset;	// 인스턴스 시작 주소 + 필드 offset
				switch (p.type)
				{
				case EParamType::Float:  ImGui::SliderFloat(p.name, (float*)ptr, p.minV, p.maxV); break;
				case EParamType::Int:    ImGui::SliderInt(p.name, (int*)ptr, (int)p.minV, (int)p.maxV); break;
				case EParamType::Color4: ImGui::ColorEdit4(p.name, (float*)ptr); break;
				case EParamType::Float3: ImGui::SliderFloat3(p.name, (float*)ptr, p.minV, p.maxV); break;
				}
			}
			if (g == (int)EParamGroup::Renderer)
			{
				ImGui::Combo("Size Mode", &s.sizeMode, kSizeModeNames, (int)EUniformMode::Count);
				switch ((EUniformMode)s.sizeMode)
				{
				case EUniformMode::Uniform: ImGui::SliderFloat("Size", &s.sizeMin[0], 0.001f, 0.5f); break;
				case EUniformMode::RandomUniform:
					ImGui::SliderFloat("Size Min", &s.sizeMin[0], 0.001f, 0.5f); 
					ImGui::SliderFloat("Size Max", &s.sizeMax[0], 0.001f, 0.5f);
					break;
				case EUniformMode::NonUniform:
					ImGui::SliderFloat2("Size XY", s.sizeMin, 0.001f, 0.5f);
					break;
				case EUniformMode::RandomNonUniform:
					ImGui::SliderFloat2("Size Min XY", s.sizeMin, 0.001f, 0.5f);
					ImGui::SliderFloat2("Size Max XY", s.sizeMax, 0.001f, 0.5f);
					break;
				}
			}
		}

		ImGui::End();
	}
}
