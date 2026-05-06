#pragma once

#include "CoreMinimal.h"
#include "Anim/PyManimAnimation.h"
#include "PyManimGroupAnimation.generated.h"

UCLASS(BlueprintType)
class UPyManimGroupAnimation : public UPyManimAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void AddAnimation(UPyManimAnimation* Animation);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	void SetPlayMode(bool bSequential) { bIsSequential = bSequential; }

	UFUNCTION(BlueprintCallable, Category = "PyManim|Animation")
	int32 GetAnimationCount() const { return Animations.Num(); }

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	TArray<UPyManimAnimation*> Animations;

	bool bIsSequential = false;
	float SequentialElapsed = 0.0f;
	int32 CurrentSequentialIndex = 0;
};
