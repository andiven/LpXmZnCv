/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/TextureRenderTarget.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialRenderProxy.h"
#include "ExoSkyPlanetsCompute.h"

struct FVertexOutput
{
	FVector4f VertexPosition;
	FVector4f VertexNormal;
};

struct EXOSKYPLANETSCOMPUTE_API FVertexComputeShaderDispatchParams
{
	int X;
	int Y;
	int Z;

	uint32 Width = 1;
	FVector3f RebasePosition;
	float PlanetRadius;
	FRHITexture* PositionTexture;
	FRHITexture* NormalTexture;
	TArray<FVertexOutput> Output;
	UObject* World;

	FVertexComputeShaderDispatchParams(int x, int y, int z)
		: X(x)
		, Y(y)
		, Z(z)
	{
	}
};

DECLARE_CYCLE_STAT(TEXT("Mesh Compute"), STAT_VertexComputeShader_Execute, STATGROUP_ExoskyPlanetsCompute);
class EXOSKYPLANETSCOMPUTE_API FVertexComputeShaderInterface {
public:
	static void DispatchRenderThread(
		FRHICommandListImmediate& RHICmdList,
		FVertexComputeShaderDispatchParams Params,
		TFunction<void(TArray<FVector3f> Vertices, TArray<FVector4f> Normals)> AsyncCallback
	);

	static void DispatchGameThread(
		FVertexComputeShaderDispatchParams Params, UTextureRenderTarget2D* PositionTexture, UTextureRenderTarget2D* NormalTexture,
		TFunction<void(TArray<FVector3f> Vertices, TArray<FVector4f> Normals)> AsyncCallback
	)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[Params, PositionTexture, NormalTexture, AsyncCallback](FRHICommandListImmediate& RHICmdList)
		{
			FVertexComputeShaderDispatchParams Params2(Params.X, Params.Y, Params.Z);
			Params2 = Params;
			Params2.PositionTexture = PositionTexture->GetRenderTargetResource()->GetTexture2DRHI();
			Params2.NormalTexture = NormalTexture->GetRenderTargetResource()->GetTexture2DRHI();
			DispatchRenderThread(RHICmdList, Params2, AsyncCallback);
		});
	}

	static void Dispatch(
		FVertexComputeShaderDispatchParams Params,  
		TFunction<void(TArray<FVector3f> Vertices, TArray<FVector4f> Normals)> AsyncCallback
	)
	{
		if (IsInRenderingThread()) {
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), Params, AsyncCallback);
		}else{
			//DispatchGameThread(Params, PositionTexture NormalTexture EdgeTexture AsyncCallback);
		}
	}
};