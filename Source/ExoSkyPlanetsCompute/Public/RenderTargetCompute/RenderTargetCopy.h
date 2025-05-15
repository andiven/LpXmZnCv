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

struct EXOSKYPLANETSCOMPUTE_API FRenderTargetCopyDispatchParams
{
    UTextureRenderTarget2D* SourceTexture;
    UTextureRenderTarget2D* DestTexture;
    FRenderTarget* SourceTarget;
    FRenderTarget* DestTarget;

    FSceneInterface* Scene;
};

class EXOSKYPLANETSCOMPUTE_API FRenderTargetCopyInterface {
public:
    static void DispatchRenderThread(
        FRHICommandListImmediate& RHICmdList,
        FRenderTargetCopyDispatchParams Params
    );

    static void DispatchGameThread(FRenderTargetCopyDispatchParams Params)
    {
        ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
        [Params](FRHICommandListImmediate& RHICmdList)
        {
            FRenderTargetCopyDispatchParams ParamsCopy = Params;
            ParamsCopy.SourceTarget = Params.SourceTexture->GetRenderTargetResource();
            ParamsCopy.DestTarget = Params.DestTexture->GetRenderTargetResource();
            DispatchRenderThread(RHICmdList, ParamsCopy);
        }
        );
    }

    static void Dispatch(FRenderTargetCopyDispatchParams Params)
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