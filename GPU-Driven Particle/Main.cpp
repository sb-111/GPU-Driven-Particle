#include "GameCore.h"
#include "GraphicsCore.h"
#include "BufferManager.h"
#include "CommandContext.h"
#include "RenderTypes.h"
#include "RootSignature.h"
#include "PipelineState.h"

#include "CompiledShaders/TriangleVS.h"
#include "CompiledShaders/TrianglePS.h"

using namespace GameCore;
using namespace Graphics;
using namespace GP;

class ParticleApp : public IGameApp
{
public:
    void Startup(void) override 
    {
        // 정점 3개 -> VS -> RS -> PS -> RTV
        Vertex verts[3] =
        {
            {{  0.0f,  0.5f, 0.0f }, { 1,0,0,1 }},   
            {{ -0.5f, -0.5f, 0.0f }, { 0,0,1,1 }},   
            {{  0.5f, -0.5f, 0.0f }, { 0,1,0,1 }},   
        };

        m_VertexBuffer.Create(L"Triangle VB", 3, sizeof(Vertex), verts);

        m_RootSig.Reset(0, 0);
        m_RootSig.Finalize(L"triangle", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        m_PSO.SetRootSignature(m_RootSig);
        D3D12_INPUT_ELEMENT_DESC inputLayout[] =
        {
            {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };
        m_PSO.SetInputLayout(_countof(inputLayout), inputLayout);
        m_PSO.SetVertexShader(g_pTriangleVS, sizeof(g_pTriangleVS));
        m_PSO.SetPixelShader(g_pTrianglePS, sizeof(g_pTrianglePS));
        m_PSO.SetRasterizerState(RasterizerDefault);
        m_PSO.SetBlendState(BlendDisable);
        m_PSO.SetDepthStencilState(DepthStateDisabled);
        m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
        m_PSO.SetRenderTargetFormat(g_SceneColorBuffer.GetFormat(), DXGI_FORMAT_UNKNOWN);
        m_PSO.Finalize();


    }
    void Cleanup(void) override {}
    void Update(float deltaT) override { (void)deltaT; }

    // official rendering pass
    void RenderScene(void) override
    {
        // Command List 래퍼
        GraphicsContext& gfx = GraphicsContext::Begin(L"Clear");
        gfx.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);

        // 배경 초기화
        float clearColor[4] = { 0.2f, 0.4f, 0.8f, 1.0f };
        gfx.ClearColor(g_SceneColorBuffer, clearColor);

        gfx.SetRenderTarget(g_SceneColorBuffer.GetRTV());
        gfx.SetViewportAndScissor(0, 0, g_SceneColorBuffer.GetWidth(), g_SceneColorBuffer.GetHeight());
        gfx.SetRootSignature(m_RootSig);
        gfx.SetPipelineState(m_PSO);
        // 삼각형 그리기
        // 버퍼는 그냥 바이트니까 View에 해석을 담아야 함.
        // 0번 슬롯에 바인딩
        gfx.SetVertexBuffer(0, m_VertexBuffer.VertexBufferView());
        // Primitive 설정
        gfx.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfx.Draw(3);

        gfx.Finish();
    }
private:
    ByteAddressBuffer m_VertexBuffer;
    RootSignature m_RootSig;
    GraphicsPSO m_PSO;
};

CREATE_APPLICATION(ParticleApp)