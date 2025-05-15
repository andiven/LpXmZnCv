/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

using UnrealBuildTool; 

public class ExoSkyPlanetsCompute: ModuleRules 

{ 

	public ExoSkyPlanetsCompute(ReadOnlyTargetRules Target) : base(Target) 

	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateIncludePaths.AddRange(new string[] 
		{
			"ExoSkyPlanetsCompute/Private"
		});
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.Add("TargetPlatform");
		}

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"Engine",
			"MaterialShaderQualitySettings",
			"ExoSkyPlanetsShared",
			"Renderer",
			"RenderCore",
			"RHI",
			"Projects"
		});

		bUseUnity = false;
		
		// PrivateDependencyModuleNames.AddRange(new string[]
		// {
		// 	"CoreUObject",
		// 	"Renderer",
		// 	"RenderCore",
		// 	"RHI",
		// 	"Projects",
		// 	"SlateCore",
		// 	"Slate"
		// });
		
		if (Target.bBuildEditor == true)
		{

			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"UnrealEd",
					"MaterialUtilities"
				}
			);

			/*CircularlyReferencedDependentModules.AddRange(
				new string[] {
					"UnrealEd",
					"MaterialUtilities",
				}
			);*/
		}
	} 

}