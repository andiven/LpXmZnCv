/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h" 
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetRenderingLibrary.h" 
#include "Kismet/KismetMathLibrary.h"
#include "Math/TransformNonVectorized.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CoreFwd.h" 
#include "GameFramework/Character.h" 

#include "ExoSky_PlanetFunctionsLibrary.generated.h"

UENUM()
enum EMeshResolution
{
	x32         UMETA(DisplayName = "32x32"),
	x64         UMETA(DisplayName = "64x64"),
	x96         UMETA(DisplayName = "96x96"),
	x128        UMETA(DisplayName = "128x128"),
};

UENUM()
enum EGeneratorFormat
{
	r32f		UMETA(DisplayName = "R32f"),
	rg32f       UMETA(DisplayName = "RG32f"),
	rgba32f     UMETA(DisplayName = "RGBA32f"),
};

UENUM()
enum EHeightChannel 
{
	R,
	G,
	B,
};

UENUM()
enum EMaterialTypeExo 
{
	Generator,
	Layer,
	Render,
};

UENUM()
enum class ENodeProcess : uint8
{
	Empty,
	Processed,
	TexturesFinished,
	MeshProcessing,
	CollisionBuilt
};

USTRUCT()
struct FPooledTargets
{
	GENERATED_BODY()

	int NumPooled = 0;
	UPROPERTY(Transient)
	TArray<UTextureRenderTarget2D*> Taken;
	UPROPERTY(Transient)
	TArray<UTextureRenderTarget2D*> Available;
};

USTRUCT(BlueprintType)
struct FGeneratorParameters
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		TMap<FName, float> ScalarParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		TMap<FName, FLinearColor> VectorParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		TMap<FName, UTexture*> TextureParameters;
};

USTRUCT(BlueprintType)
struct FLayerData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		FName Name = "None";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		UMaterialInterface* LayerGenerator = nullptr;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
		FGeneratorParameters GeneratorParameters;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		TEnumAsByte<ETextureRenderTargetFormat> LayerFormat = RTF_RGBA8;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		int PixelsPerFace = 4;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//	bool GenerateMips = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		bool OverwriteGenerator = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		bool OptimizeMemoryUsage = true;
	
	UMaterialInterface* LayerGeneratorSetup;
};

UENUM(BlueprintType)
enum EFoliageLayerEditType
{
	ASSET    UMETA(DisplayName = "From Asset"),
	MANUAL   UMETA(DisplayName = "From Attributes")
};

USTRUCT(BlueprintType)
struct FRenderingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderingSettings, meta = (ExposeOnSpawn = "true"))
	bool CastShadow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderingSettings, meta = (ExposeOnSpawn = "true"))
	FLightingChannels LightingChannels;
};

static TArray<FRotator> GetExoCubeFaces()
{
	return TArray<FRotator>
	{
		FRotator(90.0, 0.0, 0.0),
		FRotator(-90.0, 0.0, 0.0),
		FRotator(0.0, 0.0, 90.0),
		FRotator(0.0, 0.0, -90.0),
		FRotator(0.0, 0.0, 180.0),
		FRotator(0.0, 0.0, 0.0)
	};
};

static UMaterialInstanceDynamic* GetSetupGenerator(FLayerData Layer, UObject* InOuter)
{
	UMaterialInstanceDynamic* SetupGenerator = UMaterialInstanceDynamic::Create(Layer.LayerGenerator, InOuter);

	for(auto& Scalar : Layer.GeneratorParameters.ScalarParameters)
	{
		SetupGenerator->SetScalarParameterValue(Scalar.Key, Scalar.Value);
	}
	for(auto& Vector : Layer.GeneratorParameters.VectorParameters)
	{
		SetupGenerator->SetVectorParameterValue(Vector.Key, Vector.Value);
	}
	for(auto& Texture : Layer.GeneratorParameters.TextureParameters)
	{
		SetupGenerator->SetTextureParameterValue(Texture.Key, Texture.Value);
	}

	return SetupGenerator;
}

static FVector GetEditorViewLocation(FVector Fallback, UObject* WorldContext)
{
	auto World = WorldContext->GetWorld();
	if(World == nullptr)
	{
		return FVector(0.0,0.0,0.0);
	}
	auto ViewLocations = World->ViewLocationsRenderedLastFrame;
	if(ViewLocations.Num() == 0)
	{
		return Fallback;
	}
	FVector CurrentEditorViewLocation = ViewLocations[0];
	return CurrentEditorViewLocation;
}

static FVector CubeToSphere(FVector P)
{
	FVector N;
	N = FVector(1.0, 1.0, 1.0) - (FVector((P * P).Y, (P * P).X, (P * P).X) + FVector((P * P).Z, (P * P).Z, (P * P).Y)) / 2 + FVector((P * P).Y, (P * P).X, (P * P).X) * FVector((P * P).Z, (P * P).Z, (P * P).Y) / 3.0;
	N = P * FVector(sqrt(N.X), sqrt(N.Y), sqrt(N.Z));

	return N;
}