#include "ParticleEmitter.h"
#include "TextureLibrary.h"
#include "CommandContext.h"
#include "MathConvert.h"
#include "GraphicsCommon.h"
#include "CubeMesh.h"
#include "Mesh.h"
void GP::ParticleEmitter::Init(uint32_t maxParticles, ParticleSharedResources* shared, uint32_t index)
{
	m_maxParticle = maxParticles;
	m_Shared = shared;
	m_BasePosition = m_EmitterTransform.GetTranslation();

	// 버퍼 초기화
	std::wstring tag = L" " + std::to_wstring(index); // Emitter 인덱스 
	std::vector<Particle> zeroInit(maxParticles);
	m_Pool.Create(L"Particle Pool" + tag, maxParticles, sizeof(Particle), zeroInit.data());
	m_AliveList1.Create(L"Alive1" + tag, maxParticles, sizeof(uint32_t));
	m_AliveList2.Create(L"Alive2" + tag, maxParticles, sizeof(uint32_t));
	std::vector<uint32_t> deadInit(maxParticles);
	for (uint32_t i = 0; i < maxParticles; i++)
	{
		deadInit[i] = i;
	}
	m_DeadList.Create(L"Dead" + tag, maxParticles, sizeof(uint32_t), deadInit.data());

	std::vector<uint32_t> counterInit = { 0, maxParticles, 0, 0 }; // alive, dead, realEmit, afterSim
	m_Counters.Create(L"Counters" + tag, 4, sizeof(uint32_t), counterInit.data());

	std::vector<uint32_t> argsInit = {
			0, 1, 1, 0, // emit dispatch
			0, 1, 1, 0, // simulate dispatch
			6, 0, 0, 0, // draw: 파티클당 정점 6개 고정, 인스턴스 수는 GPU가 채움
			36, 0, 0, 0, 0 // drawIndexed: (인덱스 수, 인스턴스 수)
	};
	m_IndirectArgsBuffer.Create(L"IndirectArgsBuffer" + tag, 17, sizeof(uint32_t), argsInit.data());

	m_SortKeys.Create(L"Sort Keys" + tag, m_maxParticle, sizeof(float));
}

GP::ParticleFrameCB GP::ParticleEmitter::MakeParams(const ParticleSettings& s, float dt) const
{
	ParticleFrameCB params = {};
	Math::Vector3 pos = m_EmitterTransform.GetTranslation();

	params.emitterPosition = { pos.GetX(), pos.GetY(), pos.GetZ() };
	params.emitCount = m_CurrentSpawnCount;

	Math::Vector3 dir = Math::Matrix3(m_EmitterTransform.GetRotation()).GetY();
	params.emitterDirection = { dir.GetX(), dir.GetY(), dir.GetZ() };
	params.deltaTime = dt;
	params.totalTime = m_TotalTime;

	params.startColor = { s.startColor[0], s.startColor[1], s.startColor[2], s.startColor[3] };
	params.endColor   = { s.endColor[0],   s.endColor[1],   s.endColor[2],   s.endColor[3] };

	params.speedMin = s.speedMin;
	params.speedMax = s.speedMax;
	params.lifeTimeMin = s.lifeTimeMin;
	params.lifeTimeMax = s.lifeTimeMax;

	params.gravity = { s.gravity[0], s.gravity[1], s.gravity[2] };
	params.randomeSeed = m_FrameCount;

	params.shapeType = s.shapeType;
	params.velocityMode = s.velocityMode;
	// Shape Type 지정에 따라 다른 데이터를 GPU로 보냄
	switch ((EShapeType)s.shapeType)
	{
	case EShapeType::Box:
		params.shapeData = { s.boxExtents[0], s.boxExtents[1], s.boxExtents[2] };
		break;
	case EShapeType::Sphere:
		params.shapeData = { s.sphereRadius, s.sphereSurfaceOnly ? 1.0f : 0.0f, 0.0f };
		break;
	default:
		break;
	}
	// TODO: 모드에 따라 파라미터 다르게 줄 수 있도록
	switch ((EVelocityMode)s.velocityMode)
	{
	case EVelocityMode::Velocity:
		params.coneAngle = s.coneAngle; break;
	case EVelocityMode::VelocityFromPoint:
	case EVelocityMode::VelocityInCone:
	default:
		params.coneAngle = s.coneAngle;
		break;
	}
	params.dirSpread = s.dirSpread;
	params.posSpread = s.posSpread;

	// UI에서는 deg 값을 GPU에서는 radian으로 보도록 변환
	params.spinSpeedMin = DirectX::XMConvertToRadians(s.spinSpeedMin);
	params.spinSpeedMax = DirectX::XMConvertToRadians(s.spinSpeedMax);
	params.initAngleMin = DirectX::XMConvertToRadians(s.initAngleMin);
	params.initAngleMax = DirectX::XMConvertToRadians(s.initAngleMax);
	params.sizeMode = s.sizeMode;
	params.sizeMin = { s.sizeMin[0], s.sizeMin[1] ,s.sizeMin[2] };
	params.sizeMax = { s.sizeMax[0], s.sizeMax[1] ,s.sizeMax[2] };

	params.rotationRateMin = DirectX::XMConvertToRadians(s.rotationRateMin);
	params.rotationRateMax = DirectX::XMConvertToRadians(s.rotationRateMax);
	float3 axis = { s.rotationAxis[0], s.rotationAxis[1], s.rotationAxis[2] };
	float axisLengthSq = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
	if (axisLengthSq < 1e-6f)
		axis = { 0.0f, 0.0f, 1.0f }; // 축이 0이면 기본 축으로
	params.rotationAxis = axis;
	params.useRandomAxis = s.randomRotationAxis ? 1 : 0;
	params.useRandomInitOrientation = s.randomInitOrientation ? 1 : 0;
	params.subImagesX = s.subImagesX;
	params.subImagesY = s.subImagesY;
	params.useRandomSpawnBrightness = s.randomSpawnBrightness ? 1 : 0;
	params.useSizeOverLife = s.sizeOverLife ? 1 : 0;
	params.useColorOverLife = s.colorOverLife ? 1 : 0;
	params.useAlphaOverLife = s.alphaOverLife ? 1 : 0;

	params.keyMode = s.rendererType != (int)EParticleRenderer::Ribbon ? 0 : 1;
	params.ribbonUVMode = s.ribbonUVMode;

	params.collisionEnabled = s.collisionEnabled;
	params.restitution = s.restitution;
	params.friction = s.friction;

	params.force.flags =
		(s.forceAvoidEnabled ? FORCE_AVOID : 0) |
		(s.forceTangentEnabled ? FORCE_TANGENT : 0) |
		(s.forceCurlEnabled ? FORCE_CURL : 0) |
		(s.forceCurlEnabled && s.curlPsiBoundary ? FORCE_CURL_PSI : 0) |
		(s.forceAttractEnabled ? FORCE_ATTRACT : 0);
	params.force.avoidStrength = s.forceAvoidStrength;
	params.force.tangentStrength = s.forceTangentStrength;
	const float tangentAxisLengthSq =
		s.forceTangentAxis[0] * s.forceTangentAxis[0] +
		s.forceTangentAxis[1] * s.forceTangentAxis[1] +
		s.forceTangentAxis[2] * s.forceTangentAxis[2];
	if (tangentAxisLengthSq > 1e-8f)
	{
		const float invTangentAxisLength = 1.0f / sqrtf(tangentAxisLengthSq);
		params.force.tangentAxis = {
			s.forceTangentAxis[0] * invTangentAxisLength,
			s.forceTangentAxis[1] * invTangentAxisLength,
			s.forceTangentAxis[2] * invTangentAxisLength };
	}
	else
	{
		params.force.tangentAxis = { 0.0f, 1.0f, 0.0f };
	}
	params.force.surfaceInfluenceRadius = s.surfaceInfluenceRadius;
	params.force.curlFrequency = s.curlFrequency;
	params.force.curlTargetSpeed = s.curlTargetSpeed;
	params.force.curlResponseRate = s.curlResponseRate;
	params.force.attractStrength = s.forceAttractStrength;
	params.force.attractTargetSDF = (uint)s.forceAttractTarget;

	const bool hasMorphTarget = s.morphEnabled && m_CurrentMorphTarget != nullptr && m_CurrentMorphTarget->IsValid();

	params.morph.enabled = hasMorphTarget ? 1u : 0u;
	params.morph.targetCount = hasMorphTarget ? m_CurrentMorphTarget->GetTargetCount() : 0u;
	params.morph.strength = s.morphStrength;
	const Math::Quaternion morphRotation(
		DirectX::XMConvertToRadians(s.morphTargetRotation[0]),
		DirectX::XMConvertToRadians(s.morphTargetRotation[1]),
		DirectX::XMConvertToRadians(s.morphTargetRotation[2]));
	const Math::Vector3 morphScale(s.morphTargetScale[0], s.morphTargetScale[1], s.morphTargetScale[2]);
	const Math::Vector3 morphPosition(s.morphTargetPosition[0], s.morphTargetPosition[1], s.morphTargetPosition[2]);
	const Math::AffineTransform morphTargetToWorld(Math::Matrix3(morphRotation) * Math::Matrix3::MakeScale(morphScale), morphPosition);
	params.morph.targetToWorld = ToF4x4(Math::Matrix4(morphTargetToWorld));

	return params;
}
bool GP::ParticleEmitter::IsOpaque() const
{
	return m_Settings.rendererType == (int)EParticleRenderer::Mesh &&
		m_Settings.blendMode == (int)EBlendMode::Opaque;
}
bool GP::ParticleEmitter::NeedsSort() const
{
	// 알파(깊이 키) 또는 리본(나이 키)일 때, 토글이 켜져 있으면 정렬
	return (m_Settings.blendMode == (int)EBlendMode::Alpha ||
		m_Settings.rendererType == (int)EParticleRenderer::Ribbon) && m_Settings.sortEnabled;
}
void GP::ParticleEmitter::UpdateOrbit(float dt)
{
	if (!m_Settings.orbitEnabled)
	{
		m_EmitterTransform.SetTranslation(m_BasePosition);
		return;
	}

	m_OrbitAngle += m_Settings.orbitSpeed * dt;
	if (m_OrbitAngle > 2.0f * PI)
		m_OrbitAngle -= 2.0f * PI;

	Math::Vector3 offset(m_Settings.orbitRadius * cosf(m_OrbitAngle), 0.0f,
	                     m_Settings.orbitRadius * sinf(m_OrbitAngle));
	m_EmitterTransform.SetTranslation(m_BasePosition + offset);
}
void GP::ParticleEmitter::Update(float dt)
{
	m_FrameCount++;
	m_TotalTime += dt;
	UpdateOrbit(dt);

	if (!m_Active)
	{
		m_CurrentSpawnCount = 0;
		m_FrameParams = MakeParams(m_Settings, dt);
		return;
	}

	// 상한 정하기 (원래 max 풀 개수 기준으로 sort pass dispatch 횟수가 결정되었는데,이를 줄이기 위함)
	// (살아있는 파티클 수 예측) -> 2의 거듭제곱으로 만들어야함
	uint32_t baseTerm = (uint32_t)std::ceil(m_Settings.spawnRate * m_Settings.lifeTimeMax);
	uint32_t burstTerm = (m_Settings.burstCount > 0 && m_Settings.loopDuration > 0) ? m_Settings.burstCount * std::ceil(m_Settings.lifeTimeMax / m_Settings.loopDuration) : 0;
	uint32_t estimate = baseTerm + burstTerm;
	estimate = std::min(estimate, m_maxParticle);
	uint32_t target = 64;
	while (target < estimate) { target <<= 1; }
	// spawnRate가 급격히 감소하면 예측량이 줄지만, 실제 살아있는 파티클은 예측량보다 클 수 있음
	// 예측량까지만 정렬을 하니까 다 정렬되지 않는 문제가 있을 수 있음 -> 보수적으로 줄여야 함
	if (target >= m_SortN)
	{
		m_SortN = target;
		m_Timer = 0.0f;
	}
	else
	{
		m_Timer += dt;
		if (m_Timer > m_Settings.lifeTimeMax)
		{
			// 내부적으로 누적한 타이머가 최대 시간을 넘으면 m_SortN 줄여도 됨
			m_SortN = target;
			m_Timer = 0.0f;
		}
	}


	// 이미터 나이 증가
	m_AgeInLoop += dt;
	if (m_AgeInLoop >= m_Settings.loopDuration)
	{
		// 지속시간 초과 시
		switch ((ELoopMode)m_Settings.loopMode)
		{
		case ELoopMode::Infinite:
			m_AgeInLoop -= m_Settings.loopDuration;
			m_CanBurst = true;
			break;
		case ELoopMode::Once:
			m_Active = false; // 비활성화
			break;
		case ELoopMode::Multiple:
			m_AgeInLoop -= m_Settings.loopDuration;
			if (++m_CompletedLoops < m_Settings.loopCount)
			{
				m_CanBurst = true;
			}
			else
			{
				m_Active = false;
			}
			break;
		default:
			break;
		}
	}

	m_SpawnAccumulator += m_Settings.spawnRate * dt;
	m_CurrentSpawnCount = (uint32_t)m_SpawnAccumulator; // 내림 처리 (소수점 스폰은 할 수 없으므로)
	m_SpawnAccumulator -= m_CurrentSpawnCount; // 다음 프레임에 넘겨줄 것

	if (m_CanBurst)
	{
		m_CurrentSpawnCount += m_Settings.burstCount;
		m_CanBurst = false;
	}

	m_FrameParams = MakeParams(m_Settings, dt);
}

void GP::ParticleEmitter::ResetEmitter()
{
	m_AgeInLoop = 0.0f;
	m_Active = true;
	m_CompletedLoops = 0;
	m_CanBurst = true;
}

void GP::ParticleEmitter::BindResources(ComputeContext& cpt, const ParticleViewCB& viewParams, const ParticleCollisionCB& collisionParams,
	const D3D12_CPU_DESCRIPTOR_HANDLE* sdfSRVs, uint32_t sdfCount,
	const BVHNode* bvhNodes, uint32_t bvhNodeCount)
{
	//m_ViewParams = viewParams;

	// UAV 전환
	cpt.TransitionResource(m_Pool, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(*m_CurrentAlive, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(*m_NewAlive, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(m_DeadList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(m_IndirectArgsBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cpt.TransitionResource(m_SortKeys, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// 루트 시그 + 버퍼 바인딩
	cpt.SetRootSignature(m_Shared->computeRootSig);
	cpt.SetBufferUAV(1, m_Pool);				// u0
	cpt.SetBufferUAV(2, *m_CurrentAlive);		// u1
	cpt.SetBufferUAV(3, *m_NewAlive);			// u2
	cpt.SetBufferUAV(4, m_DeadList);			// u3
	cpt.SetBufferUAV(5, m_Counters);			// u4
	cpt.SetBufferUAV(6, m_IndirectArgsBuffer);	// u5
	cpt.SetBufferUAV(8, m_SortKeys);			// u6

	cpt.SetDynamicConstantBufferView(0, sizeof(m_FrameParams), &m_FrameParams);
	cpt.SetDynamicConstantBufferView(7, sizeof(viewParams), &viewParams); // b1
	cpt.SetDynamicConstantBufferView(9, sizeof(collisionParams), &collisionParams); // b2
	if (sdfCount > 0)
	{
		cpt.SetDynamicDescriptors(10, 0, MAX_SDF_COUNT, sdfSRVs);
	}
	StructuredBuffer& morphTargetBuffer = m_CurrentMorphTarget != nullptr ?
		m_CurrentMorphTarget->GetTargetBuffer() : m_Shared->defaultMorphTargetBuffer;
	cpt.TransitionResource(morphTargetBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.SetBufferSRV(11, morphTargetBuffer); // t64

	if (bvhNodeCount > 0)
		cpt.SetDynamicSRV(12, sizeof(BVHNode) * bvhNodeCount, bvhNodes);
}

void GP::ParticleEmitter::KickoffPass(ComputeContext& cpt)
{
	ScopedTimer _prof(L"Kickoff", cpt);
	cpt.SetPipelineState(m_Shared->kickoffPSO);
	cpt.Dispatch(1, 1, 1);
}

void GP::ParticleEmitter::EmitPass(ComputeContext& cpt)
{
	ScopedTimer _prof(L"Emit", cpt);
	// 대기: 킥오프의 카운터 쓰기(COUNTER_REAL) 완료
	cpt.InsertUAVBarrier(m_Counters);
	// 대기 겸 용도 변경: 지시서를 커맨드 프로세서가 읽을 수 있게
	cpt.TransitionResource(m_IndirectArgsBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	cpt.SetPipelineState(m_Shared->emitPSO);
	cpt.DispatchIndirect(m_IndirectArgsBuffer, ARGS_EMIT_DISPATCH_X);
}

void GP::ParticleEmitter::SimulatePass(ComputeContext& cpt)
{
	// 대기: emit의 UAV 쓰기
	cpt.InsertUAVBarrier(*m_CurrentAlive);
	cpt.InsertUAVBarrier(m_Pool);
	cpt.InsertUAVBarrier(m_DeadList);
	cpt.InsertUAVBarrier(m_Counters);

	// 알파이고 소트 켜져있을 때만 
	if (NeedsSort())
	{
		// SortKeys 버퍼 양수 최대로 밀기
		ScopedTimer _profClear(L"Sort Key Clear", cpt);
		cpt.FillBuffer(m_SortKeys, 0, 1e30f, m_SortN * sizeof(float));
		cpt.TransitionResource(m_SortKeys, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}

	ScopedTimer _prof(L"Simulate", cpt);
	cpt.SetPipelineState(m_Shared->simulatePSO);
	cpt.DispatchIndirect(m_IndirectArgsBuffer, ARGS_SIMULATE_DISPATCH_X);
}

void GP::ParticleEmitter::SortPass(ComputeContext& cpt)
{
	if (!NeedsSort())
		return;
	m_Shared->sorter.Sort(cpt, *m_NewAlive, m_SortKeys, m_SortN);
}

void GP::ParticleEmitter::UpdateDrawArgs(ComputeContext& cpt)
{
	ScopedTimer _prof(L"Copy Args", cpt);
	cpt.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_COPY_SOURCE);
	// simulationCS가 확정한 생존자 수 복사
	// ARGS_DRAW_INSTANCE_COUNT 구간 (for Sprite)
	cpt.CopyBufferRegion(m_IndirectArgsBuffer, ARGS_DRAW_INSTANCE_COUNT, m_Counters, COUNTER_AFTER_SIMULATE, sizeof(uint32_t));
	// ARGS_DRAW_INDEXED_INSTANCE_COUNT 구간 (for Mesh)
	cpt.CopyBufferRegion(m_IndirectArgsBuffer, ARGS_DRAW_INDEXED_INSTANCE_COUNT, m_Counters, COUNTER_AFTER_SIMULATE, sizeof(uint32_t));
	// 인덱스 카운트 - 메시 지정 X -> 큐브 인덱스 카운트로 설정
	const uint32_t indexCount = m_ParticleMesh != nullptr ?
		m_ParticleMesh->GetIndexCount() : (uint32_t)_countof(kCubeIndices);
	cpt.FillBuffer(m_IndirectArgsBuffer, ARGS_DRAW_INDEXED_INDEX_COUNT, indexCount, sizeof(uint32_t));
	cpt.TransitionResource(m_IndirectArgsBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	// Draw가 읽을 리소스 SRV 전환
	cpt.TransitionResource(m_Pool, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(*m_NewAlive, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void GP::ParticleEmitter::Draw(GraphicsContext& gfx, bool halfResolution)
{
	ScopedTimer _prof(L"Particle Draw", gfx);
	ParticleDrawCB drawCB = {};
	drawCB.blendMode = m_Settings.blendMode;
	drawCB.alignmentMode = m_Settings.alignmentMode;

	gfx.SetDynamicConstantBufferView(1, sizeof(m_FrameParams), &m_FrameParams); // b1
	gfx.SetDynamicConstantBufferView(2, sizeof(drawCB), &drawCB);               // b2
	gfx.SetBufferSRV(3, m_Pool);												// t0
	gfx.SetBufferSRV(4, *m_NewAlive);											// t1
	const Texture* texture = m_Shared->textureLibrary->Find(m_Settings.texturePath);
	if (texture == nullptr)
		texture = &m_Shared->textureLibrary->GetFallback();
	gfx.SetDynamicDescriptor(5, 0, texture->GetSRV()); // t2
	gfx.SetBufferSRV(6, m_Counters);											// t3
	
	gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// PSO 선택(렌더러 타입, 해상도 따라)
	const int res = halfResolution ? 1 : 0;
	gfx.SetPipelineState(IsOpaque() ? m_Shared->meshOpaquePSO[res]
		: m_Shared->drawPSO[m_Settings.rendererType][res]);

	// 메시 드로우콜
	if (m_Settings.rendererType == (int)EParticleRenderer::Mesh)
	{
		if (m_ParticleMesh != nullptr)
		{
			gfx.SetVertexBuffer(0, m_ParticleMesh->GetVertexBuffer().VertexBufferView());
			gfx.SetIndexBuffer(m_ParticleMesh->GetIndexBuffer().IndexBufferView());
		}
		else
		{
			gfx.SetVertexBuffer(0, m_Shared->meshVertexBuffer.VertexBufferView());
			gfx.SetIndexBuffer(m_Shared->meshIndexBuffer.IndexBufferView());
		}
		gfx.ExecuteIndirect(Graphics::DrawIndexedIndirectCommandSignature, m_IndirectArgsBuffer, ARGS_DRAW_INDEXED_INDEX_COUNT);
	}
	else // 스프라이트, 리본 드로우콜
	{
		gfx.DrawIndirect(m_IndirectArgsBuffer, ARGS_DRAW_VERTEX_COUNT_PER_INSTANCE);
	}
}

void GP::ParticleEmitter::EndFrame()
{
	std::swap(m_CurrentAlive, m_NewAlive);
}

void GP::ParticleEmitter::TickSequence(float dt)
{
	EmitterSequence& sequence = m_Sequence;
	// Stage 없거나 실행 중 아니면 Tick X
	if (sequence.stages.empty() || !sequence.playing)
		return;
	sequence.stageTimer += dt; // 현재 스테이지 시간 누적
	// 지정 시간 지나면
	while (sequence.playing &&
		sequence.stages[sequence.currentStage].duration > 0.0f &&
		sequence.stageTimer >= sequence.stages[sequence.currentStage].duration)
	{
		const bool isLast = sequence.currentStage + 1 >= sequence.stages.size();
		if (isLast && !sequence.loop)
		{
			// 스테이지 없고 루프 없으면 중지
			// 여기서 ApplyStage를 부르면 ResetEmitter가 끝난 이미터를 되살리고 버스트가 한 번 더 나감
			sequence.stageTimer = sequence.stages[sequence.currentStage].duration;
			sequence.playing = false;
			break;
		}

		sequence.stageTimer -= sequence.stages[sequence.currentStage].duration;
		// 루프면 처음으로, 아니면 다음 스테이지로
		sequence.currentStage = isLast ? 0 : sequence.currentStage + 1;
		ApplyStage();
	}
}
void GP::ParticleEmitter::ApplyStage()
{
	if (m_Sequence.stages.empty())
		return;
	const SequenceStage& stage = m_Sequence.stages[m_Sequence.currentStage];
	m_Settings = stage.settings;
	SetMorphTarget(stage.morphTarget);
	ResetEmitter();
}
