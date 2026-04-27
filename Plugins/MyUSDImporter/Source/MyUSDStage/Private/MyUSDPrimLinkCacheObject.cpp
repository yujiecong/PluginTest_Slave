// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDPrimLinkCacheObject.h"

#include "Objects/MyUSDPrimLinkCache.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(USDPrimLinkCacheObject)

UMyUsdPrimLinkCache::UMyUsdPrimLinkCache()
{
	Impl = MakePimpl<FUsdPrimLinkCache>();
}

void UMyUsdPrimLinkCache::Serialize(FArchive& Ar)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UMyUsdPrimLinkCache::Serialize);

	if (FUsdPrimLinkCache* ImplPtr = Impl.Get())
	{
		ImplPtr->Serialize(Ar);
	}
}

FUsdPrimLinkCache& UMyUsdPrimLinkCache::GetInner()
{
	return *Impl;
}

const FUsdPrimLinkCache& UMyUsdPrimLinkCache::GetInner() const
{
	return *Impl;
}
