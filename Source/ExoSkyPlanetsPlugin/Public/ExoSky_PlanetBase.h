/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once

#if WITH_EDITOR
#include "Editor.h"
#endif

#include "Engine/World.h" 
#include "CoreMinimal.h"
#include "Math/Vector.h" 
#include "UObject/Object.h"
#include "Delegates/Delegate.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h" 
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Components/SceneComponent.h" 
#include "Math/TransformNonVectorized.h" 
#include "GameFramework/Character.h" 
#include "ExoSkyPlanetsPlugin\Public\ExoSky_PlanetMeshProvider.h"
#include "ExoSkyPlanetsShared\Public\ExoSky_PlanetFunctionsLibrary.h"
#include "ExoSkyPlanetsCompute\Public\AverageHeightCompute\AverageHeightComputeShader.h"
#include "ExoSkyPlanetsCompute\Public\VertexCompute\VertexComputeShader.h"
#include "ExoSkyPlanetsCompute\Public\RenderTargetCompute\RenderTargetComputeShader.h"
#include "ExoSkyPlanetsCompute\Public\RenderTargetCompute\RenderTargetCopy.h"

#include "ExoSky_PlanetBase.generated.h"

class ExoSky_PlanetMeshProvider;

USTRUCT(BlueprintType)
struct FPlanetNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Default)
		uint8 QID = 0;
	UPROPERTY(EditAnywhere, Category = Default)
		uint8 LOD = 0;
	UPROPERTY(EditAnywhere, Category = Default)
		float Scale = 1.0f;
	UPROPERTY(EditAnywhere, Category = Default)
		FVector ParentLocation = FVector(0.0f,0.0f,0.0f);
	UPROPERTY(EditAnywhere, Category = Default)
		FVector QuadLocation = FVector(0.0f,0.0f,0.0f);
	UPROPERTY(EditAnywhere, Category = Default)
		FVector NodeCenter = FVector(0.0f,0.0f,0.0f);
	UPROPERTY(EditAnywhere, Category = Default)
		FRotator Rotation = FRotator(0.0f, 0.0f, 0.0f);
	UPROPERTY(EditAnywhere, Category = Default)
		UMaterialInstanceDynamic * RenderMaterial = nullptr;
	UPROPERTY(EditAnywhere, Category = Default)
		TArray<FVector> Children;
	UPROPERTY(EditAnywhere, Transient, Category = Default)
		TMap<FName, UTextureRenderTarget2D*> TemporaryCaches;
	UPROPERTY(EditAnywhere, Transient, Category = Default)
		TMap<FName, UTextureRenderTarget2D*> PersistentCaches;
	UPROPERTY(EditAnywhere, Category = Default)
		FMeshAttributes MeshInfo;
	UPROPERTY(EditAnywhere, Category = Default)
		bool ChildrenBuilt = false;
	UPROPERTY(EditAnywhere, Category = Default)
		ENodeProcess NodeProcess = ENodeProcess::Empty;
};

USTRUCT(BlueprintType)
struct FLevelOfDetailSettings
{
	GENERATED_BODY()

	// Distance multiplier for subdivision.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ClampMin = "1.", ClampMax = "2.0" ))
		float SubdivisionDistanceMultiplier = 1.25;
	// Minimum vertex spacing in meters. Note: This will not always be accurate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ClampMin = "1", ClampMax ="100000000000000"))
		int VertexSpacingInMeters = 1;
	// Number of meshes to attempt to build per frame.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ClampMin = "4", ClampMax ="64"))
		int ProcessesPerFrame = 4;
};

USTRUCT(BlueprintType)
struct FOptimizationSettings
{
	GENERATED_BODY()
	//Memory intensive. This will reduce stepping or artifacts in normals.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderingSettings, meta = (ExposeOnSpawn = "true"))
		bool UseHighQualityNormals = false;
	//Clears the Generator memory after terrain generation. This will disable Generator Height / RGBA use in Materials.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RenderingSettings, meta = (ExposeOnSpawn = "true"))
		bool ClearGeneratorMemory = false;
	// Disable if terrain is invisible or incredibly laggy. Report the bug on the discord.
	// Disabled until I can find a fix for 5.4 and the instability when using nested loops.
		bool UseComputeBackend = false;
};

USTRUCT(BlueprintType)
struct FResampleSettings
{
	GENERATED_BODY()
	// Attempts to smooth out precision errors that may occur on generators. 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		bool ResampleGenerator = false;
	// Level to start resampling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default)
		int MaximumResolutionInCM = 1;
};

UCLASS(Abstract, BlueprintType)
class EXOSKYPLANETSPLUGIN_API AExoskyPlanetBase : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, Transient, SkipSerialization)
		bool RefreshSurface = false;
	//Radius In Kilometers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (DisplayName="Ground Radius", ForceUnits="Kilometers", ClampMin = "1.", ExposeOnSpawn = "true"))
		float GroundRadiusInKM = 1000.0;
	//Terrain Height Multiplier (Example: If the height is 1, and terrain multiplier is 10; 1 * 10 = 10 UU)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
		float TerrainHeightMultiplier = 400000.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
		TEnumAsByte<EMeshResolution> MeshResolution = x64;
	//Pixels between vertices on the mesh.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
		int PixelsPerFace = 4;
	//Height Channel that is chosen from the Generator for terrain height.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
		TEnumAsByte<EHeightChannel> HeightChannel = EHeightChannel::R;
	//Try to keep this to R32f. The more channels, the more memory you will have. Try to keep biome/texturing information in layers, not in the main generator.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
		TEnumAsByte<EGeneratorFormat> GeneratorFormat = EGeneratorFormat::r32f;

	//Material that will be used to determine surface generation. Generator is primarily used for determining height.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
		UMaterialInterface* GeneratorMaterial = nullptr;
	//Material that will be viewed in game and rendered on the planet surface.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (ExposeOnSpawn = "true"))
		UMaterialInterface* RenderMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Default, meta = (TitleProperty="Name", DisplayName = "Layers", ExposeOnSpawn = "true"))
		TArray<FLayerData> LayersData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering, meta = (DisplayName = "Level of Detail Settings", ExposeOnSpawn = "true"))
		FLevelOfDetailSettings LODSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering, meta = (ExposeOnSpawn = "true"))
		FOptimizationSettings OptimizationSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering, meta = (ExposeOnSpawn = "true"))
		FResampleSettings ResampleSettings;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Rendering, meta = (ShowOnlyInnerProperties, ExposeOnSpawn = "true"))
		FRenderingSettings RenderingSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Settings", meta = (ExposeOnSpawn = "true"))
		bool GenerateCollision = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision Settings", meta = (ExposeOnSpawn = "true", ShowOnlyInnerProperties))
		FBodyInstance CollisionSettings;

	UFUNCTION(BlueprintCallable, Category = Default)
		void Refresh();

protected:

	bool IsLevelBoundsRelevant() const override;
#if WITH_EDITOR
	bool ShouldTickIfViewportsOnly() const override;
	// bool ReregisterComponentsWhenModified() const override;
	void OnPie(bool Something);
	void EndPie(bool Something);
	
	void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent) override;
	void DebugLogMessages(EMaterialTypeExo LogType, FName LayerName, UMaterialInterface* Material);
#endif

	AExoskyPlanetBase();

	void Tick(float DeltaTime) override;	
	void BeginPlay() override;
	void Destroyed() override;

	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = Default)
	void EditorStartup();
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContext"), Category = Default)
	void Update();

	void InitSectionScale();
	void InitRadiusInUU();
	void InitMaxSubdivisionLevel();
	void InitFormats();
	void InitCollisionMesh();

	FVector TransformQuad(FVector Pos);

	void PoolRT(FName TextureName);
	void EmptyRTPool();
	bool PoolAvailable();

	UTextureRenderTarget2D* GetFromAvailable(FName TextureName);
	void AddToAvailable(FName TextureName, UTextureRenderTarget2D* Texture);

#if WITH_EDITOR
	bool ShouldRecompileForCompute(UMaterialInterface* Material);
	bool ShouldRecompileForRendering(UMaterialInterface* Material);
	bool IsMaterialCompiling(UMaterialInterface* InMaterial);
	bool CheckGeneratorCompiling(UMaterialInterface* InMaterial);
	bool CheckRenderCompiling(UMaterialInterface* InMaterial);
	void UpdateCompilingMaterials();
	bool IsCompiling();
#endif
	UMaterialInterface* CheckGeneratorNull(UMaterialInterface* Material);

	FVector GetPlayerLocation();
	void SmoothVelocity();
	bool VelocityCheck(float SectionScale, FVector NodeCenter);
	bool DistanceCheck(FVector NodeLocation);
	bool QuadtreeCheck(FVector NodeLocation);

	void SplitNode(FVector NodeLocation);
	void UnSplitNode(FVector NodeLocation);

	FVector CreateNode(FVector ParentLocation, uint8 QID);
	void DestroyNodeCache(FVector NodeLocation);
	void DestroyNode(FVector NodeLocation);
	void EmptyTree();

	ETextureRenderTargetFormat OptimizeMemoryFormat(int LOD, bool OptimizeMemory, ETextureRenderTargetFormat Format);
	void RenderToRT(UMaterialInterface* Material, UTextureRenderTarget2D* Texture);
	void CreateNodeCache(FVector Lookup);
	void AsyncCreateRenderTarget2D(FName TextureName, UTextureRenderTarget2D* RenderTarget, FVector Lookup);
	UMaterialInstanceDynamic* SetupParameters(UMaterialInterface* RefMaterial, FPlanetNode ThisNode, int PIXELSPERFACE );

	void DispatchNodeReadback(FVector Lookup);

	UPROPERTY(Transient, DuplicateTransient)
		bool IsInPlay = false;

	UPROPERTY(Transient, DuplicateTransient)
		TMap<FVector, FPlanetNode> NodesData;

	TArray<FVector> QIDLookup = 
	{
		FVector(1.0f, -1.0f, 0.0f),
		FVector(1.0f, 1.0f, 0.0f),
		FVector(-1.0f, -1.0f, 0.0f),
		FVector(-1.0f, 1.0f, 0.0),
	};

	TArray<ETextureRenderTargetFormat> OptimizeRTList =
	{
		ETextureRenderTargetFormat::RTF_R32f,
		ETextureRenderTargetFormat::RTF_RG32f,
		ETextureRenderTargetFormat::RTF_RGBA32f,

		ETextureRenderTargetFormat::RTF_R16f,
		ETextureRenderTargetFormat::RTF_RG16f,
		ETextureRenderTargetFormat::RTF_RGBA16f,
	};

	UPROPERTY(Transient, DuplicateTransient)
		TMap<FName, FPooledTargets> PooledTerrainTargets;

	UPROPERTY(Transient, DuplicateTransient)
		TArray<FVector2f> UVs;

	UPROPERTY(Transient, DuplicateTransient)
		TArray<UMaterial*> CompilingMaterials;
	
	UMaterialInterface * NormalComputeMat;
	UMaterialInterface * PositionComputeMat;
	UMaterialInterface * ErrorMat;
	UMaterialInterface * ResampleMat;
	UMaterialInterface * CopyMat;
	UMaterialInterface * WireframeMat;

	UTexture2D* BlueNoiseTexture;
	UTexture2D* BlueNoiseVectorTexture;

	FVector SmoothedVelocity = FVector(0.0,0.0,0.0);
	FVector PlayerLocation = FVector(0.0,0.0,0.0);
	float SectionScale = 0.0;
	int MeshRes = 64;
	int MaxSubdivisionLevel;
	int ResampleLevel;
	float RadiusInUU = 0.0;
	int GeneratorResolution = 0;
	FLinearColor HeightChannelValue = FLinearColor(1.0f, 0.0f, 0.0f, 0.0f);
	TEnumAsByte<ETextureRenderTargetFormat> GeneratorFormatRT = RTF_R32f;

	UPROPERTY(Transient, DuplicateTransient)
	bool bShouldStart = true;
	UPROPERTY(Transient, DuplicateTransient)
	bool bInitialStart = true;
	UPROPERTY(Transient, DuplicateTransient)
	bool bRTPoolCreated = false;
	UPROPERTY(Transient, DuplicateTransient)
	bool bCreateCollision = true;

	int FullPrecisionLevel;
};