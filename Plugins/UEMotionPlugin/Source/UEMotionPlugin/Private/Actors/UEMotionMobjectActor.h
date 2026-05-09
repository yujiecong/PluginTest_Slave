#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UEMotionMobjectActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

UCLASS()
class AUEMotionMobjectActor : public AActor
{
	GENERATED_BODY()

public:
	AUEMotionMobjectActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Interp, Category = "UEMotion")
	float Opacity;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	void SetOpacity(float InOpacity);

	UFUNCTION(BlueprintPure, Category = "UEMotion")
	float GetOpacity() const { return Opacity; }

	UFUNCTION(BlueprintPure, Category = "UEMotion")
	UStaticMeshComponent* GetMeshComponent() const;

	UMaterialInstanceDynamic* GetDynamicMaterial() const { return DynamicMaterial; }

	void EnsureDynamicMaterial();

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;

	void ApplyOpacityToMaterial();
};