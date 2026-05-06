#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PyManimMobject.generated.h"

class APyManimSceneActor;
class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS(BlueprintType)
class UPyManimMobject : public UObject
{
	GENERATED_BODY()

public:
	void InitializeAsSphere(APyManimSceneActor* Owner, float Radius);
	void InitializeAsMesh(APyManimSceneActor* Owner, const FString& MeshPath, float Scale);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	void SetLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	FVector GetLocation() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	void SetColor(const FLinearColor& Color);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	FLinearColor GetColor() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	void SetVisibility(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	bool GetVisibility() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	void SetScale(const FVector& Scale);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	FVector GetScale() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	void SetRotation(const FRotator& Rotation);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	FRotator GetRotation() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	void Destroy();

	UFUNCTION(BlueprintCallable, Category = "PyManim|Mobject")
	FString GetName() const;

	void SetMobjectName(const FString& Name) { MobjectName = Name; }

private:
	void CreateStaticMeshActor(APyManimSceneActor* Owner, const FString& MeshPath, float InScale);

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
