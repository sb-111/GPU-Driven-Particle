#ifndef SCENE_LIGHTING_HLSLI
#define SCENE_LIGHTING_HLSLI

static const float3 kSunDir = normalize(float3(0.4f, 0.8f, 0.45f));
static const float kAmbient = 0.04f;

float LambertTerm(float3 worldNormal)
{
	return kAmbient + (1.0f - kAmbient) * saturate(dot(normalize(worldNormal), kSunDir));
}

#endif
