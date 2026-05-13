#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionMaterialManager.generated.h"

class UMaterialInterface;
class UMaterial;
class UMaterialInstanceConstant;
class UMaterialInstanceDynamic;

UCLASS(BlueprintType)
class UUEMotionMaterialManager : public UObject
{
	GENERATED_BODY()

public:
	UMaterialInterface* GetOrCreateBlackFloorMaterial();

	UMaterialInterface* GetOrCreateBlackFloorParentMaterial(const FString& ParentMaterialPath = TEXT("/Game/UEMotion/Materials/M_BlackFloorParent"));

	UMaterialInterface* EnsureBaseTranslucentMaterial(const FString& MaterialPath = TEXT("/Game/UEMotion/Materials/M_UEMotionBaseTranslucent"));

	UMaterialInterface* GetOrCreateBaseMaterial();

private:
	UPROPERTY()
	UMaterialInterface* CachedBlackFloorMaterial = nullptr;

	UPROPERTY()
	UMaterialInterface* CachedBlackFloorParentMaterial = nullptr;

	UPROPERTY()
	UMaterialInterface* CachedBaseTranslucentMaterial = nullptr;

	UPROPERTY()
	UMaterialInterface* CachedBaseMaterial = nullptr;

	bool SaveAssetToObject(UObject* Asset, const FString& PackagePath, const FString& AssetName);
};
