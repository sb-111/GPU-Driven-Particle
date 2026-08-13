#include "ParticleShared.h"
#include "ParticleRandom.hlsli"
#include "SDFCollision.hlsli"
// ψ 채널 오프셋
static const float3 kPsiOffset1 = float3(31.4f, 47.7f, 12.3f); // ψy 용
static const float3 kPsiOffset2 = float3(-23.1f, 9.2f, 78.8f); // ψz 용
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
	const float3 X = float3(e, 0, 0), Y = float3(0, e, 0), Z = float3(0, 0, e);

    // ψx = Noise(s), ψy = Noise(s + kPsiOffset1), ψz = Noise(s + kPsiOffset2)
	// 여섯개 편미분
	float dPsiZ_dy = Noise(s + Y + kPsiOffset2) - Noise(s - Y + kPsiOffset2);
	float dPsiZ_dx = Noise(s + X + kPsiOffset2) - Noise(s - X + kPsiOffset2);
	float dPsiY_dz = Noise(s + Z + kPsiOffset1) - Noise(s - Z + kPsiOffset1);
	float dPsiY_dx = Noise(s + X + kPsiOffset1) - Noise(s - X + kPsiOffset1);
	float dPsiX_dy = Noise(s + Y) - Noise(s - Y);
	float dPsiX_dz = Noise(s + Z) - Noise(s - Z);

	// ∇ × ψ
	return float3(dPsiZ_dy - dPsiY_dz,
                  dPsiX_dz - dPsiZ_dx,
                  dPsiY_dx - dPsiX_dy) / (2.0f * e);
}

// 경계를 포텐셜 단계에서 처리
float3 PsiConstrained(float3 noisePos, float3 worldPos, float influenceRadius)
{
	float3 psi = float3(Noise(noisePos),
	                    Noise(noisePos + kPsiOffset1),
	                    Noise(noisePos + kPsiOffset2));

	SDFQueryResult q = QuerySceneSDF(worldPos); // 지점마다 씬 질의
	if (q.colliderType != COLLIDER_TYPE_NONE && q.distance < influenceRadius)
	{
		float3 n = QueryColliderNormal(q.colliderType, q.colliderIndex, worldPos); // 지점마다 법선
		float alpha = saturate(q.distance / influenceRadius);
		psi = lerp(n * dot(n, psi), psi, alpha); // 표면: 법선 성분만 유지
	}
	return psi;
}
float3 CurlNoisePsiBoundary(float3 noisePos, float3 worldPos, float curlFrequency, float influenceRadius)
{
	const float e = 0.25f;
	const float3 noiseOffsetX = float3(e, 0, 0), noiseOffsetY = float3(0, e, 0), noiseOffsetZ = float3(0, 0, e);
	// 노이즈 공간 오프셋 ±e를 월드 좌표로 환산
	const float eWorld = e / curlFrequency;
	const float3 worldOffsetX = float3(eWorld, 0, 0), worldOffsetY = float3(0, eWorld, 0), worldOffsetZ = float3(0, 0, eWorld);

	float3 psiXPlus = PsiConstrained(noisePos + noiseOffsetX, worldPos + worldOffsetX, influenceRadius);
	float3 psiXMinus = PsiConstrained(noisePos - noiseOffsetX, worldPos - worldOffsetX, influenceRadius);
	float3 psiYPlus = PsiConstrained(noisePos + noiseOffsetY, worldPos + worldOffsetY, influenceRadius);
	float3 psiYMinus = PsiConstrained(noisePos - noiseOffsetY, worldPos - worldOffsetY, influenceRadius);
	float3 psiZPlus = PsiConstrained(noisePos + noiseOffsetZ, worldPos + worldOffsetZ, influenceRadius);
	float3 psiZMinus = PsiConstrained(noisePos - noiseOffsetZ, worldPos - worldOffsetZ, influenceRadius);

	// v = ∇×ψ
	return float3((psiYPlus.z - psiYMinus.z) - (psiZPlus.y - psiZMinus.y),
	              (psiZPlus.x - psiZMinus.x) - (psiXPlus.z - psiXMinus.z),
	              (psiXPlus.y - psiXMinus.y) - (psiYPlus.x - psiYMinus.x)) / (2.0f * e);
}
