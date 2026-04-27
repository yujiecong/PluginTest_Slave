// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDInfoCacheObject.h"

#include "Objects/USDInfoCache.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MyUSDInfoCacheObject)

UMyUsdInfoCache::UMyUsdInfoCache()
{
	Impl = MakePimpl<FUsdInfoCache>();
}

void UMyUsdInfoCache::Serialize(FArchive& Ar)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UMyUsdInfoCache::Serialize);

	if (FUsdInfoCache* ImplPtr = Impl.Get())
	{
		ImplPtr->Serialize(Ar);
	}
}

FUsdInfoCache& UMyUsdInfoCache::GetInner()
{
	return *Impl;
}

const FUsdInfoCache& UMyUsdInfoCache::GetInner() const
{
	return *Impl;
}
