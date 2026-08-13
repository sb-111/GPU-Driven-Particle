#pragma once
#include "ParticleSetting.h"
#include "ParticleSystem.h"
#include "MeshLibrary.h"
#include "Mesh.h"
#include "BufferManager.h"
#include "Camera.h"
#include "imgui/imgui.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>


namespace GameCore { extern HWND g_hWnd; }

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
	inline ParticleSettings MakeOverdrawPreset() // 오버드로우 측정용: 평형 생존수 고정(spawnRate x lifeTime), 랜덤 요소 제거
	{
		ParticleSettings s{};
		s.spawnRate = 10000.0f;                 // 평형 생존수 = 10000 x 2 = 20000
		s.lifeTimeMin = 2.0f;   s.lifeTimeMax = 2.0f;
		s.speedMin = 0.5f;      s.speedMax = 0.5f;
		s.spinSpeedMin = 0.0f;  s.spinSpeedMax = 0.0f;
		s.initAngleMin = 0.0f;  s.initAngleMax = 0.0f;
		s.dirSpread = 2.0f;                     // 이미터 주변 공 형태로 뭉치게
		s.randomSpawnBrightness = false;
		s.shapeType = (int)EShapeType::Point;
		s.velocityMode = (int)EVelocityMode::Velocity;
		s.sizeMode = (int)EUniformMode::Uniform;
		s.sizeMin[0] = 0.5f;    s.sizeMin[1] = 0.5f; // 큰 쿼드 = 픽셀 부하 극대화
		s.sizeOverLife = false;
		s.startColor[0] = 0.15f; s.startColor[1] = 0.18f; s.startColor[2] = 0.25f; s.startColor[3] = 1.0f;
		s.endColor[0] = 0.15f;   s.endColor[1] = 0.18f;   s.endColor[2] = 0.25f;   s.endColor[3] = 1.0f;
		s.gravity[0] = 0.0f;    s.gravity[1] = 0.0f;    s.gravity[2] = 0.0f;
		s.blendMode = (int)EBlendMode::Additive;         // 정렬 변수 제거
		s.rendererType = (int)EParticleRenderer::Sprite;
		s.textureIndex = (int)ETexture::Fire;
		return s;
	}
	// 클라이언트 영역을 정확한 픽셀 크기로 변경 (WM_SIZE -> Display::Resize 경로로 렌더 해상도까지 갱신)
	inline void SetClientSize(int width, int height)
	{
		RECT r{ 0, 0, width, height };
		AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, FALSE);
		::SetWindowPos(GameCore::g_hWnd, nullptr, 0, 0, r.right - r.left, r.bottom - r.top, SWP_NOMOVE | SWP_NOZORDER);
	}
	inline void DrawParticlePanel(ParticleSystem& system, bool& paused, Camera& camera, MeshLibrary& meshLibrary)
	{
		if (!ImGui::Begin("Particle Tuning"))
		{
			ImGui::End();
			return;
		}

		// Emitter 선택 및 추가
		static int selected = 0;
		int emitterCount = (int)system.GetEmitterCount();
		if (emitterCount == 0)
		{
			ImGui::TextDisabled("No emitters in the loaded scene");
			if (ImGui::Button("Add Emitter"))
			{
				system.AddEmitter(Math::OrthogonalTransform(Math::Vector3(0.0f, 0.0f, 0.0f)));
				selected = 0;
			}
			ImGui::End();
			return;
		}
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
		ParticleEmitter& emitter = system.GetEmitter(selected);
		ParticleSettings& s = emitter.GetSettings();
		bool restart = false;

		if (ImGui::Button("Fire")) { s = MakeFirePreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Smoke")) { s = MakeSmokePreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Sort Test")) { s = MakeArtifactPreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Ribbon")) { s = MakeRibbonPreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Overdraw")) { s = MakeOverdrawPreset(); restart = true; }
		ImGui::SameLine();
		if (ImGui::Button("Restart")) restart = true;
		ImGui::SameLine();
		ImGui::Checkbox("Pause", &paused);

		
		// 해상도 스냅 (오버드로우 측정용, 클라이언트 영역 기준)
		if (ImGui::Button("1920x1129")) SetClientSize(1920, 1129);
		ImGui::SameLine();
		if (ImGui::Button("960x564")) SetClientSize(960, 564);
		ImGui::SameLine();
		ImGui::Text("Resolution");

		// 저해상도 렌더 on/off
		ImGui::Checkbox("Half Resolution", &system.GetHalfResolution());

		Math::Vector3 camPos = camera.GetPosition();
		float camPosF[3] = { camPos.GetX(), camPos.GetY(), camPos.GetZ() };
		if (ImGui::DragFloat3("Camera Pos", camPosF, 0.1f))
		{
			Math::Vector3 newPos(camPosF[0], camPosF[1], camPosF[2]);
			camera.SetEyeAtUp(newPos, newPos + camera.GetForward(), Math::Vector3(0.0f, 1.0f, 0.0f));
		}

		float clipZ[2] = { camera.GetNearClip(), camera.GetFarClip() };
		if (ImGui::DragFloat2("Near / Far", clipZ, 0.1f, 0.0f, 0.0f, "%.2f"))
		{
			clipZ[0] = std::max(clipZ[0], 0.01f);
			clipZ[1] = std::max(clipZ[1], clipZ[0] + 0.01f); // 0 < near < far 유지
			const float aspect =
				static_cast<float>(Graphics::g_SceneColorBuffer.GetHeight()) /
				static_cast<float>(Graphics::g_SceneColorBuffer.GetWidth());
			camera.SetPerspective(camera.GetFOV(), aspect, clipZ[0], clipZ[1]);
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
		static const char* kRibbonUVModeNames[(int)ERibbonUVMode::Count] = { "Stretch", "Tile" };
	
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
				Math::Vector3 basePos = emitter.GetBasePosition();
				float basePosF[3] = { basePos.GetX(), basePos.GetY(), basePos.GetZ() };
				if (ImGui::DragFloat3("Position", basePosF, 0.05f))
					emitter.SetBasePosition(Math::Vector3(basePosF[0], basePosF[1], basePosF[2]));

				float baseRotF[3] = { emitter.GetBaseRotationEuler()[0], emitter.GetBaseRotationEuler()[1], emitter.GetBaseRotationEuler()[2] };
				if (ImGui::DragFloat3("Rotation", baseRotF, 1.0f, -360.0f, 360.0f))
					emitter.SetBaseRotationEuler(baseRotF);

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
				ImGui::Checkbox("Collision", &s.collisionEnabled);
				if (s.collisionEnabled)
				{
					ImGui::SliderFloat("Restitution", &s.restitution, 0.0f, 1.0f);
					ImGui::SliderFloat("Friction", &s.friction, 0.0f, 1.0f);
				}
				ImGui::Checkbox("Force Avoid", &s.forceAvoidEnabled);
				if (s.forceAvoidEnabled)
					ImGui::SliderFloat("Avoid Strength", &s.forceAvoidStrength, 0.0f, 50.0f);
				ImGui::Checkbox("Force Tangent", &s.forceTangentEnabled);
				if (s.forceTangentEnabled)
				{
					ImGui::SliderFloat("Tangent Strength", &s.forceTangentStrength, 0.0f, 50.0f);
					ImGui::DragFloat3("Tangent Axis", s.forceTangentAxis, 0.01f, -1.0f, 1.0f);
				}
				ImGui::Checkbox("Force Curl", &s.forceCurlEnabled);
				if (s.forceCurlEnabled)
				{
					ImGui::SliderFloat("Curl Frequency", &s.curlFrequency, 0.05f, 4.0f);
					ImGui::SliderFloat("Curl Target Speed", &s.curlTargetSpeed, 0.0f, 50.0f);
					ImGui::SliderFloat("Curl Response Rate", &s.curlResponseRate, 0.1f, 20.0f);
					ImGui::Checkbox("Curl Boundary: Psi (Bridson)", &s.curlPsiBoundary);
				}
				if (s.forceAvoidEnabled || s.forceTangentEnabled || s.forceCurlEnabled)
					ImGui::SliderFloat("Surface Influence Radius", &s.surfaceInfluenceRadius, 0.1f, 10.0f);
				ImGui::Checkbox("Force Attract", &s.forceAttractEnabled);
				if (s.forceAttractEnabled)
				{
					ImGui::SliderFloat("Attract Strength", &s.forceAttractStrength, 0.0f, 50.0f);
					ImGui::SliderInt("Attract Target SDF", &s.forceAttractTarget, 0, MAX_SDF_COUNT - 1);
				}
				ImGui::Checkbox("Morph", &s.morphEnabled);
				if (s.morphEnabled)
				{
					ImGui::SliderFloat("Morph Strength", &s.morphStrength, 0.0f, 50.0f);
					ImGui::DragFloat3("Target Pos", s.morphTargetPosition, 0.1f);
					ImGui::DragFloat3("Target Rotation", s.morphTargetRotation, 1.0f, -360.0f, 360.0f);
					ImGui::DragFloat3("Target Scale", s.morphTargetScale, 0.05f, 0.01f, 20.0f);
				}
			}
			if (g == (int)EParamGroup::Emit)
			{
				ImGui::Checkbox("Random Spawn Brightness", &s.randomSpawnBrightness);
				ImGui::Combo("Shape Type", &s.shapeType, kShapeNames, (int)EShapeType::Cone); // Cone(위치 방출)은 TODO
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
				// 리본 전용 설정
				if (s.rendererType == (int)EParticleRenderer::Ribbon)
					ImGui::Combo("UV Mode", &s.ribbonUVMode, kRibbonUVModeNames, (int)ERibbonUVMode::Count);
				// 메시 전용 설정
				if (s.rendererType == (int)EParticleRenderer::Mesh)
				{
					const std::string& currentMeshPath = emitter.GetParticleMeshPath();
					if (ImGui::BeginCombo("Particle Mesh", currentMeshPath.empty() ? "Cube (default)" : currentMeshPath.c_str()))
					{
						if (ImGui::Selectable("Cube (default)", currentMeshPath.empty()))
							emitter.SetParticleMesh(nullptr, "");
						for (const std::string& assetPath : meshLibrary.GetAssetPaths())
						{
							if (ImGui::Selectable(assetPath.c_str(), assetPath == currentMeshPath))
							{
								Mesh* particleMesh = meshLibrary.Get(assetPath.c_str());
								if (particleMesh != nullptr)
									emitter.SetParticleMesh(particleMesh, assetPath);
							}
						}
						ImGui::EndCombo();
					}
				}
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

		if (ImGui::CollapsingHeader("Morph Target", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Current: %s", emitter.GetMorphTargetPath().empty() ? "none" : emitter.GetMorphTargetPath().c_str());

			static char morphPathBuf[260] = "Meshes/stanford-bunny.obj";
			static int morphSampleCount = 32768;
			static int morphSeed = 324;
			static char morphError[128] = "";
			if (ImGui::BeginCombo("Mesh##Morph", morphPathBuf))
			{
				for (const std::string& assetPath : meshLibrary.GetAssetPaths())
				{
					if (ImGui::Selectable(assetPath.c_str(), assetPath == morphPathBuf))
						strcpy_s(morphPathBuf, assetPath.c_str());
				}
				ImGui::EndCombo();
			}
			ImGui::InputInt("Sample Count", &morphSampleCount);
			ImGui::InputInt("Seed", &morphSeed);
			if (ImGui::Button("Set Target"))
			{
				morphError[0] = '\0';
				Mesh* mesh = (morphSampleCount > 0) ? meshLibrary.Get(morphPathBuf) : nullptr;
				if (mesh == nullptr || mesh->GetCPUVertices().empty() || mesh->GetCPUIndices().empty())
				{
					sprintf_s(morphError, "load failed: %s", morphPathBuf);
				}
				else
				{
					emitter.SetMorphTarget(system.ResolveSurfaceMorphTarget(
						morphPathBuf, *mesh, (uint32_t)morphSampleCount, (uint32_t)morphSeed));
					emitter.SetMorphTargetPath(morphPathBuf);
					emitter.SetMorphTargetSampleCount((uint32_t)morphSampleCount);
					emitter.SetMorphTargetSeed((uint32_t)morphSeed);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear Target"))
			{
				morphError[0] = '\0';
				emitter.SetMorphTarget(nullptr);
				emitter.SetMorphTargetPath("");
				emitter.SetMorphTargetSampleCount(0);
				emitter.SetMorphTargetSeed(0);
			}
			if (morphError[0] != '\0')
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", morphError);
		}

		if (ImGui::CollapsingHeader("Sequence", ImGuiTreeNodeFlags_DefaultOpen))
		{
			EmitterSequence& seq = emitter.GetSequence();
			if (seq.stages.empty())
			{
				ImGui::TextDisabled("No sequence");
				if (ImGui::Button("Add Stage From Current"))
				{
					SequenceStage stage;
					stage.duration = 5.0f;
					stage.settings = s;
					seq.stages.push_back(stage);
					seq.currentStage = 0;
					seq.stageTimer = 0.0f;
					seq.playing = true;
					emitter.ApplyStage();
				}
			}
			else
			{
				if (ImGui::Button(seq.playing ? "Pause##Seq" : "Play##Seq"))
					seq.playing = !seq.playing;
				ImGui::SameLine();
				if (ImGui::Button("Restart##Seq"))
				{
					seq.currentStage = 0;
					seq.stageTimer = 0.0f;
					seq.playing = true;
					emitter.ApplyStage();
				}
				ImGui::SameLine();
				if (ImGui::Button("Next##Seq"))
				{
					if (seq.currentStage + 1 < seq.stages.size())
						seq.currentStage++;
					else if (seq.loop)
						seq.currentStage = 0;
					seq.stageTimer = 0.0f;
					emitter.ApplyStage();
				}
				ImGui::SameLine();
				ImGui::Checkbox("Loop##Seq", &seq.loop);

				for (size_t i = 0; i < seq.stages.size(); ++i)
				{
					char label[64];
					sprintf_s(label, "Stage %d (%.1fs)%s", (int)i, seq.stages[i].duration,
						seq.stages[i].replacesMorphTarget ? " [morph]" : "");
					if (ImGui::Selectable(label, i == seq.currentStage))
					{
						seq.currentStage = i;
						seq.stageTimer = 0.0f;
						emitter.ApplyStage();
					}
				}

				SequenceStage& cur = seq.stages[seq.currentStage];
				if (cur.duration > 0.0f)
					ImGui::ProgressBar(seq.stageTimer / cur.duration, ImVec2(-1.0f, 0.0f));
				else
					ImGui::TextDisabled("Duration 0 = hold");
				ImGui::DragFloat("Duration##Seq", &cur.duration, 0.1f, 0.0f, 3600.0f, "%.1f");

				// 패널의 라이브 settings를 현재 스테이지 사본에 저장
				if (ImGui::Button("Capture To Stage"))
					cur.settings = s;
				ImGui::SameLine();
				if (ImGui::Button("Add Stage From Current"))
				{
					SequenceStage stage;
					stage.duration = 5.0f;
					stage.settings = s;
					stage.morphTarget = seq.stages.back().morphTarget; // 타깃은 마지막 스테이지에서 가져오기
					seq.stages.push_back(stage);
					seq.currentStage = seq.stages.size() - 1;
					seq.stageTimer = 0.0f;
					emitter.ApplyStage();
				}
				ImGui::SameLine();
				if (ImGui::Button("Delete Stage"))
				{
					seq.stages.erase(seq.stages.begin() + seq.currentStage);
					if (seq.currentStage >= seq.stages.size() && seq.currentStage > 0)
						seq.currentStage--;
					seq.stageTimer = 0.0f;
					if (!seq.stages.empty())
						emitter.ApplyStage();
				}
				ImGui::SameLine();
				if (ImGui::Button("Clear Sequence"))
				{
					seq.stages.clear();
					seq.currentStage = 0;
					seq.stageTimer = 0.0f;
					seq.playing = true;
				}
			}
		}

		if (ImGui::CollapsingHeader("Scene Collision", ImGuiTreeNodeFlags_DefaultOpen))
		{
			CollisionSettings& c = system.GetCollisionSettings();
			ImGui::Checkbox("Plane", &c.planeEnabled);
			if (c.planeEnabled)
			{
				ImGui::SliderFloat3("Plane Normal", c.planeNormal, -1.0f, 1.0f);
				ImGui::SliderFloat("Plane Offset", &c.planeOffset, -5.0f, 5.0f);
			}
			ImGui::Checkbox("Sphere", &c.sphereEnabled);
			if (c.sphereEnabled)
			{
				ImGui::DragFloat3("Sphere Center", c.sphereCenter, 0.1f);
				ImGui::SliderFloat("Collider Radius", &c.sphereRadius, 0.1f, 5.0f);
			}
		}

		ImGui::End();

		// 프리셋/재시작 버튼 눌린 경우, 선택된 Emitter만 리셋
		if (restart)
			emitter.ResetEmitter();
	}
}
