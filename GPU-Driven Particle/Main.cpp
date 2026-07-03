#include "GameCore.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"

using namespace GameCore;
using namespace Graphics;

class ParticleApp : public IGameApp
{
public:
    void Startup(void) override {}
    void Cleanup(void) override {}
    void Update(float deltaT) override { (void)deltaT; }
    void RenderScene(void) override
    {
        GraphicsContext& gfx = GraphicsContext::Begin(L"Clear");
        gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        float clearColor[4] = { 0.2f, 0.4f, 0.8f, 1.0f };
        gfx.ClearColor(g_SceneColorBuffer, clearColor);
        gfx.Finish();
    }
};

CREATE_APPLICATION(ParticleApp)