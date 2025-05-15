/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetStringLibrary.h" 
#include "Kismet/KismetMathLibrary.h"
#include "CoreFwd.h" 
#include "ExoSky_PlanetFunctionsLibrary.h"

#if USE_DMC

#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h" 
#include "DynamicMesh/MeshAttributeUtil.h"

#elif USE_RMC

#include "RealtimeMeshLibrary.h"
#include "RealtimeMeshComponent.h"
#include "RealtimeMeshCore.h"
#include "RealtimeMeshSimple.h"

#elif USE_PMC

#include "ProceduralMeshComponent.h"

#endif
#include "ExoSky_PlanetMeshProvider.generated.h"

USTRUCT(BlueprintType)
struct FMeshAttributes
{
	GENERATED_BODY()
#if USE_DMC
		UDynamicMeshComponent* RenderMesh = nullptr;
		UMaterialInterface* RenderMaterial = nullptr;
#elif USE_RMC
		URealtimeMeshComponent* RenderMesh = nullptr;
		URealtimeMeshSimple* SimpleMesh = nullptr;
		FRealtimeMeshSectionGroupKey GroupKey;
		UMaterialInterface* RenderMaterial = nullptr;
#elif USE_PMC
		UProceduralMeshComponent* RenderMesh = nullptr;
		UMaterialInterface* RenderMaterial = nullptr;
#endif
};

class EXOSKYPLANETSPLUGIN_API ExoSky_PlanetMeshProvider
{
public:
	ExoSky_PlanetMeshProvider();
	~ExoSky_PlanetMeshProvider();

	static bool HasPhysics(FMeshAttributes MeshInfo, bool bCreateCollision);
	static bool IsMeshValid(FMeshAttributes MeshInfo);

	static bool DestroyMesh(FMeshAttributes MeshInfo);
	static bool DisableMesh(FMeshAttributes MeshInfo);
	static bool HideMesh(FMeshAttributes MeshInfo);
	static bool EnableMesh(FMeshAttributes MeshInfo, bool bCreateCollision, FBodyInstance CollisionSettings);
	static FMeshAttributes CreateMesh(UObject* Outer, USceneComponent* RootComponent, FRenderingSettings RenderingSettings, TArray<FVector3f> Vertices, TArray<FVector4f> Normals, TArray<FVector2f> UVs, bool bCreateCollision, UMaterialInterface* RenderMaterial, UMaterialInterface* WireframeMaterial, int MeshRes);
};