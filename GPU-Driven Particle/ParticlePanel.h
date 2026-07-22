#pragma once
#include "ParticleSetting.h"
#include "ParticleSystem.h"
#include "Camera.h"
#include "imgui/imgui.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>


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
		bool (*visibleIf)(const ParticleSettings&) = nullptr;
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
		{ "Spin Speed Min", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, spinSpeedMin), 0.0f,  360.0f, "%.3f", 0,
			[](const ParticleSettings& s) { return s.alignmentMode == (int)EAlignmentMode::UnAligned; } },
		{ "Spin Speed Max", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, spinSpeedMax), 0.0f,  360.0f, "%.3f", 0,
			[](const ParticleSettings& s) { return s.alignmentMode == (int)EAlignmentMode::UnAligned; } },
		{ "Init Angle Min", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, initAngleMin), 0.0f,  360.0f, "%.3f", 0,
			[](const ParticleSettings& s) { return s.alignmentMode == (int)EAlignmentMode::UnAligned; } },
		{ "Init Angle Max", EParamGroup::Emit,   EParamType::Float,  offsetof(ParticleSettings, initAngleMax), 0.0f,  360.0f, "%.3f", 0,
			[](const ParticleSettings& s) { return s.alignmentMode == (int)EAlignmentMode::UnAligned; } },
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
		s.endColor[0] = 0.35f;   s.endColor[1] = 0.35f;   s.endColor[2] = 0.35f;   s.endColor[3] = 0.0f; // a=0: 수명 끝 페이드아웃
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
		s.endColor[0] = 0.0f;   s.endColor[1] = 0.0f;   s.endColor[2] = 1.0f;   s.endColor[3] = 0.0f;   // 파랑, a=0: 수명 끝 페이드아웃
		s.blendMode = (int)EBlendMode::Alpha;
		s.textureIndex = (int)ETexture::Smoke;
		return s;
	}
	inline ParticleSettings MakeRibbonPreset() // 리본 검증용
	{
		ParticleSettings s{};
		s.spawnRate = 60.0f;
		s.lifeTimeMin = 2.0f;   s.lifeTimeMax = 2.0f;   // 고정 수명: 중간에 끊겨서 팝핑 방지
		s.speedMin = 4.0f;      s.speedMax = 4.0f;      // 고정 속도
		s.spinSpeedMin = 0.0f;  s.spinSpeedMax = 0.0f;
		s.initAngleMin = 0.0f;  s.initAngleMax = 0.0f;
		s.dirSpread = 0.0f;
		s.randomSpawnBrightness = false;
		s.shapeType = (int)EShapeType::Point;
		s.velocityMode = (int)EVelocityMode::Velocity;
		s.sizeMode = (int)EUniformMode::Uniform;
		s.sizeMin[0] = 0.2f;    s.sizeMin[1] = 0.2f;
		s.sizeOverLife = true;  
		s.startColor[0] = 1.0f; s.startColor[1] = 0.6f; s.startColor[2] = 0.2f; s.startColor[3] = 1.0f;
		s.endColor[0] = 1.0f;   s.endColor[1] = 0.2f;   s.endColor[2] = 0.0f;   s.endColor[3] = 0.0f;
		s.gravity[0] = 0.0f;    s.gravity[1] = -9.8f;   s.gravity[2] = 0.0f;
		s.blendMode = (int)EBlendMode::Additive;
		s.rendererType = (int)EParticleRenderer::Ribbon;
		s.textureIndex = (int)ETexture::Fire;
		return s;
	}
	inline void DrawParticlePanel(ParticleSystem& system, bool& paused, Camera& camera)
	{
		if (!ImGui::Begin("Particle Tuning"))
		{
			ImGui::End();
			return;
		}

		// Emitter 선택 및 추가
		static int selected = 0;
		int emitterCount = (int)system.GetEmitterCount();
		if (selected >= emitterCount) selected = emitterCount - 1;

		char curName[32];
		sprintf_s(curName, "Emitter %d", selected); // Emitter 네임
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::BeginCombo("##EmitterSelect", curName))
		{
			for (int i = 0; i < emitterCount; ++i)
			{
				char name[32];
				sprintf_s(name, "Emitter %d", i);
				// 클릭되면 true -> selected 변경
				if (ImGui::Selectable(name, i == selected)) selected = i;
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Add Emitter") && emitterCount < 8)
		{
			// Emitter 추가
			system.AddEmitter(Math::OrthogonalTransform(Math::Vector3(3.0f * emitterCount, 0.0f, 0.0f)));
			selected = emitterCount;
		}
		// 선택된 Emitter의 Settings를 편집
		ParticleSettings& s = system.GetEmitter(selected).GetSettings();
		bool restart = false;

		if (ImGui::Button("Fire")) { s = MakeFirePreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Smoke")) { s = MakeSmokePreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Sort Test")) { s = MakeArtifactPreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Ribbon")) { s = MakeRibbonPreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Restart")) restart = true;
		ImGui::SameLine();
		ImGui::Checkbox("Pause", &paused);

		
		Math::Vector3 camPos = camera.GetPosition();
		float camPosF[3] = { camPos.GetX(), camPos.GetY(), camPos.GetZ() };
		if (ImGui::DragFloat3("Camera Pos", camPosF, 0.1f))
		{
			Math::Vector3 newPos(camPosF[0], camPosF[1], camPosF[2]);
			camera.SetEyeAtUp(newPos, newPos + camera.GetForward(), Math::Vector3(0.0f, 1.0f, 0.0f));
		}

		ImGui::Separator();

		static const char* kGroupNames[(int)EParamGroup::Count] = { "Emitter", "Emit", "Simulate", "Renderer" };
		static const char* kSizeModeNames[(int)EUniformMode::Count] = { "Uniform", "Random Uniform", "Non Uniform", "Random Non Uniform" };
		static const char* kBlendModeNames[(int)EBlendMode::Count] = { "Additive", "Alpha"};
		static const char* kTextureNames[(int)ETexture::Count] = { "Fire", "Smoke", "Spark", "Boom (8x8)", "Explosion (4x4)" };
		static const char* kShapeNames[(int)EShapeType::Count] = { "Point", "Box", "Sphere", "Cone" };
		static const char* kVelocityNames[(int)EVelocityMode::Count] = { "Velocity", "Velocity From Point", "Velocity In Cone"};
		static const char* kLoopModeNames[(int)ELoopMode::Count] = { "Infinite", "Once", "Multiple"};
		static const char* kAlignmentModeNames[(int)EAlignmentMode::Count] = { "Unaligned", "Velocity aligned"};
		static const char* kRendererNames[(int)EParticleRenderer::Count] = { "Sprite", "Mesh", "Ribbon"};
	
		for (int g = 0; g < (int)EParamGroup::Count; ++g)
		{
			// 접혀있으면(false) 스킵
			if (!ImGui::CollapsingHeader(kGroupNames[g], ImGuiTreeNodeFlags_DefaultOpen))
				continue;

			for (const UIParamDesc& p : g_ParticleParams)
			{
				if ((int)p.group != g) // 같은 그룹 아니면 스킵
					continue;
				if (p.visibleIf && !p.visibleIf(s)) // 보이지 않아야 하면 스킵
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
				ImGui::Checkbox("Orbit", &s.orbitEnabled);
				if (s.orbitEnabled)
				{
					ImGui::SliderFloat("Orbit Radius", &s.orbitRadius, 0.1f, 10.0f);
					ImGui::SliderFloat("Orbit Speed", &s.orbitSpeed, 0.1f, 10.0f, "%.2f rad/s");
				}
			}
			if (g == (int)EParamGroup::Simulate)
			{
				ImGui::Checkbox("Size Over Life", &s.sizeOverLife);
			}
			if (g == (int)EParamGroup::Emit)
			{
				ImGui::Checkbox("Random Spawn Brightness", &s.randomSpawnBrightness);
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
				// 메시 회전 (Rotation Rate 방식)
				if (s.rendererType == (int)EParticleRenderer::Mesh)
				{
					ImGui::SliderFloat2("Rotation Rate", &s.rotationRateMin, 0.0f, 720.0f, "%.0f deg/s");
					ImGui::Checkbox("Random Rotation Axis", &s.randomRotationAxis);
					if (!s.randomRotationAxis)
						ImGui::SliderFloat3("Rotation Axis", s.rotationAxis, -1.0f, 1.0f);
					ImGui::Checkbox("Random Init Orientation", &s.randomInitOrientation);
				}
			}
			if (g == (int)EParamGroup::Renderer)
			{
				ImGui::Combo("Renderer##type", &s.rendererType, kRendererNames, (int)EParticleRenderer::Count);
				ImGui::Combo("Blend Mode", &s.blendMode, kBlendModeNames, (int)EBlendMode::Count);
				if (s.blendMode == (int)EBlendMode::Alpha || s.rendererType == (int)EParticleRenderer::Ribbon)
					ImGui::Checkbox("Sort", &s.sortEnabled);
				// 스프라이트 전용 설정
				if (s.rendererType == (int)EParticleRenderer::Sprite)
				{
					ImGui::Combo("Alignment Mode", &s.alignmentMode, kAlignmentModeNames, (int)EAlignmentMode::Count);
					ImGui::Combo("Texture", &s.textureIndex, kTextureNames, (int)ETexture::Count);
					ImGui::SliderInt("SubImages X", &s.subImagesX, 1, 16);
					ImGui::SliderInt("SubImages Y", &s.subImagesY, 1, 16);
				}
			}
		}

		ImGui::End();

		// 프리셋/재시작 버튼 눌린 경우, 선택된 Emitter만 리셋
		if (restart)
			system.GetEmitter(selected).ResetEmitter();
	}
}
