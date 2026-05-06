#pragma once

#include "CoreMinimal.h"
#include "Anim/PyManimAnimation.h"
#include "PyManimRotateAnimation.generated.h"

class UPyManimMobject;

UCLASS(BlueprintType)
class UPyManimRotateAnimation : public UPyManimAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetRotationAngle(float Angle) { TargetAngle = Angle; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetAxis(const FVector& InAxis) { Axis = InAxis; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetTargetMobject(UPyManimMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UPyManimMobject* TargetMobject = nullptr;

	float TargetAngle = 360.0f;
	FVector Axis = FVector(0, 0, 1);
	FRotator StartRotation = FRotator::ZeroRotator;
};
