//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//
// NOTE: Trimmed for the GPU-Driven Particle framework. See BufferManager.h.
//

#include "pch.h"
#include "BufferManager.h"
#include "Display.h"

namespace Graphics
{
    DepthBuffer g_SceneDepthBuffer;
    ColorBuffer g_SceneColorBuffer;
    ColorBuffer g_OverlayBuffer;

	ColorBuffer g_SceneColorHalfBuffer;
	DepthBuffer g_SceneDepthHalfBuffer;

    DXGI_FORMAT DefaultHdrColorFormat = DXGI_FORMAT_R11G11B10_FLOAT;
}

#define DSV_FORMAT DXGI_FORMAT_D32_FLOAT

void Graphics::InitializeRenderingBuffers( uint32_t bufferWidth, uint32_t bufferHeight )
{
    g_SceneColorBuffer.Create( L"Main Color Buffer", bufferWidth, bufferHeight, 1, DefaultHdrColorFormat );
    g_SceneDepthBuffer.Create( L"Scene Depth Buffer", bufferWidth, bufferHeight, DSV_FORMAT );
    g_OverlayBuffer.Create( L"UI Overlay", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM );

	g_SceneColorHalfBuffer.Create(L"Particle Half Color Buffer", bufferWidth / 2, bufferHeight / 2, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	g_SceneDepthHalfBuffer.Create(L"Particle Half Depth Buffer", bufferWidth / 2, bufferHeight / 2, 1, DSV_FORMAT);
}

void Graphics::ResizeDisplayDependentBuffers(uint32_t NativeWidth, uint32_t NativeHeight)
{
    (NativeWidth); (NativeHeight);
    g_OverlayBuffer.Create( L"UI Overlay", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R8G8B8A8_UNORM );
}

void Graphics::DestroyRenderingBuffers()
{
    g_SceneDepthBuffer.Destroy();
    g_SceneColorBuffer.Destroy();
    g_OverlayBuffer.Destroy();

	g_SceneColorHalfBuffer.Destroy();
	g_SceneDepthHalfBuffer.Destroy();
}
