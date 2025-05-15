/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#include "ExoSkyPlanetsShared.h"
#include "Interfaces/IPluginManager.h" 

#define LOCTEXT_NAMESPACE "FExoSkyPlanetsShared"

void FExoSkyPlanetsShared::StartupModule() 
{
    
}

void FExoSkyPlanetsShared::ShutdownModule()
{

}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FExoSkyPlanetsShared, ExoSkyPlanetsShared)