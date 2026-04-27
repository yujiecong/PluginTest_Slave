// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/PimplPtr.h"

#include "MyUSDPrimLinkCacheObject.generated.h"

#define UE_API MYUSDSTAGE_API

class FUsdPrimLinkCache;
namespace UE
{
	class FSdfPath;
}

/**
 * Minimal UObject wrapper around FUsdPrimLinkCache, since we want this to be accessible
 * from MyUSDSchemas which is an RTTI-enabled module, but also to be owned by an independently
 * serializable UObject
 */
UCLASS(MinimalAPI)
class UMyUsdPrimLinkCache : public UObject
{
	GENERATED_BODY()

public:
	UE_API UMyUsdPrimLinkCache();

	// Begin UObject interface
	UE_API virtual void Serialize(FArchive& Ar) override;
	// End UObject interface

	UE_API FUsdPrimLinkCache& GetInner();
	UE_API const FUsdPrimLinkCache& GetInner() const;

private:
	TPimplPtr<FUsdPrimLinkCache> Impl;
};

#undef UE_API
