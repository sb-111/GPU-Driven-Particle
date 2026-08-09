#include "ParticleShared.h"
#include "ParticleRandom.hlsli"
// float3 -> scalar
float Noise(float3 s)
{
	int3 i = floor(s); // 현재 위치가 속한 정수 격자 cell의 최소 꼭짓점
	float3 f = s - i;
	float3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0); // quintic 페이드

    // 꼭짓점 8개 Hash를 u로 삼선형 보간
	return lerp(lerp(lerp(HashCell(i + int3(0, 0, 0)), HashCell(i + int3(1, 0, 0)), u.x),
                     lerp(HashCell(i + int3(0, 1, 0)), HashCell(i + int3(1, 1, 0)), u.x), u.y),
                lerp(lerp(HashCell(i + int3(0, 0, 1)), HashCell(i + int3(1, 0, 1)), u.x),
                     lerp(HashCell(i + int3(0, 1, 1)), HashCell(i + int3(1, 1, 1)), u.x), u.y), u.z);
}
// 벡터 포텐셜 ψ를 만들고, ∇ × ψ(curl)를 구함
float3 CurlNoise(float3 s)
{
	const float e = 0.25f;
	const float3 O1 = float3(31.4f, 47.7f, 12.3f); // ψy 용
	const float3 O2 = float3(-23.1f, 9.2f, 78.8f); // ψz 용
	const float3 X = float3(e, 0, 0), Y = float3(0, e, 0), Z = float3(0, 0, e);

    // ψx = Noise(s), ψy = Noise(s + O1), ψz = Noise(s + O2)
	// 여섯개 편미분
	float dPsiZ_dy = Noise(s + Y + O2) - Noise(s - Y + O2);
	float dPsiZ_dx = Noise(s + X + O2) - Noise(s - X + O2);
	float dPsiY_dz = Noise(s + Z + O1) - Noise(s - Z + O1);
	float dPsiY_dx = Noise(s + X + O1) - Noise(s - X + O1);
	float dPsiX_dy = Noise(s + Y) - Noise(s - Y);
	float dPsiX_dz = Noise(s + Z) - Noise(s - Z);

	// ∇ × ψ
	return float3(dPsiZ_dy - dPsiY_dz,
                  dPsiX_dz - dPsiZ_dx,
                  dPsiY_dx - dPsiX_dy) / (2.0f * e);
}
