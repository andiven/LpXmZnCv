/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thank you to Usualspace, Aiden Soedjarwo, and many others for your help.
This would not be possible without them. */

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Modules/ModuleManager.h"

DECLARE_STATS_GROUP(TEXT("ExoskyPlanetsCompute"), STATGROUP_ExoskyPlanetsCompute, STATCAT_Advanced);

class FExoSkyPlanetsCompute : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
