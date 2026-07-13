#include "ParticleShared.h"

static const float2 QuadVert[6] =
{
	float2(-1, -1), float2(1, 1), float2(-1, 1), // 좌하→우상→좌상 (CCW)
    float2(-1, -1), float2(1, -1), float2(1, 1) // 좌하→우하→우상 (CCW)
};

cbuffer VSConstants : register(b0)
{
	ParticleDrawCB drawParams;
}
cbuffer ParticleParams : register(b1)
{
	ParticleFrameCB frameParams;
}
StructuredBuffer<Particle> g_ParticleBuffer : register(t0);
ByteAddressBuffer NewAliveList : register(t1);

struct VSOutput
{
	float4 pos : SV_Position;
	float4 color : COLOR;
	float2 uv : TEXCOORD0;
};
// 입력: 정점 id
VSOutput main(uint vid : SV_VertexID, uint iid: SV_InstanceID)
{
	// Alive 접근 : Instance id를 통해 접근
	uint index = NewAliveList.Load(iid * 4);
	Particle p = g_ParticleBuffer[index];

	float normalizedAge = 1.0f - (p.lifeTime / p.initialLife); // 0 -> 1 (progress)
	float brightness = 1.0f - normalizedAge;				   // 시간 흐르면 밝기 감소 (fade)
	float sizeScale = 1.0f - (normalizedAge * normalizedAge);  // 시간 흐르면 사이즈 감소 (fade)

	float2 scaledSize = p.size.xy * sizeScale;

	float age = p.initialLife - p.lifeTime; // 증가하는 값
	float angleZ = p.angle.z + p.spinSpeed * age;
	float s = sin(angleZ);
	float c = cos(angleZ);

	// Scale
	float3x3 scaleMat = float3x3(
		scaledSize.x, 0, 0,
		0, scaledSize.y, 0,
		0, 0, 1
	);
	// Rotation(Z축 회전 = roll),
	float3x3 rotationMatZ = float3x3(
	    c, -s, 0,
		s, c, 0,
		0, 0, 1
	);
	// Rotaion (로컬 기저 -> 카메라 기저)
	float3 camBack = cross(drawParams.camRight, drawParams.camUp);
	float3x3 screenAlignedRotationMat = float3x3(
			drawParams.camRight.x, drawParams.camUp.x, camBack.x,
			drawParams.camRight.y, drawParams.camUp.y, camBack.y,
			drawParams.camRight.z, drawParams.camUp.z, camBack.z
	);
	
	float2 corner = QuadVert[vid]; // Instance 내 정점
	float3 localPos = float3(corner, 0);
	// S->R(로컬에서 z축 회전)->R(월드 기저로 옮김)
	float3x3 finalMat = mul(screenAlignedRotationMat, mul(rotationMatZ, scaleMat));
	
	float3 worldPos = p.position + mul(finalMat, localPos);

	
	VSOutput output;
	output.pos = mul(drawParams.viewProj, float4(worldPos, 1.0f));
	output.color.rgb = lerp(p.color.rgb, frameParams.endColor.rgb, normalizedAge);
	output.color.rgb *= brightness;
	output.color.a = p.color.a;
	output.uv = (corner * 0.5f + 0.5f);
	output.uv.y = 1 - output.uv.y;

	
	return output;
}
