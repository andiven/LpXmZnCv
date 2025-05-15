/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "ExoSky_PlanetBase.h"

AExoskyPlanetBase::AExoskyPlanetBase()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> tNormalComputeMat(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Backend/Compute/C_Normal.C_Normal'"));

	if(tNormalComputeMat.Succeeded())
	{
		NormalComputeMat = tNormalComputeMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> tPositionComputeMat(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Backend/Compute/C_Position.C_Position'"));

	if(tPositionComputeMat.Succeeded())
	{
		PositionComputeMat = tPositionComputeMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> tCopyMat(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Backend/Compute/C_Copy.C_Copy'"));

	if(tCopyMat.Succeeded())
	{
		CopyMat = tCopyMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> tErrorMat(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Backend/Compute/C_Error.C_Error'"));

	if(tErrorMat.Succeeded())
	{
		ErrorMat = tErrorMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> tResampleMat(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Backend/Compute/C_Resample.C_Resample'"));

	if(tResampleMat.Succeeded())
	{
		ResampleMat = tResampleMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> tWireframeMat(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Backend/Compute/C_Wireframe.C_Wireframe'"));

	if(tWireframeMat.Succeeded())
	{
		WireframeMat = tWireframeMat.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> tBlueNoiseScalar(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Extras/EngineTextures/STBlueNoise_scalar_128x128x64.STBlueNoise_scalar_128x128x64'"));

	if(tBlueNoiseScalar.Succeeded())
	{
		BlueNoiseTexture = tBlueNoiseScalar.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture2D> tBlueNoiseVector(TEXT("/Script/Engine.Material'/ExoSkyPlanetsPlugin/Extras/EngineTextures/STBlueNoise_vec2_128x128x64.STBlueNoise_vec2_128x128x64'"));

	if(tBlueNoiseVector.Succeeded())
	{
		BlueNoiseVectorTexture = tBlueNoiseVector.Object;
	}
}

bool AExoskyPlanetBase::IsLevelBoundsRelevant() const
{
	return false;
}
#if WITH_EDITOR
bool AExoskyPlanetBase::ShouldTickIfViewportsOnly() const
{
	return true;
}

void AExoskyPlanetBase::OnPie(bool Something)
{
	EmptyTree();
	EmptyRTPool();
}
void AExoskyPlanetBase::EndPie(bool Something)
{
	EmptyTree();
	EmptyRTPool();
	if(!IsInPlay)
	{
		IsInPlay = false;
		bShouldStart = true;
	}
}

void AExoskyPlanetBase::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	RefreshSurface = false;
	CompilingMaterials.Empty();

	FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if(PropertyName == "MeshResolution")
	{
		int NewRes = MeshRes;
		switch(MeshResolution)
		{
			case EMeshResolution::x32:
				NewRes = 32;
				break;
			case EMeshResolution::x64:
				NewRes = 64;
				break;
			case EMeshResolution::x96:
				NewRes = 96;
				break;
			case EMeshResolution::x128:
				NewRes = 128;
				break;
			default:
				break;
		}
		PixelsPerFace = int(float(PixelsPerFace) * float(MeshRes) / float(NewRes));

		for(int i = 0; i < LayersData.Num(); i++)
		{
			int LayerPPF = LayersData[i].PixelsPerFace;
			LayerPPF = int(float(LayerPPF) * float(MeshRes) / float(NewRes));
			LayersData[i].PixelsPerFace = LayerPPF;
		}

		if(GeneratorResolution != (NewRes * PixelsPerFace)) {EmptyRTPool();}
	}

	// Logging
	DebugLogMessages(EMaterialTypeExo::Generator, "Generator", GeneratorMaterial);
	DebugLogMessages(EMaterialTypeExo::Render, "Render", RenderMaterial);

	TArray<FName> PastLayers;
	for(FLayerData Layer : LayersData)
	{
		if(PastLayers.Contains(Layer.Name))
		{
			FString LayerLog = this->GetName() + " Layer " + Layer.Name.ToString() + " has multiple occurrences.";
			UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
			GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
		}
		DebugLogMessages(EMaterialTypeExo::Layer, Layer.Name, Layer.LayerGenerator);
		PastLayers.Add(Layer.Name);
	}
	PastLayers.Empty();

	TArray<FName> RefreshPoolProperties = {
		"PixelsPerFace",
		"UseHighQualityGenerator",
		"UseHighQualityNormals",
		"ProcessesPerFrame",
	};
	TArray<FName> NoRefreshProperties = {
		"SubdivisionDistanceMultiplier",
		"VertexSpacingInMeters",
	};
	
	if(PropertyName == "VertexSpacingInMeters")
	{
		InitMaxSubdivisionLevel();
	}
	if(RefreshPoolProperties.Contains(PropertyName))
	{
		EmptyRTPool();
	}

	if(!NoRefreshProperties.Contains(PropertyName))
	{
		EmptyTree();
		bShouldStart = true;
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *PropertyName.ToString());
}

void AExoskyPlanetBase::DebugLogMessages(EMaterialTypeExo LayerLogType, FName LayerName, UMaterialInterface* Material)
{
	bool bIsValid = IsValid(Material);

	switch(LayerLogType)
	{
		case Generator :
			if(bIsValid)
			{
				if(!Material->GetMaterial()->HasEmissiveColorConnected())
				{
					FString LayerLog = this->GetName() + " Generator has nothing plugged into Emissive Color. As a result, the generator will do nothing.";
					UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
					GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
				}
			}
			else
			{
				FString LayerLog = this->GetName() + " has empty Generator. Falling back to 'Error' generator.";
				UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
				GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
			}
			break;
		case Layer :
			if(bIsValid)
			{
				if(!Material->GetMaterial()->HasEmissiveColorConnected())
				{
					FString LayerLog = this->GetName() + " Layer " + LayerName.ToString() + " has nothing plugged into Emissive Color. As a result, the layer will do nothing.";
					UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
					GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
				}
			}
			else
			{
				FString LayerLog = this->GetName() + " Layer " + LayerName.ToString() + " has empty Layer Generator. Falling back to 'Error' generator.";
				UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
				GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
			}
			break;
		case Render :
			if(bIsValid)
			{
				if(!Material->GetMaterial()->HasVertexPositionOffsetConnected())
				{
					FString LayerLog = this->GetName() + " Render Material has nothing plugged into World Position Offset. Please connect WPO from 'MakePlanetData'.";
					UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
					GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
				}
				if(Material->GetMaterialResource(ERHIFeatureLevel::SM6, EMaterialQualityLevel::High)->IsTangentSpaceNormal())
				{
					FString LayerLog = this->GetName() + " Render Material has 'Tangent Space Normal' enabled. Lighting will be broken until disabled.";
					UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
					GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
				}
			}
			else
			{
				FString LayerLog = this->GetName() + " Render Material is empty. Falling back to default Material.";
				UE_LOG(LogTemp, Warning, TEXT("%s"), *LayerLog);
				GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Red, LayerLog, true);
			}
			break;
		default :
			break;
	}
}

#endif

void AExoskyPlanetBase::Refresh()
{
	bInitialStart = true;
	bShouldStart = true;
}

void AExoskyPlanetBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
#if WITH_EDITOR
	if(IsCompiling())
	{
		//EmptyTree();
		return;
	}
#endif
	if(bShouldStart)
	{
		bShouldStart = false;
		EmptyTree();
		EmptyRTPool();
		EditorStartup();
	}
	else
	{
		Update();
	}
}

void AExoskyPlanetBase::BeginPlay()
{
	SetActorTickEnabled(false);
	bShouldStart = true;
	IsInPlay = true;
	SetActorTickEnabled(true);

	Super::BeginPlay();
}

void AExoskyPlanetBase::Destroyed()
{
	EmptyTree();
	EmptyRTPool();
	CollectGarbage(EObjectFlags::RF_NoFlags, true);
	Super::Destroyed();
}

void AExoskyPlanetBase::EditorStartup()
{
#if WITH_EDITOR
	FEditorDelegates::BeginPIE.AddUObject(this, &AExoskyPlanetBase::OnPie);
	FEditorDelegates::EndPIE.AddUObject(this, &AExoskyPlanetBase::EndPie);
#endif
	SetActorTickEnabled(false);

	FTimerHandle VelocityHandle;	
	FTimerDelegate VelocityDelegate;
	VelocityDelegate.BindUObject(this, &AExoskyPlanetBase::SmoothVelocity);

	GetWorld()->GetTimerManager().SetTimer(VelocityHandle, VelocityDelegate, 1./60., true);

	InitSectionScale();
	InitRadiusInUU();
	InitMaxSubdivisionLevel();
	InitFormats();
	InitCollisionMesh();
	bInitialStart = true;
	bCreateCollision = IsInPlay ? GenerateCollision : false;

	for(auto& Rotation : GetExoCubeFaces())
	{
		FPlanetNode NewNode;

		NewNode.Rotation = Rotation;
		NewNode.QID = 50;
		NewNode.NodeCenter = Rotation.RotateVector(TransformQuad(FVector(0.0, 0.0, 1.0)) * RadiusInUU);
		FVector LookupLocation = Rotation.RotateVector(FVector(0.0, 0.0, 1.0));

		NodesData.Add(LookupLocation, NewNode);
	}

	SetActorTickEnabled(true);
}

void AExoskyPlanetBase::Update()
{	
	PlayerLocation = GetPlayerLocation();
	TArray<FVector> NodeKeys;
	NodesData.GetKeys(NodeKeys);

	bool bUpdatedThisFrame = false;

	if(bInitialStart) //Reverse order; Generate closest to the camera on refresh/spawn.
	{
		for(int i = NodeKeys.Num(); i-->0;)
		{
			FVector Lookup = NodeKeys[i];
			if(NodesData.Contains(Lookup)) 
			{
				FPlanetNode ThisNode = NodesData[Lookup];

				if(QuadtreeCheck(Lookup)) { bUpdatedThisFrame = true; }

				if(NodesData.Contains(Lookup) && VelocityCheck(ThisNode.Scale, ThisNode.NodeCenter))
				{
					if(PoolAvailable() && ThisNode.NodeProcess == ENodeProcess::Empty)
					{
						CreateNodeCache(Lookup);
						//UE_LOG(LogTemp, Warning, TEXT("CREATING NODE CACHE"));
					}
					if(ThisNode.NodeProcess == ENodeProcess::TexturesFinished)
					{
						DispatchNodeReadback(Lookup);
						//UE_LOG(LogTemp, Warning, TEXT("CREATING NODE MESH"));
					}
				}
			}
		}
	}
	else
	{
		for(auto& NodeKey: NodeKeys)
		{
			if(NodesData.Contains(NodeKey)) 
			{
				FPlanetNode ThisNode = NodesData[NodeKey];
				
				if(QuadtreeCheck(NodeKey)) { bUpdatedThisFrame = true; }

				if(NodesData.Contains(NodeKey) && VelocityCheck(ThisNode.Scale, ThisNode.NodeCenter))
				{	
					if(PoolAvailable() && ThisNode.NodeProcess == ENodeProcess::Empty)
					{
						CreateNodeCache(NodeKey);
						//UE_LOG(LogTemp, Warning, TEXT("CREATING NODE CACHE"));
					}
					if(ThisNode.NodeProcess == ENodeProcess::TexturesFinished)
					{
						DispatchNodeReadback(NodeKey);
						//UE_LOG(LogTemp, Warning, TEXT("CREATING NODE MESH"));
					}
				}
			}
		}
	}

	if(!bUpdatedThisFrame && !(PooledTerrainTargets["Position"].Available.Num() == 0)) { bInitialStart = false; }
}

void AExoskyPlanetBase::InitSectionScale()
{
	switch(MeshResolution)
	{
		case EMeshResolution::x32:
			MeshRes = 32;
			break;
		case EMeshResolution::x64:
			MeshRes = 64;
			break;
		case EMeshResolution::x96:
			MeshRes = 96;
			break;
		case EMeshResolution::x128:
			MeshRes = 128;
			break;
		default:
			break;
	}
	SectionScale = ((1.0 / (float(MeshRes) - 2.0f)) * 4.0) + 2.0;

	PixelsPerFace = FMath::Clamp(PixelsPerFace, 2, 512 / MeshRes);

	GeneratorResolution = MeshRes * PixelsPerFace;

	for(int i = 0; i < LayersData.Num(); i++)
	{
		LayersData[i].PixelsPerFace = FMath::Clamp(LayersData[i].PixelsPerFace, 2, 512 / MeshRes);
	}
}

void AExoskyPlanetBase::InitRadiusInUU()
{
	RadiusInUU = GroundRadiusInKM * 100000.0f;
}

void AExoskyPlanetBase::InitMaxSubdivisionLevel()
{
	float SpacingCM = float(LODSettings.VertexSpacingInMeters) * 100.;
	MaxSubdivisionLevel = round(-((log(float(MeshRes) * ((2. * SpacingCM) / (PI * RadiusInUU))))/log(2))) - 1;
	FullPrecisionLevel = MaxSubdivisionLevel / 2 + 1;

	SpacingCM = FMath::Clamp(float(ResampleSettings.MaximumResolutionInCM), 0.0, float(LODSettings.VertexSpacingInMeters) * 100.);
	ResampleLevel = round(-((log(float(GeneratorResolution) * ((2. * SpacingCM) / (PI * RadiusInUU))))/log(2))) - 1;
}

void AExoskyPlanetBase::InitFormats()
{
	switch(HeightChannel)
	{
		case EHeightChannel::R:
			HeightChannelValue = FLinearColor(1.0f, 0.0f, 0.0f, 0.0f);
			break;
		case EHeightChannel::G:
			HeightChannelValue = FLinearColor(0.0f, 1.0f, 0.0f, 0.0f);
			break;
		case EHeightChannel::B:
			HeightChannelValue = FLinearColor(0.0f, 0.0f, 1.0f, 0.0f);
			break;
		default:
			break;
	}
	switch(GeneratorFormat)
	{
		case EGeneratorFormat::r32f:
			GeneratorFormatRT = RTF_R32f;
			break;
		case EGeneratorFormat::rg32f:
			GeneratorFormatRT = RTF_RG32f;
			break;
		case EGeneratorFormat::rgba32f:
			GeneratorFormatRT = RTF_RGBA32f;
			break;
		default:
			break;
	}
}

void AExoskyPlanetBase::InitCollisionMesh()
{
	UVs.Empty();

	int NumX = MeshRes + 1;

	FVector2D Extent = FVector2D((NumX - 1), (NumX - 1)) / 2;

	for (int i = 0; i < NumX; i++)
	{
		for (int j = 0; j < NumX; j++)
		{
			UVs.Add(FVector2f((float)j / ((float)NumX - 1), (float)i / ((float)NumX - 1)));
		}
	}
}

FVector AExoskyPlanetBase::TransformQuad(FVector Pos)
{
	return CubeToSphere(Pos);
}

void AExoskyPlanetBase::PoolRT(FName TextureName)
{
	if(!PooledTerrainTargets.Contains(TextureName))
	{
		PooledTerrainTargets.Add(TextureName);
	}
	PooledTerrainTargets[TextureName].NumPooled++;

	ETextureRenderTargetFormat OptimizedGeneratorFormat = OptimizeMemoryFormat(1000, true, GeneratorFormatRT);

	int Resolution = GeneratorResolution;
	ETextureRenderTargetFormat Format = RTF_RGBA32f;

	if(TextureName == "Generator") { 
		Format = GeneratorFormatRT; 
	}
	else if(TextureName == "Position") { 
		Format = RTF_RGBA32f; 
	}
	else if(TextureName == "Generator2") { 
		Format = OptimizedGeneratorFormat;
		Resolution /= 2;
	}
	else if(TextureName == "VertexNormal") {
		Format = RTF_RG16f;
		Resolution = MeshRes + 1;
	}
	else if(TextureName == "Normal") {
		Format = OptimizationSettings.UseHighQualityNormals ? RTF_RG16f : RTF_RG8;
	}

	PooledTerrainTargets[TextureName].Available.Add(
		UKismetRenderingLibrary::CreateRenderTarget2D(
			this,
			Resolution,
			Resolution,
			Format,
			FLinearColor::Black,
			false,
			true
		)
	);
}

void AExoskyPlanetBase::EmptyRTPool()
{
	bRTPoolCreated = false;
	
	for(auto& PooledTexture : PooledTerrainTargets)
	{
		for(auto& RT : PooledTexture.Value.Taken)
		{
			RT->ReleaseResource();
		}
		for(auto& RT : PooledTexture.Value.Available)
		{
			RT->ReleaseResource();
		}
	}
	PooledTerrainTargets.Empty();
}

bool AExoskyPlanetBase::PoolAvailable()
{
	if(!PooledTerrainTargets.Contains("Position"))
	{
		return true;
	}

	if(PooledTerrainTargets["Position"].Taken.Num() == PooledTerrainTargets["Position"].NumPooled &&
		LODSettings.ProcessesPerFrame == PooledTerrainTargets["Position"].NumPooled)
	{
		return false;
	}

	return true;
}

UTextureRenderTarget2D* AExoskyPlanetBase::GetFromAvailable(FName TextureName)
{
	UTextureRenderTarget2D* Texture = nullptr;

	if(!PooledTerrainTargets.Contains(TextureName))
	{
		PoolRT(TextureName);
	}
	else if (PooledTerrainTargets[TextureName].Available.Num() == 0)
	{
		PoolRT(TextureName);
	}

	Texture = PooledTerrainTargets[TextureName].Available[0];
	PooledTerrainTargets[TextureName].Taken.Add(Texture);
	PooledTerrainTargets[TextureName].Available.RemoveAt(0);

	return Texture;
}

void AExoskyPlanetBase::AddToAvailable(FName TextureName, UTextureRenderTarget2D* Texture)
{
	if(!IsValid(Texture) || !PooledTerrainTargets.Contains(TextureName)) {return;}

	int ID = PooledTerrainTargets[TextureName].Taken.Find(Texture);

	if(ID == -1) {return;}

	PooledTerrainTargets[TextureName].Available.Add(Texture);
	PooledTerrainTargets[TextureName].Taken.RemoveAt(ID);
}

#if WITH_EDITOR

bool AExoskyPlanetBase::ShouldRecompileForCompute(UMaterialInterface* Material)
{
	UMaterial* MaterialRef = Material->GetMaterial();
	bool bUsedWithGeometryCache = !MaterialRef->bUsedWithGeometryCache;
	bool bUnlit = !MaterialRef->GetShadingModels().HasShadingModel(EMaterialShadingModel::MSM_Unlit);
	bool bAllowNegativeEmissiveColor = !MaterialRef->bAllowNegativeEmissiveColor;

	if(bUsedWithGeometryCache || bUnlit)
	{
		FString Log = this->GetName() + "'s Generator Material" + Material->GetName() + " recompiling for generation.";

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Log);
		GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Yellow, Log, true);
		
		bool bNeedsRecompilation = true;
		Material->GetMaterial()->SetShadingModel(EMaterialShadingModel::MSM_Unlit);
		Material->GetMaterial()->bAllowNegativeEmissiveColor = true;
		Material->GetMaterial()->bUsedWithLidarPointCloud = false;
		Material->GetMaterial()->bUsedWithGeometryCache = true;
		Material->GetMaterial()->SetMaterialUsage(bNeedsRecompilation, MATUSAGE_GeometryCache);

		Material->GetMaterial()->ForceRecompileForRendering();

		return true;
	}
	return false;
}

bool AExoskyPlanetBase::ShouldRecompileForRendering(UMaterialInterface* Material)
{
	UMaterial* MaterialRef = Material->GetMaterial();
	bool bTangentSpaceNormal = MaterialRef->bTangentSpaceNormal;

	if(bTangentSpaceNormal)
	{
		FString Log = this->GetName() + "'s Render Material " + Material->GetName() + " recompiling for rendering.";

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Log);
		GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Yellow, Log, true);
		
		bool bNeedsRecompilation = true;
		Material->GetMaterial()->bTangentSpaceNormal = false;
		Material->GetMaterial()->ForceRecompileForRendering();

		return true;
	}
	return false;
}

bool AExoskyPlanetBase::IsMaterialCompiling(UMaterialInterface* InMaterial)
{
	bool bIsCompiling = false;
	if (FApp::CanEverRender())
	{
		const EMaterialQualityLevel::Type ActiveQualityLevel = GetCachedScalabilityCVars().MaterialQualityLevel;

		const ERHIFeatureLevel::Type FeatureLevel = ERHIFeatureLevel::Type::SM6;
		const EShaderPlatform ShaderPlatform = GShaderPlatformForFeatureLevel[FeatureLevel];

		FMaterialResource* CurrentResource = InMaterial->GetMaterialResource(FeatureLevel, ActiveQualityLevel);
		if (CurrentResource && !CurrentResource->IsCompilationFinished())
		{
			bIsCompiling = true;
		}
	}
	return bIsCompiling;
}

bool AExoskyPlanetBase::CheckGeneratorCompiling(UMaterialInterface* InMaterial)
{
	if(!IsValid(InMaterial)) {return false;}
	if(IsMaterialCompiling(InMaterial->GetMaterial()) || (OptimizationSettings.UseComputeBackend ? ShouldRecompileForCompute(InMaterial) : false)) 
	{
		UMaterial* ThisMaterial = InMaterial->GetMaterial();
		if(!CompilingMaterials.Contains(ThisMaterial))
		{
			FString Log = this->GetName() + "'s Generator Material " + InMaterial->GetName() + " is compiling. Terrain will re-appear when ready.";
			GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Cyan, *Log, true);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Log);

			CompilingMaterials.Add(ThisMaterial);
			bShouldStart = true;
		}
		return true;
	}
	return false;
}

bool AExoskyPlanetBase::CheckRenderCompiling(UMaterialInterface* InMaterial)
{
	if(!IsValid(InMaterial)) {return false;}
	if(IsMaterialCompiling(InMaterial->GetMaterial()) || ShouldRecompileForRendering(InMaterial)) 
	{
		UMaterial* ThisMaterial = InMaterial->GetMaterial();
		if(!CompilingMaterials.Contains(ThisMaterial))
		{
			FString Log = this->GetName() + "'s Render Material " + InMaterial->GetName() + " is compiling. Terrain will re-appear when ready.";
			GEngine->AddOnScreenDebugMessage(-1, 5., FColor::Cyan, *Log, true);
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Log);

			CompilingMaterials.Add(ThisMaterial);
			//bShouldStart = true;
		}
		return true;
	}
	return false;
}
	
void AExoskyPlanetBase::UpdateCompilingMaterials()
{
	TArray<UMaterial*> MaterialsToRemove;
	for(auto& ThisMaterial : CompilingMaterials)
	{
		if(!IsMaterialCompiling(ThisMaterial))
		{
			MaterialsToRemove.Add(ThisMaterial);
		}
	}
	for(auto& ThisMaterial : MaterialsToRemove)
	{
		CompilingMaterials.Remove(ThisMaterial);
	}
	MaterialsToRemove.Empty();
}

bool AExoskyPlanetBase::IsCompiling()
{
	CheckGeneratorCompiling(GeneratorMaterial);
	//CheckRenderCompiling(RenderMaterial);
	//CheckGeneratorCompiling(NormalComputeMat);
	//CheckGeneratorCompiling(PositionComputeMat);
	//CheckGeneratorCompiling(ErrorMat);
	//CheckGeneratorCompiling(CopyMat);

	for(auto& Layer : LayersData)
	{
		CheckGeneratorCompiling(Layer.LayerGenerator);
	}

	if(CompilingMaterials.Num() > 0)
	{
		UpdateCompilingMaterials();
		bShouldStart = true;
		return true;
	}
	return false;
}

#endif

UMaterialInterface* AExoskyPlanetBase::CheckGeneratorNull(UMaterialInterface* Material)
{
	if(Material == nullptr)
	{
		return ErrorMat;
	}
	return Material;
}

FVector AExoskyPlanetBase::GetPlayerLocation()
{
	if(IsInPlay)
	{
		if(IsValid(GetWorld()->GetFirstPlayerController()->GetPawn())) {return GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();}
		else { return PlayerLocation; }
	}
	else
	{
		return GetEditorViewLocation(PlayerLocation, GetWorld());
	}
}


void AExoskyPlanetBase::SmoothVelocity()
{
	FVector Velocity;

	if(IsInPlay)
	{
		if(IsValid(GetWorld()->GetFirstPlayerController()->GetPawn())) {Velocity = GetWorld()->GetFirstPlayerController()->GetPawn()->GetVelocity();}
	}
	else
	{
		Velocity = GetEditorViewLocation(PlayerLocation, this) - PlayerLocation;
	}

	if(SmoothedVelocity.Size() < Velocity.Size())
	{
		//Instantly update velocity if speeding up
		SmoothedVelocity = Velocity;
		
	}
	else
	{
		//Smooth velocity if slowing down
		SmoothedVelocity = (Velocity + SmoothedVelocity) / 2.0;
	}
}

bool AExoskyPlanetBase::VelocityCheck(float ScaleSection, FVector NodeCenter)
{
	FVector UpVector = (PlayerLocation - this->GetActorLocation() + NodeCenter);
	UpVector.Normalize();
	FVector VelocityDirection = SmoothedVelocity;
	VelocityDirection.Normalize();
	float AmountTowardsSurface = FMath::Clamp(FVector::DotProduct(UpVector, -VelocityDirection), 0., 1.);

	return (ScaleSection * RadiusInUU * LODSettings.SubdivisionDistanceMultiplier * 2.0) > (SmoothedVelocity.GetAbs().Size() / UGameplayStatics::GetGlobalTimeDilation(GetWorld())) * (1.0 - AmountTowardsSurface);
}

bool AExoskyPlanetBase::DistanceCheck(FVector NodeLocation)
{
	FPlanetNode ThisNode = NodesData[NodeLocation];
	FVector NodeCenter = ThisNode.NodeCenter;

	FVector PlanetLocation = this->GetActorLocation();
	FRotator PlanetRotation = this->GetActorRotation();
	FVector PlanetScale = this->GetActorScale();

	NodeCenter *= PlanetScale;
	NodeCenter = PlanetRotation.RotateVector(NodeCenter);
	NodeCenter = NodeCenter + PlanetLocation;

	return FVector::Distance(NodeCenter, PlayerLocation) < (ThisNode.Scale * RadiusInUU * LODSettings.SubdivisionDistanceMultiplier * 2.0 * 
		(FMath::Pow(2.0, -2 * (ThisNode.LOD-1)) + 1.0)); //A little bit of tomfoolery (I want farther lower level nodes to subdivide sooner)
}

bool AExoskyPlanetBase::QuadtreeCheck(FVector NodeLocation)
{
	FPlanetNode ThisNode = NodesData[NodeLocation];
	
	if((ThisNode.NodeProcess == ENodeProcess::Processed) && (ThisNode.PersistentCaches.Num() > 0))
	{
		//UE_LOG(LogTemp, Warning, TEXT("NODE IS INDEED PROCESSED"));
		bool NotValid = false;
		for(auto& Texture : ThisNode.PersistentCaches)
		{
			if(!IsValid(Texture.Value)) {NotValid = true;}
		}
		if(!NotValid) {NodesData[NodeLocation].NodeProcess = ENodeProcess::TexturesFinished;}
	}

	if(ThisNode.ChildrenBuilt)
	{
		ExoSky_PlanetMeshProvider::DisableMesh(ThisNode.MeshInfo);
	}
	else if(ThisNode.Children.Num() == 4)
	{
		int ChildrenPhysicsBuilt = 0;
		int ChildrenMeshBuilt = 0;

		for(auto& ChildLookup : ThisNode.Children)
		{
			FPlanetNode ChildNode = NodesData[ChildLookup];
			if(ExoSky_PlanetMeshProvider::HasPhysics(ChildNode.MeshInfo, bCreateCollision))	
			{
				ChildrenPhysicsBuilt++;
			}
			if(IsValid(ChildNode.MeshInfo.RenderMesh))
			{
				ChildrenMeshBuilt++;
			}
		}
		if(ChildrenPhysicsBuilt == 4) {NodesData[NodeLocation].ChildrenBuilt = true;}
		if(ChildrenMeshBuilt == 4) {ExoSky_PlanetMeshProvider::HideMesh(ThisNode.MeshInfo);}
	}

	if(!bInitialStart && (ThisNode.Children.Num() != 4))
	{ 
		ExoSky_PlanetMeshProvider::EnableMesh(ThisNode.MeshInfo, bCreateCollision, CollisionSettings); 
	}

	if (!(ThisNode.QID == 50) && !NodesData.Contains(ThisNode.ParentLocation))
	{
		DestroyNode(NodeLocation);
		return false;
	}

	if(DistanceCheck(NodeLocation) && (ThisNode.LOD <= MaxSubdivisionLevel))
	{
		if(ThisNode.Children.Num() == 0)
		{
			SplitNode(NodeLocation);
			return true;
		}
	}
	else
	{
		if(ThisNode.Children.Num() > 0)
		{
			UnSplitNode(NodeLocation);
			return false;
		}
	}
	return false;
}

//SPLIT / UNSPLIT
void AExoskyPlanetBase::SplitNode(FVector NodeLocation) 
{
	for(int i = 0; i < 4; i++)
	{
		CreateNode(NodeLocation, i);
	}
}

void AExoskyPlanetBase::UnSplitNode(FVector NodeLocation) 
{
	if(NodesData.Contains(NodeLocation))
	{
		FPlanetNode ThisNode = NodesData[NodeLocation];

		ExoSky_PlanetMeshProvider::EnableMesh(NodesData[NodeLocation].MeshInfo, bCreateCollision, CollisionSettings);
		
		for(int i = 0; i < ThisNode.Children.Num(); i++)
		{
			if(NodesData.Contains(ThisNode.Children[i]))
			{
				DestroyNode(ThisNode.Children[i]);
			}
		}
		NodesData[NodeLocation].Children.Empty();
		NodesData[NodeLocation].ChildrenBuilt = false;
	}
}

ETextureRenderTargetFormat AExoskyPlanetBase::OptimizeMemoryFormat(int LOD, bool OptimizeMemory, ETextureRenderTargetFormat Format)
{
	if (!OptimizeMemory || !OptimizeRTList.Contains(Format))
	{
		return Format;
	}
	if (LOD < FullPrecisionLevel)
	{
		switch (Format)
		{
		case ETextureRenderTargetFormat::RTF_R32f:
			return RTF_R16f;
		case ETextureRenderTargetFormat::RTF_RG32f:
			return RTF_RG16f;
		case ETextureRenderTargetFormat::RTF_RGBA32f:
			return RTF_RGBA16f;
		case ETextureRenderTargetFormat::RTF_R16f:
			return RTF_R8;
		case ETextureRenderTargetFormat::RTF_RG16f:
			return RTF_RG8;
		case ETextureRenderTargetFormat::RTF_RGBA16f:
			return RTF_RGBA8;
		default:
			return Format;
		}
	}
	else { return Format; }
}

FVector AExoskyPlanetBase::CreateNode(FVector ParentLocation, uint8 QID)
{
	FPlanetNode ParentNode = NodesData[ParentLocation];

	float Scale = ParentNode.Scale / 2.0f;
	float ScaleSection = Scale * SectionScale * RadiusInUU;

	FVector QuadLocation;
	QuadLocation = (QIDLookup[QID] * Scale) + ParentNode.QuadLocation;

	FPlanetNode CurrentNode;
	CurrentNode.QID = QID;
	CurrentNode.LOD = ParentNode.LOD + 1;
	CurrentNode.Scale = Scale;

	FVector TransformedQuad = TransformQuad(QuadLocation + FVector(0.0f, 0.0f, 1.0f));

	CurrentNode.NodeCenter = ParentNode.Rotation.RotateVector(TransformedQuad * RadiusInUU);
	CurrentNode.ParentLocation = ParentLocation;
	CurrentNode.QuadLocation = QuadLocation;
	CurrentNode.Rotation = ParentNode.Rotation;

	FVector LookupLocation = ParentNode.Rotation.RotateVector(QuadLocation + FVector(0.0f, 0.0f, 1.0));

	NodesData.Add(LookupLocation, CurrentNode);
	NodesData[ParentLocation].Children.Add(LookupLocation);

	return LookupLocation;
}

void AExoskyPlanetBase::DestroyNodeCache(FVector NodeLocation)
{
	if(!NodesData.Contains(NodeLocation)) {return;}

	FPlanetNode ThisNode = NodesData[NodeLocation];
	
	for (auto& Cache : ThisNode.TemporaryCaches)
	{
		if(IsValid(Cache.Value))
		{
			AddToAvailable(Cache.Key, Cache.Value);
		}
	}
	for (auto& Cache : ThisNode.PersistentCaches)
	{
		if(IsValid(Cache.Value))
		{
			NodesData[NodeLocation].PersistentCaches[Cache.Key]->ReleaseResource();
		}
	}
}

void AExoskyPlanetBase::DestroyNode(FVector NodeLocation) 
{ 
	if(NodesData.Contains(NodeLocation))
	{
		FPlanetNode ThisNode = NodesData[NodeLocation];
		ExoSky_PlanetMeshProvider::DestroyMesh(ThisNode.MeshInfo);

		DestroyNodeCache(NodeLocation);
		NodesData.Remove(NodeLocation);
	}
}

void AExoskyPlanetBase::EmptyTree()
{	
	for(auto& ThisNode : NodesData)
	{
		ExoSky_PlanetMeshProvider::DestroyMesh(ThisNode.Value.MeshInfo);
		DestroyNodeCache(ThisNode.Key);
	}
	NodesData.Empty();
#if USE_DMC
	TArray<UDynamicMeshComponent*> MeshComps;
	this->GetComponents<UDynamicMeshComponent>(MeshComps);
#elif USE_RMC
	TArray<URealtimeMeshComponent*> MeshComps;
	this->GetComponents<URealtimeMeshComponent>(MeshComps);
#elif USE_PMC
	TArray<UProceduralMeshComponent*> MeshComps;
	this->GetComponents<UProceduralMeshComponent>(MeshComps);
#endif
	for(auto& Comp : MeshComps)
	{
		Comp->DestroyComponent();
	}
}

void AExoskyPlanetBase::RenderToRT(UMaterialInterface* Material, UTextureRenderTarget2D* Texture)
{
	if(OptimizationSettings.UseComputeBackend)
	{
		int Size = Texture->SizeX;
		FRenderTargetComputeShaderDispatchParams Params(Size,Size,1);
		Params.RenderTarget2D = Texture;
		Params.MaterialRenderProxy = Material->GetRenderProxy();
		Params.Scene = this->GetRootComponent()->GetScene();

		FRenderTargetComputeShaderInterface::Dispatch(Params);
	}
	else
	{
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, Texture, Material);
	}
}

//NODE CACHE RELATED SHENANEGANS
void AExoskyPlanetBase::CreateNodeCache(FVector Lookup) 
{
	TMap<FName, UTextureRenderTarget2D*> TemporaryCaches;

	FPlanetNode ThisNode = NodesData[Lookup];
	
	UMaterialInstanceDynamic* SectionMaterial = SetupParameters(
		RenderMaterial,
		ThisNode,
		PixelsPerFace
	);

	UMaterialInstanceDynamic* NodeGenerator;
	NodeGenerator = SetupParameters(
		GeneratorMaterial,
		ThisNode,
		PixelsPerFace
	);

	UTextureRenderTarget2D* GeneratorTexture = GetFromAvailable("Generator");
	RenderToRT(NodeGenerator, GeneratorTexture);
	
	NodeGenerator->ConditionalBeginDestroy();

	if(ResampleSettings.ResampleGenerator && ThisNode.LOD > ResampleLevel)
	{
		UMaterialInstanceDynamic* ResampleGenerator;
		ResampleGenerator = SetupParameters(
			ResampleMat,
			ThisNode,
			PixelsPerFace
		);

		int Level = float(ThisNode.LOD - ResampleLevel);
		int ResampleResolution = FMath::Clamp(GeneratorResolution / FMath::Clamp(FMath::Pow(float(2), float(Level)), 1, 10000), MeshRes, 100000);
		ResampleGenerator->SetScalarParameterValue("Resolution", float(ResampleResolution));
		ResampleGenerator->SetScalarParameterValue("Level", float(Level));
		ResampleGenerator->SetTextureParameterValue("Texture", GeneratorTexture);

		RenderToRT(ResampleGenerator, GeneratorTexture);
	}
	
	TemporaryCaches.Add("Generator", GeneratorTexture);

	//DYNAMIC CACHE SYSTEM
	TArray<FName> PastLayers;
	for(auto& Layer : LayersData)
	{	
		if(PastLayers.Contains(Layer.Name)) {continue;}

		int LayerResolution = MeshRes * Layer.PixelsPerFace;

		UMaterialInstanceDynamic* LayerGenerator;
		LayerGenerator = SetupParameters(
			Layer.LayerGenerator,
			ThisNode,
			Layer.PixelsPerFace
		);

		for(auto& CacheElement : NodesData[Lookup].PersistentCaches)
		{
			LayerGenerator->SetTextureParameterValue(
				CacheElement.Key,
				CacheElement.Value
			);
		}

		LayerGenerator->SetTextureParameterValue (
			"Generator",
			GeneratorTexture
		);

		LayerGenerator->SetTextureParameterValue (
			"EXOSKY_Planets_GeneratorTexture",
			GeneratorTexture
		);
		
		if(Layer.OverwriteGenerator)
		{
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, GeneratorTexture, LayerGenerator);
		}
		else
		{
			UTextureRenderTarget2D* LayerTexture = UKismetRenderingLibrary::CreateRenderTarget2D(this, MeshRes * Layer.PixelsPerFace, MeshRes * Layer.PixelsPerFace, Layer.LayerFormat, FLinearColor::Black, false, false);
			NodesData[Lookup].PersistentCaches.Add(Layer.Name, LayerTexture);
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, LayerTexture, LayerGenerator);
			SectionMaterial->SetTextureParameterValue(Layer.Name, LayerTexture);
		}
		PastLayers.Add(Layer.Name);
		LayerGenerator->ConditionalBeginDestroy();
	}
	PastLayers.Empty();
	// Position Compute
	UMaterialInstanceDynamic* NodePositionMaterial = SetupParameters(
		PositionComputeMat,
		ThisNode,
		PixelsPerFace
	);

	NodePositionMaterial->SetTextureParameterValue(
		"EXOSKY_Planets_GeneratorTexture",
		GeneratorTexture
	);

	UTextureRenderTarget2D* PositionTexture = GetFromAvailable("Position");
	TemporaryCaches.Add("Position", PositionTexture);

	RenderToRT(NodePositionMaterial, PositionTexture);

	NodePositionMaterial->ConditionalBeginDestroy();

	// Normal Compute
	UMaterialInstanceDynamic* NodeNormalMaterial = SetupParameters(
		NormalComputeMat,
		ThisNode,
		PixelsPerFace
	);

	NodeNormalMaterial->SetTextureParameterValue(
		"EXOSKY_Planets_PositionCache",
		PositionTexture
	);

	NodeNormalMaterial->SetTextureParameterValue(
		"EXOSKY_Planets_GeneratorTexture",
		GeneratorTexture
	);

	UTextureRenderTarget2D* NormalTexture = GetFromAvailable("Normal");
	RenderToRT(NodeNormalMaterial, NormalTexture);
	TemporaryCaches.Add("Normal", NormalTexture);
	AsyncCreateRenderTarget2D("EXOSKY_Planets_NormalTexture", NormalTexture, Lookup);

	// UTextureRenderTarget2D* NormalTexture =  UKismetRenderingLibrary::CreateRenderTarget2D(
	// 	this,
	// 	GeneratorResolution, 
	// 	GeneratorResolution, 
	// 	OptimizationSettings.UseHighQualityNormals ? RTF_RG16f : RTF_RG8, 
	// 	FLinearColor::Black, false, true);

	// RenderToRT(NodeNormalMaterial, NormalTexture);
	// SectionMaterial->SetTextureParameterValue("EXOSKY_Planets_NormalTexture", NormalTexture);

	//NodesData[Lookup].PersistentCaches.Add("Normal", NormalTexture);

	// Vertex Normal Compute
	NodeNormalMaterial->SetScalarParameterValue("UseSpecifiedResolution", 1.0);
	NodeNormalMaterial->SetScalarParameterValue("Resolution", float(MeshRes + 1));

	UTextureRenderTarget2D* VertexNormalTexture = GetFromAvailable("VertexNormal");
	TemporaryCaches.Add("VertexNormal", VertexNormalTexture);

	RenderToRT(NodeNormalMaterial, VertexNormalTexture);
	NodeNormalMaterial->ConditionalBeginDestroy();

	// Redraw to lower precision generator.
	
	if(!OptimizationSettings.ClearGeneratorMemory)
	{
		UMaterialInstanceDynamic* GeneratorCopyMaterial = SetupParameters(
			CopyMat,
			ThisNode,
			PixelsPerFace
		);

		GeneratorCopyMaterial->SetTextureParameterValue(
			"Texture",
			GeneratorTexture
		);

		ETextureRenderTargetFormat OptimizedGeneratorFormatRT = OptimizeMemoryFormat(10000, true, GeneratorFormatRT);

		// UTextureRenderTarget2D* GeneratorTexture2 = GetFromAvailable("Generator2");
		// TemporaryCaches.Add("Generator2", GeneratorTexture2);
		// AsyncCreateRenderTarget2D("EXOSKY_Planets_GeneratorTexture", GeneratorTexture2, Lookup);
		// UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, GeneratorTexture2, GeneratorCopyMaterial);

		UTextureRenderTarget2D* GeneratorTexture2 =  UKismetRenderingLibrary::CreateRenderTarget2D(
		this,
		GeneratorResolution / 2, 
		GeneratorResolution / 2, 
		OptimizedGeneratorFormatRT,
		FLinearColor::Black, false, true);

		RenderToRT(GeneratorCopyMaterial, GeneratorTexture2);

		NodesData[Lookup].PersistentCaches.Add("Generator2", GeneratorTexture2);

		GeneratorCopyMaterial->ConditionalBeginDestroy();
		SectionMaterial->SetTextureParameterValue("EXOSKY_Planets_GeneratorTexture", GeneratorTexture2);
	}

	SectionMaterial->SetScalarParameterValue("EXOSKY_Planets_IsInRenderPass", 1.0f);

	NodesData[Lookup].RenderMaterial = SectionMaterial;
	NodesData[Lookup].TemporaryCaches = TemporaryCaches;
	NodesData[Lookup].NodeProcess = ENodeProcess::Processed;

	//UE_LOG(LogTemp, Warning, TEXT("NODE REACHED END"));
}


void AExoskyPlanetBase::AsyncCreateRenderTarget2D(FName TextureName, UTextureRenderTarget2D* RenderTarget, FVector Lookup)
{
	const int32 TextureSizeX = RenderTarget->SizeX;
	const int32 TextureSizeY = RenderTarget->SizeY;
	EPixelFormat PixelFormat = RenderTarget->GetFormat();
	const int32 NumMips = 1;

	UTextureRenderTarget2D* NewTexture =  UKismetRenderingLibrary::CreateRenderTarget2D(this, TextureSizeX, TextureSizeY, RenderTarget->RenderTargetFormat, FLinearColor::Black, false, true);
	FRenderTargetCopyDispatchParams Params;
	Params.SourceTexture = RenderTarget;
	Params.DestTexture = NewTexture;
	FRenderTargetCopyInterface::DispatchGameThread(Params);

	NodesData[Lookup].PersistentCaches.Add(TextureName, NewTexture);

	// UTexture2D* NewTexture = NewObject<UTexture2D>(
	// 		GetTransientPackage(),
	// 		MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), "EE"),
	// 		RF_Transient
	// 	);

	// NewTexture->NeverStream = true;
	// //Initialize Resource
	// {
	// 	FTexturePlatformData* PlatformData = new FTexturePlatformData;
	// 	PlatformData->SizeX = 1;
	// 	PlatformData->SizeY = 1;
	// 	PlatformData->PixelFormat = PixelFormat;

	// 	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	// 	Mip->SizeX = 1;
	// 	Mip->SizeY = 1;

	// 	PlatformData->Mips.Add(Mip);
	// 	NewTexture->SetPlatformData(PlatformData);
	// 	NewTexture->
		

	// 	int MipBytes = Mip->SizeX * Mip->SizeY * GPixelFormats[PixelFormat].BlockBytes;
	// 	Mip->BulkData.Lock(LOCK_READ_WRITE);

	// 	void* TextureData = Mip->BulkData.Realloc(MipBytes);

	// 	static TArray<uint8> DummyBytes;
	// 	DummyBytes.SetNum(MipBytes);

	// 	FMemory::Memcpy(TextureData, DummyBytes.GetData(), MipBytes);

	// 	Mip->BulkData.Unlock();

	// 	NewTexture->UpdateResource();
	// }

	// //UE_LOG(LogTemp, Warning, TEXT("Texture Async Called"));
	// Async(EAsyncExecution::Thread,[RenderTarget, NewTexture, TextureName, Lookup, this]()
    // {
	// 	if(!NodesData.Contains(Lookup)) {return;}
    //     FTextureRHIRef TextureResource = RHICreateTexture(FRHITextureCreateDesc::Create2D(TEXT(""),RenderTarget->SizeX,RenderTarget->SizeY,RenderTarget->GetFormat()));

	// 	//UE_LOG(LogTemp, Warning, TEXT("Texture Async Started"));
    //     ENQUEUE_RENDER_COMMAND(CopyRTToTexture)([RenderTarget, TextureName, TextureResource, NewTexture, this, Lookup](FRHICommandListImmediate& RHICmdList)
    //     {
	// 		if(!NodesData.Contains(Lookup)) {return;}

	//	    const FRenderTarget* RenderTargetResource = RenderTarget->GetRenderTargetResource();
	//	    RHICmdList.CopyTexture(RenderTargetResource->GetRenderTargetTexture(), TextureResource, FRHICopyTextureInfo());
	//	    RHIUpdateTextureReference(NewTexture->TextureReference.TextureReferenceRHI, TextureResource);
	//	    NewTexture->RefreshSamplerStates();
	// 		//UE_LOG(LogTemp, Warning, TEXT("Texture Finished"));

	// 		NodesData[Lookup].PersistentCaches.Add(Name, NewTexture);

    //     });
    // });
}

UMaterialInstanceDynamic* AExoskyPlanetBase::SetupParameters(UMaterialInterface* RefMaterial, FPlanetNode ThisNode, int PIXELSPERFACE )
{
	RefMaterial = CheckGeneratorNull(RefMaterial);
	UMaterialInstanceDynamic* ComputeMaterial = UMaterialInstanceDynamic::Create(RefMaterial, this);

	FVector RebaseLocation = FVector(FIntVector((TransformQuad(ThisNode.QuadLocation + FVector(0.0f, 0.0f, 1.0f)) * RadiusInUU) / 100000.0) * 100000.0);

	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_NodeScale", ThisNode.Scale * SectionScale);
	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_LOD", ThisNode.LOD);
	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_TerrainHeight", TerrainHeightMultiplier);
	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_Radius", RadiusInUU);
	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_IsPreview", 0.0f);
	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_PPF", PIXELSPERFACE);
	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_MeshResolution", float(MeshRes));
	ComputeMaterial->SetScalarParameterValue("EXOSKY_Planets_UseDMC", USE_DMC);

	ComputeMaterial->SetVectorParameterValue("EXOSKY_Planets_RebaseLocation", FLinearColor(RebaseLocation));
	ComputeMaterial->SetDoubleVectorParameterValue("EXOSKY_Planets_RebaseLocationDouble", FLinearColor(ThisNode.Rotation.RotateVector(RebaseLocation)));
	ComputeMaterial->SetVectorParameterValue("EXOSKY_Planets_Location", ThisNode.QuadLocation);
	ComputeMaterial->SetVectorParameterValue("EXOSKY_Planets_Angle", FLinearColor(ThisNode.Rotation.Roll / 360.0f * PI * 2.0f, ThisNode.Rotation.Pitch / 360.0f * PI * 2.0f, 0.0f, 0.0f));
	ComputeMaterial->SetVectorParameterValue("EXOSKY_Planets_HeightChannel", HeightChannelValue);

	return ComputeMaterial;
}

void AExoskyPlanetBase::DispatchNodeReadback(FVector Lookup)
{
	FPlanetNode ThisNode = NodesData[Lookup];

	for(auto& PersistentTexture : ThisNode.PersistentCaches)
	{
		NodesData[Lookup].RenderMaterial->SetTextureParameterValue (
			PersistentTexture.Key,
			PersistentTexture.Value
		);
	}

	NodesData[Lookup].NodeProcess = ENodeProcess::MeshProcessing;

	FAverageComputeShaderDispatchParams AverageParams(1, 1, 1);
	AverageParams.HeightChannel = HeightChannelValue;
	AverageParams.World = this;

	UTextureRenderTarget2D* Height = ThisNode.TemporaryCaches["Generator"];

	FAverageComputeShaderInterface::DispatchGameThread(AverageParams, Height, [this, ThisNode, Lookup](float OutputVal)
	{
		if(NodesData.Contains(Lookup))
		{
			NodesData[Lookup].NodeCenter = ThisNode.NodeCenter + (ThisNode.NodeCenter / RadiusInUU * OutputVal * TerrainHeightMultiplier);
			if(ThisNode.TemporaryCaches.Contains("Position"))
			{
				UTextureRenderTarget2D* Position = ThisNode.TemporaryCaches["Position"];
				UTextureRenderTarget2D* Normal = ThisNode.TemporaryCaches["Normal"];

				FVector RebaseLocation = FVector(FIntVector((TransformQuad(ThisNode.QuadLocation + FVector(0.0f, 0.0f, 1.0f)) * RadiusInUU) / 100000.0) * 100000.0);
				FVector3f RebaseLocation3f = FVector3f(ThisNode.Rotation.RotateVector(RebaseLocation));
				FVertexComputeShaderDispatchParams VertexParams(MeshRes + 1, MeshRes + 1, 1);
				VertexParams.Width = MeshRes + 1;
				VertexParams.RebasePosition = RebaseLocation3f;
				VertexParams.PlanetRadius = RadiusInUU;
				VertexParams.World = this;

				FVertexComputeShaderInterface::DispatchGameThread(VertexParams, Position, Normal, [this, Lookup, ThisNode, RebaseLocation3f](TArray<FVector3f> Vertices, TArray<FVector4f> Normals)
				{
					if(NodesData.Contains(Lookup) && IsValid(NodesData[Lookup].RenderMaterial))
					{
						FMeshAttributes MeshInfo = ExoSky_PlanetMeshProvider::CreateMesh(this, this->RootComponent, RenderingSettings, Vertices, Normals, UVs, bCreateCollision, ThisNode.RenderMaterial, WireframeMat, MeshRes);
						auto* ThisMesh = MeshInfo.RenderMesh;

						MeshInfo.RenderMesh->SetCollisionEnabled(CollisionSettings.GetCollisionEnabled(false));
						MeshInfo.RenderMesh->SetCollisionObjectType(CollisionSettings.GetObjectType());
						MeshInfo.RenderMesh->SetCollisionResponseToChannels(CollisionSettings.GetResponseToChannels());
						MeshInfo.RenderMesh->SetCollisionProfileName(CollisionSettings.GetCollisionProfileName());

						NodesData[Lookup].RenderMaterial->SetVectorParameterValue("EXOSKY_Planets_NodeRelative", ThisNode.NodeCenter);

						ThisMesh->SetRelativeLocation(FVector(RebaseLocation3f));

						NodesData[Lookup].MeshInfo = MeshInfo;

						if(ThisNode.ChildrenBuilt || (bInitialStart && ThisNode.Children.Num() > 0)) //If doing reverse order quadtree search, always hide the mesh if it has children.
						{
							ExoSky_PlanetMeshProvider::DisableMesh(MeshInfo);
						}

						for(auto& Texture : ThisNode.TemporaryCaches)
						{
							AddToAvailable(Texture.Key, Texture.Value);
							NodesData[Lookup].TemporaryCaches.Remove(Texture.Key);
						}
					}
				});
			}
		}
	});
}