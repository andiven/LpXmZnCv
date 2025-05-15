/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

using UnrealBuildTool;

public class ExoSkyPlanetsShared : ModuleRules
{
	public ExoSkyPlanetsShared(ReadOnlyTargetRules Target) : base(Target)
	{
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
				"Engine",
				"UMG"
			}
		);

		bUseUnity = false;
			
		// PrivateDependencyModuleNames.AddRange(
		// 	new string[]
		// 	{
		// 		"CoreUObject",
		// 		"Engine",
		// 		"Slate",
		// 		"SlateCore",
        //         "UMG"
		// 	}
		// );
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				
			}
		);
	}
}