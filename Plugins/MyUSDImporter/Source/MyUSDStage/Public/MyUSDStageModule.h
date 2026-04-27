// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Modules/ModuleInterface.h"
#include "Templates/SharedPointerFwd.h"

class AMyUsdStageActor;
class UWorld;
class ISequencer;

class IMyUsdStageModule : public IModuleInterface
{
public:
	virtual AMyUsdStageActor& GetUsdStageActor(UWorld* World) = 0;
	virtual AMyUsdStageActor* FindUsdStageActor(UWorld* World) = 0;

#if WITH_EDITOR
	virtual const TArray<TWeakPtr<ISequencer>>& GetExistingSequencers() const = 0;
#endif	  // WITH_EDITOR
};
