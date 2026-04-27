// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "UniversalObjectLocatorFwd.h"

#include "MyUSDLocatorFragments.generated.h"

class AMyUsdStageActor;

/**
 * 32 Bytes (40 in-editor).
 */
USTRUCT()
struct FMyUsdPrimLocatorFragment
{
	GENERATED_BODY()

	UPROPERTY()
	bool bPreferComponent = false;

	UPROPERTY()
	FString PrimPath;

	MYUSDSTAGE_API static UE::UniversalObjectLocator::TFragmentTypeHandle<FMyUsdPrimLocatorFragment> FragmentType;

	friend uint32 GetTypeHash(const FMyUsdPrimLocatorFragment& A)
	{
		return GetTypeHash(A.PrimPath) ^ GetTypeHash(A.bPreferComponent);
	}

	friend bool operator==(const FMyUsdPrimLocatorFragment& A, const FMyUsdPrimLocatorFragment& B)
	{
		return A.bPreferComponent == B.bPreferComponent && A.PrimPath == B.PrimPath;
	}

	UE::UniversalObjectLocator::FResolveResult Resolve(const UE::UniversalObjectLocator::FResolveParams& Params) const;
	UE::UniversalObjectLocator::FInitializeResult Initialize(const UE::UniversalObjectLocator::FInitializeParams& InParams);

	void ToString(FStringBuilderBase& OutStringBuilder) const;
	UE::UniversalObjectLocator::FParseStringResult TryParseString(FStringView InString, const UE::UniversalObjectLocator::FParseStringParams& Params);

	static uint32 ComputePriority(const UObject* Object, const UObject* Context);

	static const AMyUsdStageActor* GetAttachParentStageActor(const UObject* Object);
	static AMyUsdStageActor* GetAttachParentStageActor(UObject* Object);
};

template<>
struct TStructOpsTypeTraits<FMyUsdPrimLocatorFragment> : public TStructOpsTypeTraitsBase2<FMyUsdPrimLocatorFragment>
{
	enum
	{
		WithIdenticalViaEquality = true,
	};
};
