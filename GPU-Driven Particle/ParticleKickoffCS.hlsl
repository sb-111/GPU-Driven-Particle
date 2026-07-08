#include "ParticleShared.h"
cbuffer ParticleCB : register(b0)
{
	ParticleFrameCB params;
}
RWByteAddressBuffer Counters : register(u4);
RWByteAddressBuffer ArgsBuffer : register(u5);
[numthreads(1, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
	// simulation 종료 후 AliveList2에 대한 실제 카운트를 Alive Counter에 줘야 함

	// 1. Alive Counter를 실제 Simulation 끝난 후 카운터 값으로 덮어쓰기
	Counters.Store(COUNTER_ALIVE, Counters.Load(COUNTER_AFTER_SIMULATE));
	// 2. Simulation 끝난 후 카운터를 0으로 초기화
	Counters.Store(COUNTER_AFTER_SIMULATE, 0);

	// 실제 방출량 확정 (데드 카운트랑 요청한 emit 중 더 작은 것만 방출 가능)
	uint requested = params.emitCount; // cpu로부터 요청받은 값
	uint deadCount = Counters.Load(COUNTER_DEAD); // 죽은 개수 (이거 이상으로 방출 불가)
	uint realEmitCount = min(requested, deadCount);

	// 이거를 Counters의 진짜 방출량에 기록한다.
	Counters.Store(COUNTER_REAL, realEmitCount);

	// 작업 지시서 작성
	// Emit 단계의 스레드 그룹 개수
	// Simulate 단계의 스레드 그룹 개수
	ArgsBuffer.Store(ARGS_EMIT_DISPATCH_X, (realEmitCount + 63) / 64); // 실제 emitCount 기반 그룹수 정하기
	uint aliveCount = Counters.Load(COUNTER_ALIVE);
	uint totalCount = aliveCount + realEmitCount;
	ArgsBuffer.Store(ARGS_SIMULATE_DISPATCH_X, (totalCount + 63) / 64); // 현재 존재하는 파티클 수 + 이번 프레임에 방출된 파티클 수 기반 그룹 수
}
