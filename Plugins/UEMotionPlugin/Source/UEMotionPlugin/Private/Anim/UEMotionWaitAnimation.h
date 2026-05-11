#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionWaitAnimation.generated.h"

UCLASS(BlueprintType)
class UUEMotionWaitAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

protected:
	virtual void UpdateAnimation(float DeltaTime, float EasedProgress) override {}
};
