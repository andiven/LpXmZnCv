/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "AverageHeightComputeShader.h"
#include "ExoSkyPlanetsCompute/Public/AverageHeightCompute/AverageHeightComputeShader.h"
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

class EXOSKYPLANETSCOMPUTE_API FAverageComputeShader: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FAverageComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FAverageComputeShader, FGlobalShader);
	
	
	class FAverageComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		FAverageComputeShader_Perm_TEST
	>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER_TEXTURE(Texture2D, AverageTexture)
		SHADER_PARAMETER_SAMPLER(SamplerState, AverageTextureSampler)
		SHADER_PARAMETER(FVector4f, HeightChannel)
		SHADER_PARAMETER_RDG_BUFFER_UAV(RWBuffer<float>, Output)

	END_SHADER_PARAMETER_STRUCT()

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_AverageComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_AverageComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_AverageComputeShader_Z);

		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}
private:
};

IMPLEMENT_GLOBAL_SHADER(FAverageComputeShader, "/ExoSkyPlanetsComputeShaders/AverageComputeShader/AverageComputeShader.usf", "AverageComputeShader", SF_Compute);

void FAverageComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FAverageComputeShaderDispatchParams Params, TFunction<void(float OutputVal)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);
	{
		SCOPE_CYCLE_COUNTER(STAT_AverageComputeShader_Execute);
		DECLARE_GPU_STAT(AverageComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "AverageComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, AverageComputeShader);
		
		typename FAverageComputeShader::FPermutationDomain PermutationVector;
		TShaderMapRef<FAverageComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		
		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FAverageComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FAverageComputeShader::FParameters>();

			PassParameters->AverageTexture = Params.AverageHeight;
			PassParameters->AverageTextureSampler = TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();
			PassParameters->HeightChannel = Params.HeightChannel;

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateBufferDesc(sizeof(float), 1),
				TEXT("OutputBuffer"));

			PassParameters->Output = GraphBuilder.CreateUAV(FRDGBufferUAVDesc(OutputBuffer, PF_R32_FLOAT));
			
			auto GroupCount = FIntVector(Params.X, Params.Y, Params.Z);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteAverageComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
			});
			
			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteAverageComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, sizeof(float));

			auto RunnerFunc = [GPUBufferReadback, AsyncCallback, Params](auto&& RunnerFunc) -> void {
				if (GPUBufferReadback->IsReady()) {
					
					float* Buffer = (float*)GPUBufferReadback->Lock(0);
					float OutVal = Buffer[0];
					
					if (Params.AverageHeight != nullptr && IsValid(Params.World)) {
						AsyncTask(ENamedThreads::GameThread, [AsyncCallback, OutVal, Params]() {
							AsyncCallback(OutVal);
						});
					}
					GPUBufferReadback->Unlock();

					delete GPUBufferReadback;
				} else {
					if (Params.AverageHeight != nullptr && IsValid(Params.World)) {
						AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc, Params]() {
							RunnerFunc(RunnerFunc);
						});
					}
				}
			};
			if(IsValid(Params.World) && Params.AverageHeight != nullptr) {
				AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc, Params]() {
					RunnerFunc(RunnerFunc);
				});
			}	
		}
	}

	GraphBuilder.Execute();
}