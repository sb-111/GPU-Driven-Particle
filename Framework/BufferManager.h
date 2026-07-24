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
// NOTE: Trimmed for the GPU-Driven Particle framework. Only the scene render target,
// a depth buffer, and the UI overlay are kept; all post-process / scaling targets removed.
//

#pragma once

#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "GpuBuffer.h"
#include "GraphicsCore.h"

namespace Graphics
{
    extern DepthBuffer g_SceneDepthBuffer;  // D32_FLOAT
    extern ColorBuffer g_SceneColorBuffer;  // R11G11B10_FLOAT
    extern ColorBuffer g_OverlayBuffer;     // R8G8B8A8_UNORM (UI overlay)

	extern ColorBuffer g_SceneColorHalfBuffer;
	extern DepthBuffer g_SceneDepthHalfBuffer;

    void InitializeRenderingBuffers(uint32_t NativeWidth, uint32_t NativeHeight );
    void ResizeDisplayDependentBuffers(uint32_t NativeWidth, uint32_t NativeHeight);
    void DestroyRenderingBuffers();

} // namespace Graphics
