#pragma once
#include "RootSignature.h"
#include "PipelineState.h"
#include "Texture.h"
class GraphicsContext;
namespace GP
{
	class Camera;

	class SkyboxRenderer
	{
	public:
		void Init(const char* ddsPath);
		void Render(GraphicsContext& gfx, const Camera& camera);
	private:
		RootSignature m_RootSig;
		GraphicsPSO m_PSO;
		Texture m_CubeMap;
	};
}


