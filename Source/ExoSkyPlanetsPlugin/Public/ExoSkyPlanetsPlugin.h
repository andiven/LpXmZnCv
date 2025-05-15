/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Styling/SlateStyleRegistry.h" 
#include "Styling/SlateStyle.h" 

DECLARE_STATS_GROUP(TEXT("ExoskyPlanets"), STATGROUP_ExoskyPlanets, STATCAT_Advanced);
// No need for this right now. These will be implemented LATER. After console commands. 
//DECLARE_(TEXT("Quadtree Update"), STAT_QuadtreeUpdate, STATGROUP_ExoskyPlanets);
//DECLARE_CYCLE_STAT(TEXT("Quadtree Update"), STAT_QuadtreeUpdate, STATGROUP_ExoskyPlanets);
//DECLARE_CYCLE_STAT(TEXT("Create Layer Texture"), STAT_CreateLayer, STATGROUP_ExoskyPlanets);
//DECLARE_CYCLE_STAT(TEXT("Create "), STAT_, STATGROUP_ExoskyPlanets);
//
class FExoSkyPlanetsPluginModule : public IModuleInterface
{
public:
	static TSharedPtr<FSlateStyleSet> StyleSet;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

TSharedPtr< FSlateStyleSet > FExoSkyPlanetsPluginModule::StyleSet = nullptr;
