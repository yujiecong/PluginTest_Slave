// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/PimplPtr.h"

#include "MyUSDInfoCacheObject.generated.h"

#define UE_API MYUSDSTAGE_API

class FMyUsdInfoCache;
namespace UE
{
	class FSdfPath;
}

/**
 * Minimal UObject wrapper around FMyUsdInfoCache, since we want this data to be
 * owned by an independently serializable UObject, but the implementation must be in
 * an RTTI-enabled module
 */
UCLASS(MinimalAPI)
class UMyUsdInfoCache : public UObject
{
	GENERATED_BODY()

public:
	UE_API UMyUsdInfoCache();

	// Begin UObject interface
	UE_API virtual void Serialize(FArchive& Ar) override;
	// End UObject interface

	UE_API FMyUsdInfoCache& GetInner();
	UE_API const FMyUsdInfoCache& GetInner() const;

private:
	TPimplPtr<FMyUsdInfoCache> Impl;
};

#undef UE_API
