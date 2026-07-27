Texture2D<float> g_SceneDepth : register(t2);
SamplerState g_LinearClamp : register(s0);
struct PSInput
{
	float4 pos : SV_Position;
	float2 uv : TEXCOORD0;
};
float main(PSInput input) : SV_Depth
{
	// input.pos.xy는 현재 RT 기준 pixel 중심 좌표
	int2 base = int2(input.pos.xy) * 2; // 씬뎁스 크기 기준 좌표 변환
	int3 a = int3(base,0); // 좌상단
	int3 b = int3(base.x + 1, base.y, 0);
	int3 c = int3(base.x, base.y + 1,0);
	int3 d = int3(base.x + 1, base.y + 1,0);

	// Texel 읽기 (uv가 아닌 텍셀의 정수 좌표로 읽음)
	float ta = g_SceneDepth.Load(a);
	float tb = g_SceneDepth.Load(b);
	float tc = g_SceneDepth.Load(c);
	float td = g_SceneDepth.Load(d);

	// 씬뎁스의 2x2 깊이 중에 제일 작은 깊이(제일 멀리있는 것) 
	float finalDepth = min(min(min(ta, tb), tc), td);

	return finalDepth;
}
