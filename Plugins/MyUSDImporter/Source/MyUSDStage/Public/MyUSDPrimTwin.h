// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "Delegates/DelegateCombinations.h"
#include "UObject/WeakObjectPtr.h"

#include "MyUSDPrimTwin.generated.h"

/** The Unreal equivalent (twin) of a USD prim */
UCLASS(Transient, MinimalAPI)
class UMyUsdPrimTwin final : public UObject
{
	GENERATED_BODY()

public:
	UMyUsdPrimTwin& AddChild(const FString& InPrimPath);
	void RemoveChild(const TCHAR* InPrimPath);
	const TMap<FString, TObjectPtr<UMyUsdPrimTwin>>& GetChildren() const
	{
		return Children;
	}

	UMyUsdPrimTwin* GetParent() const
	{
		return Parent.Get();
	}

	void Clear();

	void Iterate(TFunction<void(UMyUsdPrimTwin&)> Func, bool bRecursive)
	{
		for (TMap<FString, TObjectPtr<UMyUsdPrimTwin>>::TIterator It = Children.CreateIterator(); It; ++It)
		{
			// During some busy transitions it is possible that our children were already destroyed by
			// the time we try clearing our own children
			if (UMyUsdPrimTwin* Child = It->Value)
			{
				Func(*Child);

				if (bRecursive)
				{
					Child->Iterate(Func, bRecursive);
				}
			}
		}
	}

	UMyUsdPrimTwin* Find(const FString& InPrimPath);
	UMyUsdPrimTwin* Find(const USceneComponent* InSceneComponent);

	USceneComponent* GetSceneComponent() const;

	// Begin UObject interface
	void Serialize(FArchive& Ar) override;
	// End UObject interface

	DECLARE_EVENT_OneParam(UMyUsdPrimTwin, FOnUsdPrimTwinDestroyed, const UMyUsdPrimTwin&);
	FOnUsdPrimTwinDestroyed OnDestroyed;

public:
	const static FName GetChildrenPropertyName()
	{
		return GET_MEMBER_NAME_CHECKED(UMyUsdPrimTwin, Children);
	}

public:
	UPROPERTY()
	FString PrimPath;

	UPROPERTY()
	TWeakObjectPtr<class USceneComponent> SceneComponent;

private:
	// Transient as we don't want to save this to disk, but we'll implement Serialize() to
	// duplicate this over when we're being duplicated
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UMyUsdPrimTwin>> Children;

	UPROPERTY(Transient)
	TWeakObjectPtr<UMyUsdPrimTwin> Parent;
};
