/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "ExoSkyPlanetsPlugin.h"
#include "Interfaces/IPluginManager.h" 

#define LOCTEXT_NAMESPACE "FExoSkyPlanetsPluginModule"

void FExoSkyPlanetsPluginModule::StartupModule() 
{
    StyleSet = MakeShareable(new FSlateStyleSet("ExoSkyPlanetsStyleSet"));

    FString ResourcesDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / TEXT("ExoSkyPlanetsPlugin/Resources/"));
    StyleSet->Set("ClassThumbnail.ExoskyPlanetBase", new FSlateImageBrush(ResourcesDir + "PlanetLogo64.png", FVector2D(64, 64)));
    StyleSet->Set("ClassIcon.ExoskyPlanetBase", new FSlateImageBrush(ResourcesDir + "PlanetLogo16.png", FVector2D(16, 16)));
    StyleSet->Set("ClassThumbnail.ExoskyFoliageType", new FSlateImageBrush(ResourcesDir + "FoliageIcon64.png", FVector2D(64, 64)));
    StyleSet->Set("ClassIcon.ExoskyFoliageType", new FSlateImageBrush(ResourcesDir + "FoliageIcon16.png", FVector2D(64, 64)));

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FExoSkyPlanetsPluginModule::ShutdownModule()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
    ensure(StyleSet.IsUnique());
    StyleSet.Reset();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FExoSkyPlanetsPluginModule, ExoSkyPlanetsPlugin)