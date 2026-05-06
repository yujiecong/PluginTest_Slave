#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionFadeAnimation.generated.h"

class UUEMotionMobject;

UCLASS(BlueprintType)
class UUEMotionFadeAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetFadeOut(bool bOut) { bFadeOut = bOut; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetTargetMobject(UUEMotionMobject* InTarget);

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	UUEMotionMobject* TargetMobject = nullptr;

	bool bFadeOut = false;
};
