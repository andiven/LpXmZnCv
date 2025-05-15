/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

using UnrealBuildTool;

public class ExoSkyPlanetsPlugin : ModuleRules
{
	public ExoSkyPlanetsPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		int MeshBackend = 0;

		switch(MeshBackend) 
		{
			case 0 : //Dynamic Mesh Component Backend
				PublicDependencyModuleNames.Add("GeometryCore");
				PublicDependencyModuleNames.Add("GeometryFramework");
				PublicDependencyModuleNames.Add("DynamicMesh");

				PublicDefinitions.Add("USE_DMC=1"); 
				PublicDefinitions.Add("USE_RMC=0");
				PublicDefinitions.Add("USE_PMC=0");
				break;
			case 1 : //Realtime Mesh Component Backend
				PublicDependencyModuleNames.Add("RealtimeMeshComponent");

				PublicDefinitions.Add("USE_DMC=0");
				PublicDefinitions.Add("USE_RMC=1");
				PublicDefinitions.Add("USE_PMC=0");
				break;
			case 2 : //Procedural Mesh Component Backend
				PublicDependencyModuleNames.Add("ProceduralMeshComponent");

				PublicDefinitions.Add("USE_DMC=0");
				PublicDefinitions.Add("USE_RMC=0");
				PublicDefinitions.Add("USE_PMC=1");
				break;
			default : //Realtime Mesh Pro Support (Coming Soon, Third Party Purchase Required.)
				break;
		}

		bUseUnity = false;
		
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"RHI",
				"RenderCore",
				"Renderer",
				"CoreUObject",
				"Projects",
				"ExoSkyPlanetsShared",
				"Engine",
				"UMG",
				"ExoSkyPlanetsCompute"
			}
		);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
			}
		);

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				
			}
		);
	}
}