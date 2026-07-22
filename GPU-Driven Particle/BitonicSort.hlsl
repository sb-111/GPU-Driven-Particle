RWByteAddressBuffer AliveList : register(u0);       // 정렬 대상: 파티클 풀 인덱스
RWStructuredBuffer<float> SortKeys : register(u1);  // 정렬 기준: 슬롯별 깊이 키 or 파티클 나이

cbuffer SortParams : register(b0)
{
	uint k; // STAGE: 정렬 블록 크기 (방향이 k칸마다 교대)
	uint j; // PASS: 비교 거리 (짝 = j칸 떨어진 슬롯)
}

[numthreads(64, 1, 1)]
void main( uint3 tid : SV_DispatchThreadID )
{
	uint self = tid.x;
	// 이번 pass에서 비교해야 할 짝 슬롯
	uint partner = self ^ j;

	// 쌍마다 일꾼은 앞쪽 슬롯 하나 = 어떤 슬롯도 두 스레드가 동시에 안 만짐
	if (partner < self)
		return;

	// 읽기: 지역 변수 선언 (전부 레지스터)
	float selfKey = SortKeys[self];
	float partnerKey = SortKeys[partner];
	uint selfPoolIndex = AliveList.Load(self * 4);
	uint partnerPoolIndex = AliveList.Load(partner * 4);

	// 판정: 오름차순이고, 앞이 더 큰 경우 or 내림차순이고, 앞이 더 작은 경우
	bool isAscending = (self & k) == 0;			// 오름차순인가
	bool isSelfBigger = selfKey > partnerKey;	// 앞이 더 큰가

	if (isAscending == isSelfBigger)
	{
		// 쓰기: Key와 풀 인덱스 두개 같이 스왑
		SortKeys[self] = partnerKey;
		SortKeys[partner] = selfKey;
		AliveList.Store(self * 4, partnerPoolIndex);
		AliveList.Store(partner * 4, selfPoolIndex);
	}
}
