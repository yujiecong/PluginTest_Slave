// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"

#include "MyUSDTransactor.generated.h"

#define UE_API MYUSDSTAGE_API

class AMyUsdStageActor;

namespace UsdUtils
{
	class FMyUsdTransactorImpl;

	struct FSdfChangeListEntry;
	using FObjectChangesByPath = TMap<FString, TArray<FSdfChangeListEntry>>;
}

namespace UE
{
	namespace UsdTransactor
	{
		/** Actors and components cleared to be transacted by ConcertSync even if they are transient will receive this tag */
		extern MYUSDSTAGE_API const FName ConcertSyncEnableTag;
	}
}

/**
 * Class that allows us to log prim attribute changes into the unreal transaction buffer.
 * The AMyUsdStageActor owns one of these, and whenever a USD notice is fired this class transacts and serializes
 * the notice data with itself. When undo/redoing it applies its values to the AUsdStageActors' current stage.
 *
 * Additionally this class naturally allows multi-user (ConcertSync) support for USD stage interactions, by letting
 * these notice data to be mirrored on other clients.
 */
UCLASS(MinimalAPI)
class UMyUsdTransactor : public UObject
{
	GENERATED_BODY()

public:
	// Boilerplate for Pimpl usage with UObject
	UE_API UMyUsdTransactor();
	UE_API UMyUsdTransactor(FVTableHelper& Helper);
	UE_API ~UMyUsdTransactor();

	UE_API void Initialize(AMyUsdStageActor* InStageActor);
	UE_API void Update(const UsdUtils::FObjectChangesByPath& NewInfoChanges, const UsdUtils::FObjectChangesByPath& NewResyncChanges);

	// Begin UObject interface
	UE_API virtual void Serialize(FArchive& Ar) override;
#if WITH_EDITOR
	UE_API virtual void PreEditUndo();
	UE_API virtual void PostEditUndo();
#endif	  // WITH_EDITOR
		  //~ End UObject interface

private:
	TWeakObjectPtr<AMyUsdStageActor> StageActor;

	TUniquePtr<UsdUtils::FMyUsdTransactorImpl> Impl;
};

#undef UE_API
