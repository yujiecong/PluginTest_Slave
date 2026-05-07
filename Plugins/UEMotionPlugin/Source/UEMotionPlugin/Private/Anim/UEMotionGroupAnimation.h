#pragma once

#include "CoreMinimal.h"
#include "Anim/UEMotionAnimation.h"
#include "UEMotionGroupAnimation.generated.h"

UCLASS(BlueprintType)
class UUEMotionGroupAnimation : public UUEMotionAnimation
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void AddAnimation(UUEMotionAnimation* Animation);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	void SetPlayMode(bool bSequential) { bIsSequential = bSequential; }

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Animation")
	const TArray<UUEMotionAnimation*>& GetAnimations() const { return Animations; }

	virtual void Reset() override;

protected:
	virtual void TickAnimation(float DeltaTime, float EasedProgress) override;

private:
	UPROPERTY()
	TArray<UUEMotionAnimation*> Animations;

	bool bIsSequential = false;
};
