#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UEMotionAxisActor.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class AUEMotionAxisActor : public AActor
{
	GENERATED_BODY()

public:
	AUEMotionAxisActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Axis")
	void InitializeAxis(EAxis::Type InAxis, float InLength, const FLinearColor& InColor);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Axis")
	EAxis::Type GetAxisType() const { return static_cast<EAxis::Type>(AxisType.GetValue()); }

	UFUNCTION(BlueprintPure, Category = "UEMotion|Axis")
	float GetAxisLength() const { return AxisLength; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Axis")
	void SetAxisColor(const FLinearColor& InColor);

	UFUNCTION(BlueprintPure, Category = "UEMotion|Axis")
	UStaticMeshComponent* GetMeshComponent() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Axis")
	static bool CreateAxisMaterials();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UEMotion|Axis")
	UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Axis")
	TEnumAsByte<EAxis::Type> AxisType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Axis")
	float AxisLength = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Axis")
	FLinearColor AxisColor = FLinearColor::Red;

private:
	UPROPERTY()
	UMaterialInterface* AxisMaterial;

	void SetupMesh();
	void ApplyRotationForAxis();
	void CreateOrLoadAxisMaterial();
	static UMaterialInterface* CreateStaticAxisMaterial(const FString& MaterialName, const FLinearColor& Color);
};
