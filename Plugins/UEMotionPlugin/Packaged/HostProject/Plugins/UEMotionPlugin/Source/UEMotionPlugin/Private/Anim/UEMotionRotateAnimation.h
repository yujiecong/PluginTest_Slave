#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionRotateAnimation.generated.h"

class UUEMotionMobject;

UCLASS(BlueprintType)
class UUEMotionRotateAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetRotationAngle(float Angle) { TargetAngle = Angle; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetAxis(const FVector& InAxis) { Axis = InAxis; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetTargetMobject(UUEMotionMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UUEMotionMobject* TargetMobject = nullptr;

	float TargetAngle = 360.0f;
	FVector Axis = FVector(0, 0, 1);
	FRotator StartRotation = FRotator::ZeroRotator;
};
