/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/TextureRenderTarget.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "ExoSkyPlanetsCompute.h"

struct EXOSKYPLANETSCOMPUTE_API FAverageComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	
	FRHITexture* AverageHeight;
	FVector4f HeightChannel;
	float Output;
	UObject* World;

	FAverageComputeShaderDispatchParams(int x, int y, int z)
		: X(x)
		, Y(y)
		, Z(z)
	{
	}
};

DECLARE_CYCLE_STAT(TEXT("Average Compute"), STAT_AverageComputeShader_Execute, STATGROUP_ExoskyPlanetsCompute);
class EXOSKYPLANETSCOMPUTE_API FAverageComputeShaderInterface {
public:
	static void DispatchRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FAverageComputeShaderDispatchParams Params,
		TFunction<void(float OutputVal)> AsyncCallback
	);

	static void DispatchGameThread(
		FAverageComputeShaderDispatchParams Params, UTextureRenderTarget2D* RT,
		TFunction<void(float OutputVal)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[Params, RT, AsyncCallback](FRHICommandListImmediate& RHICmdList)
		{
			FAverageComputeShaderDispatchParams Params2(1, 1, 1);
			Params2 = Params;
			Params2.AverageHeight = RT->GetRenderTargetResource()->GetTexture2DRHI();
			DispatchRenderThread(RHICmdList, Params2, AsyncCallback);
		});
	}

	static void Dispatch(
		FAverageComputeShaderDispatchParams Params,  
		TFunction<void(float OutputVal)> AsyncCallback
	)
	{
		if (IsInRenderingThread()) {
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}else{
			//DispatchGameThread(Params, AsyncCallback);
		}
	}
};