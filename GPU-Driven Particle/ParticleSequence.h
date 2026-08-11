#pragma once
#include "ParticleSetting.h"

#include <string>
#include <vector>

namespace GP
{
	class MorphTargetSet;

	// 시퀀스 스테이지 한개
	struct SequenceStage
	{
		float duration = 0.0f; // 스테이지 유지 시간

		bool replacesMorphTarget = false;	// JSON에 morph 내용이 있었는지
		std::string morphMeshPath;
		uint32_t morphSampleCount = 0;
		uint32_t morphSeed = 0;
		MorphTargetSet* morphTarget = nullptr;

		ParticleSettings settings;
	};

	struct EmitterSequence
	{
		std::vector<SequenceStage> stages;	// 비어 있으면 시퀀스 없음
		bool loop = false;

		// 재생 상태 (로드 시 항상 0부터)
		size_t currentStage = 0; // 현재 스테이지에서 얼마나 지났는지
		float stageTimer = 0.0f;
		bool playing = true;
	};
}
