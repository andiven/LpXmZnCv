/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "VertexComputeShader.h"
#include "ExoSkyPlanetsCompute/Public/VertexCompute/VertexComputeShader.h"
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
#include "ExoSkyPlanetsCompute.h"

class EXOSKYPLANETSCOMPUTE_API FVertexComputeShader: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FVertexComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FVertexComputeShader, FGlobalShader);
	
	
	class FVertexComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("Surface Vertex Compute", 4);
	using FPermutationDomain = TShaderPermutationDomain<
		FVertexComputeShader_Perm_TEST
	>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )

		SHADER_PARAMETER(uint32, Width)
		SHADER_PARAMETER(FVector3f, RebasePosition)
		SHADER_PARAMETER(float, PlanetRadius)
		SHADER_PARAMETER_TEXTURE(Texture2D, PositionTexture)
		SHADER_PARAMETER_TEXTURE(Texture2D, NormalTexture)

		SHADER_PARAMETER_SAMPLER(SamplerState, BilinearSampler)
		SHADER_PARAMETER_SAMPLER(SamplerState, PointSampler)

		SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<FVertexOutput>, Output)

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

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_VertexComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_VertexComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_VertexComputeShader_Z);

		OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);
	}
private:
};

IMPLEMENT_GLOBAL_SHADER(FVertexComputeShader, "/ExoSkyPlanetsComputeShaders/VertexComputeShader/VertexComputeShader.usf", "VertexComputeShader", SF_Compute);

void FVertexComputeShaderInterface::DispatchRenderThread(FRHICommandListImmediate& RHICmdList, FVertexComputeShaderDispatchParams Params, TFunction<void(TArray<FVector3f> Vertices, TArray<FVector4f> Normals)> AsyncCallback) {
	FRDGBuilder GraphBuilder(RHICmdList);

	{
		SCOPE_CYCLE_COUNTER(STAT_VertexComputeShader_Execute);
		DECLARE_GPU_STAT(VertexComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "VertexComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, VertexComputeShader);
		
		typename FVertexComputeShader::FPermutationDomain PermutationVector;
		
		TShaderMapRef<FVertexComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);

		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FVertexComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FVertexComputeShader::FParameters>();

			PassParameters->Width = Params.Width;
			PassParameters->RebasePosition = Params.RebasePosition;
			PassParameters->PlanetRadius = Params.PlanetRadius;
			PassParameters->PositionTexture = Params.PositionTexture;
			PassParameters->NormalTexture = Params.NormalTexture;

			PassParameters->BilinearSampler = TStaticSamplerState<ESamplerFilter::SF_Bilinear, ESamplerAddressMode::AM_Clamp, ESamplerAddressMode::AM_Clamp>::GetRHI();
			PassParameters->PointSampler = TStaticSamplerState<ESamplerFilter::SF_Point, ESamplerAddressMode::AM_Clamp, ESamplerAddressMode::AM_Clamp>::GetRHI();

			FRDGBufferRef OutputBuffer = GraphBuilder.CreateBuffer(
				FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector4f) * 2, Params.Width * Params.Width),
				TEXT("OutputBuffer"));

			PassParameters->Output = GraphBuilder.CreateUAV(OutputBuffer, ERDGUnorderedAccessViewFlags::None);
			
			auto GroupCount = FIntVector(Params.X, Params.Y, Params.Z);
			GraphBuilder.AddPass(
				RDG_EVENT_NAME("ExecuteVertexComputeShader"),
				PassParameters,
				ERDGPassFlags::AsyncCompute,
				[&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
			{
				FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
			});

			int32 Length = int32(Params.Width * Params.Width);
			
			FRHIGPUBufferReadback* GPUBufferReadback = new FRHIGPUBufferReadback(TEXT("ExecuteVertexComputeShaderOutput"));
			AddEnqueueCopyPass(GraphBuilder, GPUBufferReadback, OutputBuffer, Length * sizeof(FVector4f) * 2);

			auto RunnerFunc = [GPUBufferReadback, AsyncCallback, Params, Length](auto&& RunnerFunc) -> void {
				if (GPUBufferReadback->IsReady()) {
					//Access each 32 bit interval of the buffer. For each element of the 2 float4 vectors. Instead of doing some weird indexing shit with a FVector4* buffer.
					float* Buffer = (float*)GPUBufferReadback->Lock(Length * sizeof(FVector4f) * 2);

					if (Params.PositionTexture != nullptr && IsValid(Params.World)) {
						AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask , [AsyncCallback, Buffer, Params, Length]() {
							TArray<FVector3f> Vertices;
							TArray<FVector4f> Normals;

							Vertices.SetNum(Length);
							Normals.SetNum(Length);
							
							int RowWidth = Params.Width;

							for(int32 i = 0; i < Length; i++)
							{
								float* Element = &Buffer[i*8];

								int32 Index = int32(Element[3]);
								Vertices[Index] = FVector3f(Element[0], Element[1], Element[2]) * Params.PlanetRadius;
								Normals[Index] = FVector4f(Element[4], Element[5], Element[6], Element[7]);
							}

							AsyncTask(ENamedThreads::GameThread, [AsyncCallback, Vertices, Normals, Params]() {
								AsyncCallback(Vertices, Normals);
							});
							Vertices.Empty();
							Normals.Empty();
						});
					}
					GPUBufferReadback->Unlock();

					delete GPUBufferReadback;
				} else {
					if (Params.PositionTexture != nullptr && IsValid(Params.World)) {
						AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc, Params]() {
							RunnerFunc(RunnerFunc);
						});
					}
				}
			};
			if (Params.PositionTexture != nullptr && IsValid(Params.World)) {
				AsyncTask(ENamedThreads::ActualRenderingThread, [RunnerFunc, Params]() {
					RunnerFunc(RunnerFunc);
				});
			}
		} 
	}

	GraphBuilder.Execute();
}