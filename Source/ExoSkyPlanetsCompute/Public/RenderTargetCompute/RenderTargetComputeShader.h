/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"
#include "RenderGraphDefinitions.h" 
#include "SceneInterface.h"
#include "ExoSkyPlanetsCompute.h"

struct EXOSKYPLANETSCOMPUTE_API FRenderTargetComputeShaderDispatchParams
{
    int X;
    int Y;
    int Z;

    UTextureRenderTarget2D* RenderTarget2D;
    FRenderTarget* RenderTarget;

    FMaterialRenderProxy* MaterialRenderProxy;
    FSceneInterface* Scene;

    FRenderTargetComputeShaderDispatchParams(int x, int y, int z)
        : X(x)
        , Y(y)
        , Z(z)
    {    
    }
};

DECLARE_CYCLE_STAT(TEXT("Generator Compute"), STAT_RenderTargetComputeShader_Execute, STATGROUP_ExoskyPlanetsCompute);
class EXOSKYPLANETSCOMPUTE_API FRenderTargetComputeShaderInterface {
public:
    static void DispatchRenderThread(
        FRHICommandListImmediate& RHICmdList,
        FRenderTargetComputeShaderDispatchParams Params
    );

    static void DispatchGameThread(FRenderTargetComputeShaderDispatchParams Params)
    {
        ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
        [Params](FRHICommandListImmediate& RHICmdList)
        {
            FRenderTargetComputeShaderDispatchParams ParamsCopy = Params;
            ParamsCopy.RenderTarget = Params.RenderTarget2D->GetRenderTargetResource();
            DispatchRenderThread(RHICmdList, ParamsCopy);
        }
        );
    }

    static void Dispatch(FRenderTargetComputeShaderDispatchParams Params)
    {
        if(IsInRenderingThread())
        {
            DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params);
        }
        else
        {
            DispatchGameThread(Params);
        }
    }
};