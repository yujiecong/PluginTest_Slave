#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionScaleAnimation.generated.h"

class UUEMotionMobject;

UCLASS(BlueprintType)
class UUEMotionScaleAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetStartScale(const FVector& Scale) { StartScale = Scale; bStartSet = true; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetEndScale(const FVector& Scale) { EndScale = Scale; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetTargetMobject(UUEMotionMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UUEMotionMobject* TargetMobject = nullptr;

	FVector StartScale = FVector(0.5f);
	FVector EndScale = FVector(2.0f);
	bool bStartSet = false;
};
