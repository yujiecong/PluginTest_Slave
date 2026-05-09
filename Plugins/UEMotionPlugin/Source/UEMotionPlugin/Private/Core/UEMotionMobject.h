#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionMobject.generated.h"

class AUEMotionSceneActor;
class AUEMotionMobjectActor;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;

UCLASS(BlueprintType)
class UUEMotionMobject : public UObject
{
	GENERATED_BODY()

public:
	void InitializeAsSphere(AUEMotionSceneActor* Owner, float Radius);
	void InitializeAsMesh(AUEMotionSceneActor* Owner, const FString& MeshPath, float Scale);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void SetLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	FVector GetLocation() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void SetColor(const FLinearColor& Color);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	FLinearColor GetColor() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void SetVisibility(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	bool GetVisibility() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void SetScale(const FVector& Scale);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	FVector GetScale() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void SetRotation(const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	FRotator GetRotation() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void SetOpacity(float InOpacity);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	float GetOpacity() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void Destroy();

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	FString GetName() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	void GetBounds(FVector& OutOrigin, FVector& OutBoxExtent) const;

	void SetMobjectName(const FString& Name) { MobjectName = Name; }

	AActor* GetInternalActor() const { return InternalActor.Get(); }

	void InitializeFromSpawnedActor(AActor* SpawnedActor, UStaticMeshComponent* MeshComp);

	void SetSourceAssetPath(const FString& Path) { SourceAssetPath = Path; bIsAssetBased = !Path.IsEmpty(); }

	FString GetSourceAssetPath() const { return SourceAssetPath; }

	bool IsAssetBased() const { return bIsAssetBased; }

private:
	void CreateStaticMeshActor(AUEMotionSceneActor* Owner, const FString& MeshPath, float InScale);
	UMaterialInterface* GetOrCreateBaseMaterial();

	bool bVisible = true;
	float CurrentOpacity = 1.0f;
	FLinearColor CurrentColor = FLinearColor::White;
	FString MobjectName;

	UPROPERTY()
	TWeakObjectPtr<AActor> InternalActor;

	UPROPERTY()
	TWeakObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY()
	TWeakObjectPtr<UMaterialInstanceDynamic> MaterialInstance;

	UPROPERTY()
	UMaterialInterface* CachedBaseMaterial = nullptr;

	FString SourceAssetPath;
	bool bIsAssetBased = false;
};
