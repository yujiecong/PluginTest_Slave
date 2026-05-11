#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionMoveAnimation.generated.h"

class UUEMotionMobject;

UCLASS(BlueprintType)
class UUEMotionMoveAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetTarget(const FVector& InTarget) { End = InTarget; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetStart(const FVector& InStart) { Start = InStart; bStartSet = true; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetTargetMobject(UUEMotionMobject* InTarget);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	UUEMotionMobject* GetTargetMobject() const { return TargetMobject; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	FVector GetTargetLocation() const { return End; }

protected:
	virtual void UpdateAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UUEMotionMobject* TargetMobject = nullptr;

	FVector Start = FVector::ZeroVector;
	FVector End = FVector(100, 0, 0);
	bool bStartSet = false;
};
