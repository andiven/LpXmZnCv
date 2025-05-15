/* Copyright 2021-2024 "EXOSKY PLANETS", made by Collin Blumenauer. All Rights Reserved.

Thanks to my community and my close graphics programming friends.
This would not be possible without them. */

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include "Styling/SlateStyleRegistry.h" 
#include "Styling/SlateStyle.h" 


class FExoSkyPlanetsShared : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
