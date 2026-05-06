#pragma once

#include "CoreMinimal.h"
#include "Anim/PyManimAnimation.h"
#include "PyManimWaitAnimation.generated.h"

UCLASS(BlueprintType)
class UPyManimWaitAnimation : public UPyManimAnimation
{
	GENERATED_BODY()

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override {}
};
