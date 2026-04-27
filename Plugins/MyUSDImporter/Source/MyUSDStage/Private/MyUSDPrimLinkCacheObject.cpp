// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDPrimLinkCacheObject.h"

#include "Objects/MyUSDPrimLinkCache.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(USDPrimLinkCacheObject)

UMyUsdPrimLinkCache::UMyUsdPrimLinkCache()
{
	Impl = MakePimpl<FMyUsdPrimLinkCache>();
}

void UMyUsdPrimLinkCache::Serialize(FArchive& Ar)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UMyUsdPrimLinkCache::Serialize);

	if (FMyUsdPrimLinkCache* ImplPtr = Impl.Get())
	{
		ImplPtr->Serialize(Ar);
	}
}

FMyUsdPrimLinkCache& UMyUsdPrimLinkCache::GetInner()
{
	return *Impl;
}

const FMyUsdPrimLinkCache& UMyUsdPrimLinkCache::GetInner() const
{
	return *Impl;
}
