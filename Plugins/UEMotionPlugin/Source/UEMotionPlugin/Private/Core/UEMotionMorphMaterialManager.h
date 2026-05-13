#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionMorphMaterialManager.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMesh;
class UUEMotionMobject;

UCLASS(BlueprintType)
class UUEMotionMorphMaterialManager : public UObject
{
	GENERATED_BODY()

public:

	UMaterialInterface* GetOrCreateMorphMaterial(
		UUEMotionMobject* SourceMobject,
		const TArray<FVector>& SourceVertices,
		const TArray<FVector>& TargetVertices
	);

	bool ApplyMorphToMobject(
		UUEMotionMobject* Mobject,
		float MorphProgress
	);

	void SetMorphProgress(UUEMotionMobject* Mobject, float Progress);

	static UUEMotionMorphMaterialManager* Get();

private:
	UMaterialInterface* CreateMorphParentMaterial();
	UMaterialInstanceDynamic* CreateMorphMaterialInstance(
		UMaterialInterface* ParentMaterial,
		const TArray<FVector>& SourceVertices,
		const TArray<FVector>& TargetVertices
	);

	UPROPERTY()
	TMap<UUEMotionMobject*, UMaterialInstanceDynamic*> ActiveMorphMaterials;

	UPROPERTY()
	UMaterialInterface* CachedParentMaterial = nullptr;

	static UUEMotionMorphMaterialManager* Instance;
};
