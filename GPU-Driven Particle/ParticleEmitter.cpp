#include "ParticleEmitter.h"
#include "CommandContext.h"
#include "MathConvert.h"
#include "GraphicsCommon.h"
void GP::ParticleEmitter::Init(uint32_t maxParticles, ParticleSharedResources* shared, uint32_t index)
{
	m_maxParticle = maxParticles;
	m_Shared = shared;

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

	return params;
}
void GP::ParticleEmitter::Update(float dt)
{
	m_FrameCount++;

	if (!m_Active)
	{
		m_CurrentSpawnCount = 0;
		m_FrameParams = MakeParams(m_Settings, dt);
		return;
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

void GP::ParticleEmitter::BindResources(ComputeContext& cpt, const ParticleViewCB& viewParams)
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
}

void GP::ParticleEmitter::KickoffPass(ComputeContext& cpt)
{
	cpt.SetPipelineState(m_Shared->kickoffPSO);
	cpt.Dispatch(1, 1, 1);
}

void GP::ParticleEmitter::EmitPass(ComputeContext& cpt)
{
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

	// SortKeys 버퍼 양수 최대로 밀기
	cpt.FillBuffer(m_SortKeys, 0, 1e30f, m_maxParticle * sizeof(float));
	cpt.TransitionResource(m_SortKeys, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	cpt.SetPipelineState(m_Shared->simulatePSO);
	cpt.DispatchIndirect(m_IndirectArgsBuffer, ARGS_SIMULATE_DISPATCH_X);
}

void GP::ParticleEmitter::SortPass(ComputeContext& cpt)
{
	if (m_Settings.blendMode != (int)EBlendMode::Alpha || !m_Settings.sortEnabled)
		return;
	m_Shared->sorter.Sort(cpt, *m_NewAlive, m_SortKeys, m_maxParticle);
}

void GP::ParticleEmitter::UpdateDrawArgs(ComputeContext& cpt)
{
	cpt.TransitionResource(m_Counters, D3D12_RESOURCE_STATE_COPY_SOURCE);
	// simulationCS가 확정한 생존자 수 복사
	// ARGS_DRAW_INSTANCE_COUNT 구간 (for Sprite)
	cpt.CopyBufferRegion(m_IndirectArgsBuffer, ARGS_DRAW_INSTANCE_COUNT, m_Counters, COUNTER_AFTER_SIMULATE, sizeof(uint32_t));
	// ARGS_DRAW_INDEXED_INSTANCE_COUNT 구간 (for Mesh)
	cpt.CopyBufferRegion(m_IndirectArgsBuffer, ARGS_DRAW_INDEXED_INSTANCE_COUNT, m_Counters, COUNTER_AFTER_SIMULATE, sizeof(uint32_t));
	cpt.TransitionResource(m_IndirectArgsBuffer, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

	// Draw가 읽을 리소스 SRV 전환
	cpt.TransitionResource(m_Pool, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	cpt.TransitionResource(*m_NewAlive, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

void GP::ParticleEmitter::Draw(GraphicsContext& gfx)
{
	ParticleDrawCB drawCB = {};
	drawCB.blendMode = m_Settings.blendMode;
	drawCB.alignmentMode = m_Settings.alignmentMode;

	gfx.SetDynamicConstantBufferView(1, sizeof(m_FrameParams), &m_FrameParams); // b1
	gfx.SetDynamicConstantBufferView(2, sizeof(drawCB), &drawCB);               // b2
	gfx.SetBufferSRV(3, m_Pool);												// t0
	gfx.SetBufferSRV(4, *m_NewAlive);											// t1
	gfx.SetDynamicDescriptor(5, 0, m_Shared->spriteTextures[m_Settings.textureIndex].GetSRV()); // t2
	gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	if (m_Settings.rendererType == (int)EParticleRenderer::Sprite)
	{
		// 블렌드 모드에 따른 다른 PSO 설정
		gfx.SetPipelineState(m_Settings.blendMode == (int)EBlendMode::Additive ? m_Shared->drawAdditivePSO : m_Shared->drawAlphaPSO);
		gfx.DrawIndirect(m_IndirectArgsBuffer, ARGS_DRAW_VERTEX_COUNT_PER_INSTANCE);
	}
	else if (m_Settings.rendererType == (int)EParticleRenderer::Mesh)
	{
		gfx.SetPipelineState(m_Settings.blendMode == (int)EBlendMode::Additive ? m_Shared->meshAdditivePSO : m_Shared->meshAlphaPSO);
		gfx.SetVertexBuffer(0, m_Shared->meshVertexBuffer.VertexBufferView());
		gfx.SetIndexBuffer(m_Shared->meshIndexBuffer.IndexBufferView());
		gfx.ExecuteIndirect(Graphics::DrawIndexedIndirectCommandSignature, m_IndirectArgsBuffer, ARGS_DRAW_INDEXED_INDEX_COUNT);
	}
}

void GP::ParticleEmitter::EndFrame()
{
	std::swap(m_CurrentAlive, m_NewAlive);
}
