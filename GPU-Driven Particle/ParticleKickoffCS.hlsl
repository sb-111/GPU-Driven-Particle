#include "ParticleShared.h"
RWByteAddressBuffer Counters : register(u4);

[numthreads(1, 1, 1)]
void main( uint3 id : SV_DispatchThreadID )
{
	// simulation 종료 후 AliveList2에 대한 실제 카운트를 Alive Counter에 줘야 함

	// 1. Alive Counter를 실제 Simulation 끝난 후 카운터 값으로 덮어쓰기
	Counters.Store(COUNTER_ALIVE, Counters.Load(COUNTER_AFTER_SIMULATE));
	// 2. Simulation 끝난 후 카운터를 0으로 초기화
	Counters.Store(COUNTER_AFTER_SIMULATE, 0);

}
