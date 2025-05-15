/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "RenderTargetCopy.h"
#include "ExoSkyPlanetsCompute/Public/RenderTargetCompute/RenderTargetCopy.h"
#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MeshDrawShaderBindings.h"
#include "RHIGPUReadback.h"
#include "MeshPassUtils.h"
#include "MaterialShader.h"
#include "MaterialShared.h"

void FRenderTargetCopyInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FRenderTargetCopyDispatchParams Params)
{
    FRDGBuilder GraphBuilder(RHICmdList);
    {
       //FRDGTextureDesc Desc(FRDGTextureDesc::Create2D(Params.RenderTarget->GetSizeXY(), Params.RenderTarget2D->GetFormat(), FClearValueBinding::Black, TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV));
       //FRDGTextureRef TmpTexture = GraphBuilder.CreateTexture(Desc, TEXT("RenderTargetComputeShader_TempTexture"));
       FRDGTextureRef TmpTexture = RegisterExternalTexture(GraphBuilder, Params.SourceTarget->GetRenderTargetTexture(), TEXT("RenderTargetComputeShader_RT"));
       FRDGTextureRef TargetTexture = RegisterExternalTexture(GraphBuilder, Params.DestTarget->GetRenderTargetTexture(), TEXT("RenderTargetComputeShader_RT2"));
       
       //FRHICommandCopyTexture(Params.RenderTarget->GetRenderTargetTexture(), Params.TextureTarget->GetRHI());

       AddCopyTexturePass(GraphBuilder, TmpTexture, TargetTexture, FRHICopyTextureInfo());
    }
   GraphBuilder.Execute();
}