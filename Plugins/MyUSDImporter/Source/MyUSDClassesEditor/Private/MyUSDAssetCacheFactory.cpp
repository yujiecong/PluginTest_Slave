// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDAssetCacheFactory.h"

#include "MyUSDAssetCache3.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(USDAssetCacheFactory)

UMyUsdAssetCacheFactory::UMyUsdAssetCacheFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UUsdAssetCache3::StaticClass();
}

UObject* UMyUsdAssetCacheFactory::FactoryCreateNew(
	UClass* Class,
	UObject* InParent,
	FName Name,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn
)
{
	return NewObject<UUsdAssetCache3>(InParent, Name, Flags | RF_Transactional | RF_Public | RF_Standalone);
}

bool UMyUsdAssetCacheFactory::ShouldShowInNewMenu() const
{
	return true;
}
