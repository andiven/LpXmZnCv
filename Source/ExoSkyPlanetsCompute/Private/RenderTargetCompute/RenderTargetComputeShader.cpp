/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "RenderTargetComputeShader.h"
#include "ExoSkyPlanetsCompute/Public/RenderTargetCompute/RenderTargetComputeShader.h"
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

class EXOSKYPLANETSCOMPUTE_API FRenderTargetComputeShader: public FMeshMaterialShader
{
public:
    DECLARE_SHADER_TYPE(FRenderTargetComputeShader, MeshMaterial);
    SHADER_USE_PARAMETER_STRUCT_WITH_LEGACY_BASE(FRenderTargetComputeShader, FMeshMaterialShader)

    class FRenderTargetComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("Compute to RT", 6);
    using FPermutationDomain = TShaderPermutationDomain<FRenderTargetComputeShader_Perm_TEST>;

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, RenderTarget)
        SHADER_PARAMETER(uint32, Width)

    END_SHADER_PARAMETER_STRUCT()

public:
    static bool ShouldCompilePermutation(const FMaterialShaderPermutationParameters& Parameters)
    {
        const FPermutationDomain PermutationVector(Parameters.PermutationId);

        const bool bIsCompatible = Parameters.MaterialParameters.bIsUsedWithGeometryCache;
        return bIsCompatible;
    }

    static void ModifyCompilationEnvironment(const FMaterialShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    {
        FMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
        
        const FPermutationDomain PermutationVector(Parameters.PermutationId);

        OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_RenderTargetComputeShader_X);
        OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_RenderTargetComputeShader_Y);
        OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_RenderTargetComputeShader_Z);
    }
};


IMPLEMENT_MATERIAL_SHADER_TYPE(, FRenderTargetComputeShader, TEXT("/ExoSkyPlanetsComputeShaders/RenderTargetComputeShader/RenderTargetComputeShader.usf"), TEXT("RenderTargetComputeShader"), SF_Compute);

void FRenderTargetComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FRenderTargetComputeShaderDispatchParams Params)
{
    FRDGBuilder GraphBuilder(RHICmdList);
    {
        SCOPE_CYCLE_COUNTER(STAT_RenderTargetComputeShader_Execute);
        DECLARE_GPU_STAT(RenderTargetComputeShader);
        RDG_EVENT_SCOPE(GraphBuilder, "RenderTargetComputeShader");
        RDG_GPU_STAT_SCOPE(GraphBuilder, RenderTargetComputeShader);

        const FSceneInterface* LocalScene = Params.Scene;
        const FMaterialRenderProxy* MaterialRenderProxy = nullptr;
        const FMaterial* MaterialResource = &Params.MaterialRenderProxy->GetMaterialWithFallback(GMaxRHIFeatureLevel, MaterialRenderProxy);
        MaterialRenderProxy = MaterialRenderProxy ? MaterialRenderProxy : Params.MaterialRenderProxy;

        typename FRenderTargetComputeShader::FPermutationDomain PermutationVector;

        TShaderRef<FRenderTargetComputeShader> ComputeShader = MaterialResource->GetShader<FRenderTargetComputeShader>(&FLocalVertexFactory::StaticType, PermutationVector, false);

        bool bIsShaderValid = ComputeShader.IsValid();
        if(MaterialRenderProxy && !MaterialRenderProxy->GetMaterialInterface()->CheckMaterialUsage_Concurrent(MATUSAGE_GeometryCache))
        {
            #if WITH_EDITOR
            GEngine->AddOnScreenDebugMessage((uint64) 5643264352356, 6.f, FColor::Red, FString(TEXT("Can't use the specified material because it has not been compiled with bUsedWithGeometryCache.")));
            #endif

            bIsShaderValid = false;
        }

        if(bIsShaderValid)
        {
            FRenderTargetComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FRenderTargetComputeShader::FParameters>();

            //FRDGTextureDesc Desc(FRDGTextureDesc::Create2D(Params.RenderTarget->GetSizeXY(), Params.RenderTarget2D->GetFormat(), FClearValueBinding::Black, TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV));
            //FRDGTextureRef TmpTexture = GraphBuilder.CreateTexture(Desc, TEXT("RenderTargetComputeShader_TempTexture"));
            FRDGTextureRef TargetTexture = RegisterExternalTexture(GraphBuilder, Params.RenderTarget->GetRenderTargetTexture(), TEXT("RenderTargetComputeShader_RT"));
            PassParameters->RenderTarget = GraphBuilder.CreateUAV(TargetTexture);
            PassParameters->Width = Params.RenderTarget->GetSizeXY().X;

            /*FViewUniformShaderParameters ViewUniformShaderParameters;

			ViewUniformShaderParameters.GameTime = 0.;
			ViewUniformShaderParameters.RealTime = 0.;
			ViewUniformShaderParameters.Random = 0.;
			
			auto ViewUniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(ViewUniformShaderParameters, UniformBuffer_SingleFrame);
			PassParameters->View = ViewUniformBuffer;*/

            auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(Params.X, Params.Y, Params.Z), FIntVector(32, 32, 1));            
            GraphBuilder.AddPass(
                RDG_EVENT_NAME("ExecuteRenderTargetComputeShader"),
                PassParameters,
                ERDGPassFlags::AsyncCompute,
                [&PassParameters, ComputeShader, MaterialRenderProxy, MaterialResource, LocalScene, GroupCount](FRHIComputeCommandList& RHICmdList)
            {
                FMeshPassProcessorRenderState DrawRenderState;

                MaterialRenderProxy->UpdateUniformExpressionCacheIfNeeded(LocalScene->GetFeatureLevel());

                FMeshMaterialShaderElementData ShaderElementData;

                FMeshProcessorShaders PassShaders;
                PassShaders.ComputeShader = ComputeShader;

                FMeshDrawShaderBindings ShaderBindings;
                ShaderBindings.Initialize(PassShaders);

                int32 DataOffset = 0;
                FMeshDrawSingleShaderBindings SingleShaderBindings = ShaderBindings.GetSingleShaderBindings(SF_Compute, DataOffset);
                ComputeShader->GetShaderBindings(LocalScene->GetRenderScene(), LocalScene->GetFeatureLevel(), nullptr, *MaterialRenderProxy, *MaterialResource, DrawRenderState, ShaderElementData, SingleShaderBindings);
                
                ShaderBindings.Finalize(&PassShaders);

                UE::MeshPassUtils::Dispatch(RHICmdList, ComputeShader, ShaderBindings, *PassParameters, GroupCount);
            });


            //if(TargetTexture->Desc.Format ==  Params.RenderTarget2D->GetFormat())
            //{
            //    AddCopyTexturePass(GraphBuilder, TmpTexture, TargetTexture, FRHICopyTextureInfo());
            //}
        }
    }
    GraphBuilder.Execute();
}