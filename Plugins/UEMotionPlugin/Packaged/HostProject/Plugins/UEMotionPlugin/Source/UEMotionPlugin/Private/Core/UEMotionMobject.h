#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionMobject.generated.h"

class AUEMotionSceneActor;
class UProceduralMeshComponent;
class UMaterialInterface;

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
	void Destroy();

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Mobject")
	FString GetName() const;

	void SetMobjectName(const FString& Name) { MobjectName = Name; }

private:
	void CreateStaticMeshActor(AUEMotionSceneActor* Owner, const FString& MeshPath, float InScale);

	bool bVisible = true;
	FLinearColor CurrentColor = FLinearColor::White;
	FString MobjectName;

	UPROPERTY()
	TWeakObjectPtr<AActor> InternalActor;

	UPROPERTY()
	TWeakObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY()
	TWeakObjectPtr<UMaterialInstanceDynamic> MaterialInstance;
};
