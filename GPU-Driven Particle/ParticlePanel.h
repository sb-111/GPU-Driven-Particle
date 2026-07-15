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
		const char* format = "%.3f";
		ImGuiSliderFlags flags = 0;
	};

	// g_ParticleParams 그룹별로 순회하며 위젯 생성
	inline const UIParamDesc g_ParticleParams[] =
	{
		{ "Spawn Rate",    EParamGroup::Emitter,  EParamType::Float,  offsetof(ParticleSettings, spawnRate),    0.0f,  1000000.0f , "%.0f", ImGuiSliderFlags_Logarithmic},
		{ "Burst Count",   EParamGroup::Emitter,  EParamType::Int,    offsetof(ParticleSettings, burstCount),   0.0f,  1000000.0f, "%.0f", ImGuiSliderFlags_Logarithmic },

		{ "LifeTime Min",  EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, lifeTimeMin),  0.05f, 10.0f },
		{ "LifeTime Max",  EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, lifeTimeMax),  0.05f, 10.0f },
		{ "Speed Min",     EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, speedMin),     0.0f,  30.0f },
		{ "Speed Max",     EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, speedMax),     0.0f,  30.0f },
		{ "Spin Speed Min", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, spinSpeedMin), 0.0f,  360.0f  },
		{ "Spin Speed Max", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, spinSpeedMax), 0.0f,  360.0f  },
		{ "Init Angle Min", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, initAngleMin), 0.0f,  360.0f  },
		{ "Init Angle Max", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, initAngleMax), 0.0f,  360.0f  },
		{ "Dir Spread",    EParamGroup::Emit,    EParamType::Float,  offsetof(ParticleSettings, dirSpread),    0.0f,  5.0f },
		{ "Start Color",   EParamGroup::Emit,    EParamType::Color4, offsetof(ParticleSettings, startColor),   0.0f,  1.0f },

		{ "Gravity",       EParamGroup::Simulate,   EParamType::Float3, offsetof(ParticleSettings, gravity),     -20.0f, 20.0f },
		{ "End Color",     EParamGroup::Simulate,   EParamType::Color4, offsetof(ParticleSettings, endColor),     0.0f,  1.0f },
	};

	inline ParticleSettings MakeFirePreset()
	{
		return ParticleSettings{};
	}

	inline ParticleSettings MakeSmokePreset()
	{
		ParticleSettings s{};
		s.spawnRate = 30.0f;
		s.lifeTimeMin = 4.0f;   s.lifeTimeMax = 5.0f;
		s.speedMin = 0.5f;      s.speedMax = 1.0f;
		s.spinSpeedMin = 0.0f;  s.spinSpeedMax = 20.0f;
		s.initAngleMin = 0.0f;  s.initAngleMax = 360.0f;
		s.sizeMode = (int)EUniformMode::Uniform;
		s.sizeMin[0] = 0.45f;   s.sizeMin[1] = 0.45f;
		s.dirSpread = 0.4f;
		s.shapeType = (int)EShapeType::Box;
		s.boxExtents[0] = 0.3f; s.boxExtents[1] = 0.3f; s.boxExtents[2] = 0.3f;
		s.startColor[0] = 0.7f; s.startColor[1] = 0.7f; s.startColor[2] = 0.7f; s.startColor[3] = 1.0f; 
		s.gravity[0] = 0.0f;    s.gravity[1] = 0.0f;    s.gravity[2] = 0.0f;
		s.endColor[0] = 0.35f;   s.endColor[1] = 0.35f;   s.endColor[2] = 0.35f;   s.endColor[3] = 1.0f;
		s.blendMode = (int)EBlendMode::Alpha;
		s.textureIndex = (int)ETexture::Smoke;
		return s;
	}
	inline ParticleSettings MakeArtifactPreset() // 미정렬 문제 아티팩트 재현용
	{
		ParticleSettings s{};
		s.spawnRate = 15.0f;
		s.burstCount = 200000;
		s.loopMode = (int)ELoopMode::Infinite;
		s.loopDuration = 1.7f;
		s.lifeTimeMin = 4.0f;   s.lifeTimeMax = 5.0f;
		s.speedMin = 0.5f;      s.speedMax = 1.0f;
		s.spinSpeedMin = 0.0f;  s.spinSpeedMax = 20.0f;
		s.initAngleMin = 0.0f;  s.initAngleMax = 360.0f;
		s.sizeMode = (int)EUniformMode::Uniform;
		s.sizeMin[0] = 0.45f;   s.sizeMin[1] = 0.45f; // 나중에 작은 걸로 보고 싶으면 슬라이더에서 0.1f로 조정(uniform)
		s.dirSpread = 0.945f;
		s.shapeType = (int)EShapeType::Box;
		s.boxExtents[0] = 0.3f; s.boxExtents[1] = 0.3f; s.boxExtents[2] = 0.3f;
		s.startColor[0] = 1.0f; s.startColor[1] = 0.0f; s.startColor[2] = 0.0f; s.startColor[3] = 1.0f; // 빨강
		s.gravity[0] = 0.0f;    s.gravity[1] = 0.0f;    s.gravity[2] = 0.0f;
		s.endColor[0] = 0.0f;   s.endColor[1] = 0.0f;   s.endColor[2] = 1.0f;   s.endColor[3] = 1.0f;   // 파랑
		s.blendMode = (int)EBlendMode::Alpha;
		s.textureIndex = (int)ETexture::Smoke;
		return s;
	}
	inline bool DrawParticlePanel(ParticleSettings& s, bool& paused)
	{
		bool restart = false;

		if (!ImGui::Begin("Particle Tuning"))
		{
			ImGui::End();
			return restart;
		}

		if (ImGui::Button("Fire")) { s = MakeFirePreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Smoke")) { s = MakeSmokePreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Sort Test")) { s = MakeArtifactPreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Restart")) restart = true;
		ImGui::SameLine();
		ImGui::Checkbox("Pause", &paused);

		ImGui::Separator();

		static const char* kGroupNames[(int)EParamGroup::Count] = { "Emitter", "Emit", "Simulate", "Renderer" };
		static const char* kSizeModeNames[(int)EUniformMode::Count] = { "Uniform", "Random Uniform", "Non Uniform", "Random Non Uniform" };
		static const char* kBlendModeNames[(int)EBlendMode::Count] = { "Additive", "Alpha"};
		static const char* kTextureNames[(int)ETexture::Count] = { "Fire", "Smoke", "Spark" };
		static const char* kShapeNames[(int)EShapeType::Count] = { "Point", "Box", "Sphere", "Cone" };
		static const char* kVelocityNames[(int)EVelocityMode::Count] = { "Velocity", "Velocity From Point", "Velocity In Cone"};
		static const char* kLoopModeNames[(int)ELoopMode::Count] = { "Infinite", "Once", "Multiple"};
		static const char* kAlignmentModeNames[(int)EAlignmentMode::Count] = { "Unaligned", "Velocity aligned"};
	
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
				case EParamType::Float:  ImGui::SliderFloat(p.name, (float*)ptr, p.minV, p.maxV, p.format, p.flags); break;
				case EParamType::Int:    ImGui::SliderInt(p.name, (int*)ptr, (int)p.minV, (int)p.maxV); break;
				case EParamType::Color4: ImGui::ColorEdit4(p.name, (float*)ptr); break;
				case EParamType::Float3: ImGui::SliderFloat3(p.name, (float*)ptr, p.minV, p.maxV); break;
				}
			}
			if (g == (int)EParamGroup::Emitter)
			{
				ImGui::Combo("Loop Mode", &s.loopMode, kLoopModeNames, (int)ELoopMode::Count);
				ImGui::SliderFloat("Loop Duration", &s.loopDuration, 0.1f, 10.0f, "%.2f s");
				if ((ELoopMode)s.loopMode == ELoopMode::Multiple)
					ImGui::SliderInt("Loop Count", &s.loopCount, 1, 20);
			}
			if (g == (int)EParamGroup::Emit)
			{
				ImGui::Combo("Shape Type", &s.shapeType, kShapeNames, (int)EShapeType::Sphere);
				switch ((EShapeType)s.shapeType)
				{
				case EShapeType::Point:
					break;                                   
				case EShapeType::Box:
					ImGui::SliderFloat3("Box Extents", s.boxExtents, 0.0f, 20.0f);
					break;
				case EShapeType::Sphere:
					ImGui::SliderFloat("Sphere Radius", &s.sphereRadius, 0.0f, 10.0f);
					ImGui::Checkbox("Surface Only", &s.sphereSurfaceOnly);
					break;
				}
				ImGui::Combo("Velocity Mode", &s.velocityMode, kVelocityNames, (int)EVelocityMode::Count);
				if ((EVelocityMode)s.velocityMode == EVelocityMode::VelocityInCone)
					ImGui::SliderFloat("Cone Angle", &s.coneAngle, 0.0f, 89.0f);
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
			if (g == (int)EParamGroup::Renderer)
			{
				ImGui::Combo("Blend Mode", &s.blendMode, kBlendModeNames, (int)EBlendMode::Count);
				ImGui::Combo("Alignment Mode", &s.alignmentMode, kAlignmentModeNames, (int)EAlignmentMode::Count);
				ImGui::Combo("Texture", &s.textureIndex, kTextureNames, (int)ETexture::Count);
			}
		}

		ImGui::End();
		return restart;
	}
}
