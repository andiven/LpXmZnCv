/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "ExoSky_PlanetMeshProvider.h"

ExoSky_PlanetMeshProvider::ExoSky_PlanetMeshProvider()
{
}

ExoSky_PlanetMeshProvider::~ExoSky_PlanetMeshProvider()
{
}

bool ExoSky_PlanetMeshProvider::IsMeshValid(FMeshAttributes MeshInfo)
{
    return IsValid(MeshInfo.RenderMesh);
}

bool ExoSky_PlanetMeshProvider::HideMesh(FMeshAttributes MeshInfo)
{
    if(IsMeshValid(MeshInfo))
    {
        MeshInfo.RenderMesh->SetVisibility(false);

        return true;
    }
    return false;
}

bool ExoSky_PlanetMeshProvider::DisableMesh(FMeshAttributes MeshInfo)
{
    if(IsMeshValid(MeshInfo))
    {
        MeshInfo.RenderMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        MeshInfo.RenderMesh->SetVisibility(false);
	    //ExoSky_PlanetMeshProvider::DestroyMesh(ThisNode.MeshInfo)

        return true;
    }
    return false;
}

bool ExoSky_PlanetMeshProvider::EnableMesh(FMeshAttributes MeshInfo, bool bCreateCollision, FBodyInstance CollisionSettings)
{
    if(IsMeshValid(MeshInfo))
    {
        if(bCreateCollision) {
            MeshInfo.RenderMesh->SetCollisionEnabled(CollisionSettings.GetCollisionEnabled(false));
            MeshInfo.RenderMesh->SetCollisionProfileName(CollisionSettings.GetCollisionProfileName());
            MeshInfo.RenderMesh->SetCollisionObjectType(CollisionSettings.GetObjectType());
            MeshInfo.RenderMesh->SetCollisionResponseToChannels(CollisionSettings.GetResponseToChannels());
        }
        MeshInfo.RenderMesh->SetVisibility(true);
        return true;
    }
    return false;
}

#if USE_DMC

//DYNAMIC MESH COMPONENT
bool ExoSky_PlanetMeshProvider::HasPhysics(FMeshAttributes MeshInfo, bool bCreateCollision)
{
    if(!bCreateCollision) { return IsMeshValid(MeshInfo); }
    return IsMeshValid(MeshInfo) && MeshInfo.RenderMesh->ContainsPhysicsTriMeshData(false);
}

//DYNAMIC MESH COMPONENT
bool ExoSky_PlanetMeshProvider::DestroyMesh(FMeshAttributes MeshInfo)
{
    if(IsMeshValid(MeshInfo))
    {
        MeshInfo.RenderMesh->SetMaterial(0, MeshInfo.RenderMaterial->GetMaterial());
        MeshInfo.RenderMesh->DestroyComponent();
        MeshInfo.RenderMaterial->ConditionalBeginDestroy();
        return true;
    }
    return false;
}

//DYNAMIC MESH COMPONENT
FMeshAttributes ExoSky_PlanetMeshProvider::CreateMesh(UObject* Outer, USceneComponent* RootComponent, FRenderingSettings RenderingSettings, TArray<FVector3f> Vertices, TArray<FVector4f> Normals, TArray<FVector2f> UVs, bool bCreateCollision, UMaterialInterface* RenderMaterial, UMaterialInterface* WireframeMaterial, int MeshRes)
{
    UDynamicMeshComponent* RenderMesh = NewObject<UDynamicMeshComponent>(Outer, MakeUniqueObjectName(Outer, UDynamicMeshComponent::StaticClass(), "DynamicMesh"), RF_Transient);				
    RenderMesh->RegisterComponent();
    RenderMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
    RenderMesh->LightingChannels = RenderingSettings.LightingChannels;
    RenderMesh->bCastStaticShadow = false;
    RenderMesh->bAffectDynamicIndirectLighting = false;
    RenderMesh->CastShadow = RenderingSettings.CastShadow;
    RenderMesh->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;

    RenderMesh->bUseAsyncCooking = true;
    RenderMesh->bComputeFastLocalBounds = true;
    RenderMesh->bCastShadowAsTwoSided = false;
    RenderMesh->SetTangentsType(EDynamicMeshComponentTangentsMode::NoTangents);

    FDynamicMesh3* SimpleMesh = RenderMesh->GetMesh();
    SimpleMesh->EnableVertexNormals(FVector3f(0.0,0.0,1.));
    SimpleMesh->EnableVertexUVs(FVector2f(0.0,0.0));
    SimpleMesh->EnableAttributes();

    for(int i = 0; i < UVs.Num(); i++)
    {
        FVector4f NormalRef = Normals[i];
        FVector3f Normal = FVector3f(NormalRef.X,NormalRef.Y, NormalRef.Z);
        SimpleMesh->AppendVertex(FVector3d(Vertices[i]));
        SimpleMesh->SetVertexNormal(i, Normal);
        SimpleMesh->SetVertexUV(i, UVs[i]);
        SimpleMesh->Attributes()->PrimaryUV()->AppendElement(UVs[i]);
    }

    for(int i = 0; i < UVs.Num(); i++)
    {
        if(UVs[i].Y == 1.0) { break; }
        if(UVs[i].X == 1.0) { continue; }

        if(int(Normals[i].W) != 1)
        {
            SimpleMesh->AppendTriangle(
                i,
                i + MeshRes + 1,
                i + MeshRes + 2
                );
            SimpleMesh->AppendTriangle(
                i,
                i + MeshRes + 1,
                i + MeshRes + 2
            );

            SimpleMesh->AppendTriangle(
                i,
                i + MeshRes + 2,
                i + 1
            );
        }
        else
        {
            SimpleMesh->AppendTriangle(
                i,
                i + MeshRes + 1,
                i + 1
            );

            SimpleMesh->AppendTriangle(
                i + 1,
                i + MeshRes + 1,
                i + MeshRes + 2
            );
        }
    }
    Vertices.Empty();
    Normals.Empty();
    if(bCreateCollision)
    {
        RenderMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        RenderMesh->CollisionType = ECollisionTraceFlag::CTF_UseComplexAsSimple;
        RenderMesh->bDeferCollisionUpdates = true;
        RenderMesh->SetComplexAsSimpleCollisionEnabled(true, false);
        RenderMesh->UpdateCollision();
    }
    UE::Geometry::CopyVertexUVsToOverlay(*SimpleMesh, *SimpleMesh->Attributes()->PrimaryUV());

    RenderMesh->NotifyMeshUpdated();

    RenderMesh->SetMaterial(0, RenderMaterial);
    RenderMesh->SetDefaultWireframeMaterial(WireframeMaterial);

    FMeshAttributes MeshAttributes;
    MeshAttributes.RenderMesh = RenderMesh;
    MeshAttributes.RenderMaterial = RenderMaterial;

    return MeshAttributes;
}

#elif USE_RMC

//REALTIME MESH COMPONENT
bool ExoSky_PlanetMeshProvider::HasPhysics(FMeshAttributes MeshInfo, bool bCreateCollision)
{
    if(!bCreateCollision) { return IsMeshValid(MeshInfo); }
    return IsMeshValid(MeshInfo) && MeshInfo.SimpleMesh->ContainsPhysicsTriMeshData(false);
}

//REALTIME MESH COMPONENT
bool ExoSky_PlanetMeshProvider::DestroyMesh(FMeshAttributes MeshInfo)
{
    if(IsMeshValid(MeshInfo))
    {
        MeshInfo.RenderMesh->SetMaterial(0, MeshInfo.RenderMaterial->GetMaterial());
        MeshInfo.SimpleMesh->RemoveSectionGroup(MeshInfo.GroupKey);
        MeshInfo.RenderMesh->DestroyComponent();
        MeshInfo.RenderMaterial->ConditionalBeginDestroy();
        return true;
    }
    return false;
}

//REALTIME MESH COMPONENT
FMeshAttributes ExoSky_PlanetMeshProvider::CreateMesh(UObject* Outer, USceneComponent* RootComponent, FRenderingSettings RenderingSettings, TArray<FVector3f> Vertices, TArray<FVector4f> Normals, TArray<FVector2f> UVs, bool bCreateCollision, UMaterialInterface* RenderMaterial, UMaterialInterface* WireframeMaterial, int MeshRes)
{
    URealtimeMeshComponent* RenderMesh = NewObject<URealtimeMeshComponent>(Outer, MakeUniqueObjectName(Outer, URealtimeMeshComponent::StaticClass(), "RealtimeMesh"), RF_Transient);				
    RenderMesh->RegisterComponent();
    RenderMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
    RenderMesh->LightingChannels = RenderingSettings.LightingChannels;
    RenderMesh->bCastStaticShadow = false;
    RenderMesh->bAffectDynamicIndirectLighting = false;
    RenderMesh->CastShadow = RenderingSettings.CastShadow;
    RenderMesh->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;

    //LINE 57 RUNTIMEMESHCOMPONENT.CPP ADD THE RF_TRANSIENT TAG TO THE NEWOBJECT REALTIMEMESH
    URealtimeMeshSimple* SimpleMesh = RenderMesh->InitializeRealtimeMesh<URealtimeMeshSimple>();
    
    FRealtimeMeshStreamSet StreamSet = FRealtimeMeshStreamSet();
    TRealtimeMeshBuilderLocal<uint32, FPackedNormal, FVector2DHalf, 1> Builder(StreamSet);

    Builder.EnableTexCoords();
    Builder.EnableTangents();
    Builder.EnablePolyGroups();
    Builder.EnableColors();

    for(int32 i = 0; i < UVs.Num(); i++)
    {
        FVector4f NormalRef = Normals[i];
        FVector3f Normal = FVector3f(NormalRef.X,NormalRef.Y, NormalRef.Z);
        Builder.AddVertex(Vertices[i]).SetTexCoord(UVs[i]).SetNormal(Normal).SetColor(FColor::White);
    }

    for(int32 i = 0; i < UVs.Num(); i++)
    {
        if(UVs[i].Y == 1.0) { break; }
        if(UVs[i].X == 1.0) { continue; }

        if(int(Normals[i].W) != 1)
        {
            Builder.AddTriangle(
                i,
                i + MeshRes + 1,
                i + MeshRes + 2
            );

            Builder.AddTriangle(
                i,
                i + MeshRes + 2,
                i + 1
            );
        }
        else
        {
            Builder.AddTriangle(
                i,
                i + MeshRes + 1,
                i + 1
            );

            Builder.AddTriangle(
                i + 1,
                i + MeshRes + 1,
                i + MeshRes + 2
            );
        }
    }
    
    Vertices.Empty();
    Normals.Empty();

    if(bCreateCollision)
    {
        RenderMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FRealtimeMeshCollisionConfiguration CollisionSettings;
        CollisionSettings.bUseAsyncCook = true;
        CollisionSettings.bShouldFastCookMeshes = true;
        CollisionSettings.bUseComplexAsSimpleCollision = true;

        SimpleMesh->SetCollisionConfig(CollisionSettings);
    }

    FRealtimeMeshSectionGroupKey SectionGroupKey = FRealtimeMeshSectionGroupKey::CreateUnique(0);
    FRealtimeMeshSectionKey Key = FRealtimeMeshSectionKey::CreateForPolyGroup(SectionGroupKey, 0);

    SimpleMesh->CreateSectionGroup(SectionGroupKey, StreamSet);
    SimpleMesh->UpdateSectionConfig(Key, FRealtimeMeshSectionConfig(ERealtimeMeshSectionDrawType::Static, 0), bCreateCollision);

    SimpleMesh->SetupMaterialSlot(0, "PrimaryMaterial", RenderMaterial);

    FMeshAttributes MeshAttributes;
    MeshAttributes.RenderMesh = RenderMesh;
    MeshAttributes.SimpleMesh = SimpleMesh;
    MeshAttributes.GroupKey = SectionGroupKey;
    MeshAttributes.RenderMaterial = RenderMaterial;

    return MeshAttributes;
}

#elif USE_PMC
//REALTIME MESH COMPONENT
bool ExoSky_PlanetMeshProvider::HasPhysics(FMeshAttributes MeshInfo, bool bCreateCollision)
{
    if(!bCreateCollision) { return IsMeshValid(MeshInfo); }
    return IsMeshValid(MeshInfo) && MeshInfo.RenderMesh->ContainsPhysicsTriMeshData(false);
}

//PROCEDURAL MESH COMPONENT
bool ExoSky_PlanetMeshProvider::DestroyMesh(FMeshAttributes MeshInfo)
{
    if(IsMeshValid(MeshInfo))
    {
        MeshInfo.RenderMesh->SetMaterial(0, MeshInfo.RenderMaterial->GetMaterial());
        MeshInfo.RenderMesh->ClearMeshSection(0);
        MeshInfo.RenderMesh->DestroyComponent();
        MeshInfo.RenderMaterial->ConditionalBeginDestroy();
        return true;
    }
    return false;
}

//PROCEDURAL MESH COMPONENT
FMeshAttributes ExoSky_PlanetMeshProvider::CreateMesh(UObject* Outer, USceneComponent* RootComponent, FRenderingSettings RenderingSettings, TArray<FVector3f> Vertices, TArray<FVector4f> Normals, TArray<FVector2f> UVs, bool bCreateCollision, UMaterialInterface* RenderMaterial, UMaterialInterface* WireframeMaterial, int MeshRes)
{
    UProceduralMeshComponent* RenderMesh = NewObject<UProceduralMeshComponent>(Outer, MakeUniqueObjectName(Outer, UProceduralMeshComponent::StaticClass(), "ProceduralMesh"), RF_Transient);				
    RenderMesh->RegisterComponent();
    RenderMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
    RenderMesh->LightingChannels = RenderingSettings.LightingChannels;
    RenderMesh->bCastStaticShadow = false;
    RenderMesh->bAffectDynamicIndirectLighting = false;
    RenderMesh->CastShadow = RenderingSettings.CastShadow;
    RenderMesh->ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Rigid;

    RenderMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

    TArray<FVector> BuilderVertices;
    TArray<FVector> BuilderNormals;
    TArray<FVector2D> BuilderUVs;
    TArray<FColor> BuilderColors;
    TArray<FProcMeshTangent> BuilderTangents;
    BuilderVertices.SetNum(Vertices.Num());
    BuilderNormals.SetNum(Normals.Num());
    BuilderUVs.SetNum(UVs.Num());
    BuilderColors.SetNum(UVs.Num());
    BuilderTangents.SetNum(UVs.Num());

    for(int32 i = 0; i < UVs.Num(); i++)
    {
        FVector4f Normal = Normals[i];
        FVector2f UV = UVs[i];
        BuilderNormals[i] = FVector(Normal.X, Normal.Y, Normal.Z);
        BuilderVertices[i] = FVector(Vertices[i]);
        BuilderUVs[i] = FVector2D(UV.X, UV.Y);
        BuilderColors[i] = FColor::White;
        BuilderTangents[i] = FProcMeshTangent(FVector(0.0, 0.0, 1.0), true);
    }

    TArray<int32> Triangles;
    for(int32 i = 0; i < UVs.Num(); i++)
    {
        if(UVs[i].Y == 1.0) { break; }
        if(UVs[i].X == 1.0) { continue; }
        if(int(Normals[i].W) != 1)
        {
            Triangles.Add(i);
            Triangles.Add(i + MeshRes + 1);
            Triangles.Add(i + MeshRes + 2);

            Triangles.Add(i);
            Triangles.Add(i + MeshRes + 2);
            Triangles.Add(i + 1);
        }
        else
        {
            Triangles.Add(i);
            Triangles.Add(i + MeshRes + 1);
            Triangles.Add(i + 1);

            Triangles.Add(i + 1);
            Triangles.Add(i + MeshRes + 1);
            Triangles.Add(i + MeshRes + 2);
        }
    }

    RenderMesh->CreateMeshSection(0, BuilderVertices, Triangles, BuilderNormals, BuilderUVs,BuilderColors, BuilderTangents, true);
    RenderMesh->SetMaterial(0, RenderMaterial);

    BuilderVertices.Empty();
    BuilderNormals.Empty();
    BuilderNormals.Empty();
    BuilderUVs.Empty();
    BuilderTangents.Empty();
    BuilderColors.Empty();

    Vertices.Empty();
    Normals.Empty();
    Triangles.Empty();

    FMeshAttributes MeshAttributes;
    MeshAttributes.RenderMesh = RenderMesh;
    MeshAttributes.RenderMaterial = RenderMaterial;

    return MeshAttributes;
}


#endif