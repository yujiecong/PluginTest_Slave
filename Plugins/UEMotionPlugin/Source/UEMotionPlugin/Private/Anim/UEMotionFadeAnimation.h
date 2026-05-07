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
	void SetFadeIn(bool bIn) { bFadeIn = bIn; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetFadeOut(bool bOut) { bFadeIn = !bOut; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetTargetMobject(UUEMotionMobject* InTarget);

	virtual void Reset() override;

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	TWeakObjectPtr<UUEMotionMobject> TargetMobject;

	bool bFadeIn = false;
};
